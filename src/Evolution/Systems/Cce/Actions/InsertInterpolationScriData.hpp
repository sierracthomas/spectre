// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <tuple>

#include "DataStructures/DataBox/DataBox.hpp"
#include "Evolution/Systems/Cce/OptionTags.hpp"
#include "Evolution/Systems/Cce/Tags.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Time/Tags.hpp"

namespace Cce {
namespace Actions {

namespace detail {
template <typename Tag>
struct get_interpolator_argument_tag {
  using type = Tag;
};

template <typename Tag>
struct get_interpolator_argument_tag<Tags::Du<Tag>> {
  using type = Tag;
};

template <typename Tag>
struct InsertIntoInterpolationManagerImpl {
  using return_tags =
      tmpl::list<Tags::InterpolationManager<ComplexDataVector, Tag>>;
  using argument_tags =
      tmpl::list<typename get_interpolator_argument_tag<Tag>::type,
                 Tags::InertialRetardedTime>;
  static void apply(
      const gsl::not_null<ScriPlusInterpolationManager<ComplexDataVector, Tag>*>
          interpolation_manager,
      const db::item_type<Tag>& scri_data,
      const Scalar<DataVector>& inertial_retarded_time) noexcept {
    interpolation_manager->insert_data(get(inertial_retarded_time),
                                       get(scri_data).data());
  }
};

template <typename LhsTag, typename RhsTag>
struct InsertIntoInterpolationManagerImpl<::Tags::Multiplies<LhsTag, RhsTag>> {
  using return_tags = tmpl::list<Tags::InterpolationManager<
      ComplexDataVector, ::Tags::Multiplies<LhsTag, RhsTag>>>;
  using argument_tags =
      tmpl::list<typename get_interpolator_argument_tag<LhsTag>::type, RhsTag,
                 Tags::InertialRetardedTime>;
  static void apply(const gsl::not_null<ScriPlusInterpolationManager<
                        ComplexDataVector, ::Tags::Multiplies<LhsTag, RhsTag>>*>
                        interpolation_manager,
                    const db::item_type<LhsTag>& lhs_data,
                    const db::item_type<RhsTag>& rhs_data,
                    const Scalar<DataVector>& inertial_retarded_time) noexcept {
    interpolation_manager->insert_data(get(inertial_retarded_time),
                                       get(lhs_data).data(),
                                       get(rhs_data).data());
  }
};
}  // namespace detail

/*!
 * \ingroup ActionGroup
 * \brief Places the data from the current hypersurface necessary to compute
 * `Tag` in the `ScriPlusInterpolationManager` associated with the `Tag`.
 *
 * \details Adds both the appropriate scri+ value(s) and a number of target
 * inertial times to interpolate of quantity equal to the
 * `InitializationTags::ScriOutputDensity` determined from options, equally
 * spaced between the current time and the next time in the algorithm.
 *
 * Uses:
 * - `::Tags::TimeStepId`
 * - `::Tags::Next<::Tags::TimeStepId>`
 * - `Cce::InitializationTags::ScriOutputDensity`
 * - if `Tag` is `::Tags::Multiplies<Lhs, Rhs>`:
 *   - `Lhs` and `Rhs`
 * - if `Tag` has `Cce::Tags::Du<Argument>`:
 *   - `Argument`
 * - otherwise uses `Tag`
 *
 * \ref DataBoxGroup changes:
 * - Modifies:
 *   - `Tags::InterpolationManager<ComplexDataVector, Tag>`
 * - Adds: nothing
 * - Removes: nothing
 */
template <typename Tag>
struct InsertInterpolationScriData {
  using const_global_cache_tags =
      tmpl::list<InitializationTags::ScriOutputDensity>;
  template <typename DbTags, typename... InboxTags, typename Metavariables,
            typename ArrayIndex, typename ActionList,
            typename ParallelComponent>
  static auto apply(db::DataBox<DbTags>& box,
                    const tuples::TaggedTuple<InboxTags...>& /*inboxes*/,
                    const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const ActionList /*meta*/,
                    const ParallelComponent* const /*meta*/) noexcept {
    if (db::get<::Tags::TimeStepId>(box).substep() == 0) {
      // insert the data points into the interpolator.
      db::mutate_apply<detail::InsertIntoInterpolationManagerImpl<Tag>>(
          make_not_null(&box));

      // insert the target times into the interpolator.
      db::mutate<Tags::InterpolationManager<ComplexDataVector, Tag>>(
          make_not_null(&box),
          [](const gsl::not_null<
                 ScriPlusInterpolationManager<ComplexDataVector, Tag>*>
                 interpolation_manager,
             const TimeStepId& time_id, const TimeStepId& next_time_id,
             const size_t number_of_interpolated_times) noexcept {
            const double time_of_current_step = time_id.substep_time().value();
            const double time_of_next_step =
                next_time_id.substep_time().value();
            for (size_t i = 0; i < number_of_interpolated_times; ++i) {
              interpolation_manager->insert_target_time(
                  time_of_current_step +
                  (time_of_next_step - time_of_current_step) *
                      static_cast<double>(i) /
                      static_cast<double>(number_of_interpolated_times));
            }
          },
          db::get<::Tags::TimeStepId>(box),
          db::get<::Tags::Next<::Tags::TimeStepId>>(box),
          db::get<InitializationTags::ScriOutputDensity>(box));
    }
    return std::forward_as_tuple(std::move(box));
  }
};
}  // namespace Actions
}  // namespace Cce
