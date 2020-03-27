// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "DataStructures/DataBox/DataBox.hpp"
#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/VariablesTag.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Parallel/Invoke.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/Literals.hpp"
#include "Utilities/Requires.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TaggedTuple.hpp"
#include "Utilities/TypeTraits.hpp"

/// \cond
// IWYU pragma: no_forward_declare db::DataBox
namespace intrp {
template <class Metavariables>
struct Interpolator;
template <typename Metavariables, typename InterpolationTargetTag>
class InterpolationTarget;
namespace Actions {
template <typename InterpolationTargetTag>
struct CleanUpInterpolator;
}  // namespace Actions
namespace Tags {
template <typename TemporalId>
struct IndicesOfFilledInterpPoints;
template <typename TemporalId>
struct CompletedTemporalIds;
template <typename TemporalId>
struct TemporalIds;
}  // namespace Tags
}  // namespace intrp
template <typename TagsList>
struct Variables;
/// \endcond

namespace intrp {

namespace InterpolationTarget_detail {

// apply_callback accomplishes the overload for the
// two signatures of callback functions.
// Uses SFINAE on return type.
template <typename T, typename DbTags, typename Metavariables,
          typename TemporalId>
auto apply_callback(
    const gsl::not_null<db::DataBox<DbTags>*> box,
    const gsl::not_null<Parallel::ConstGlobalCache<Metavariables>*> cache,
    const TemporalId& temporal_id) noexcept
    -> decltype(T::post_interpolation_callback::apply(box, cache, temporal_id),
                bool()) {
  return T::post_interpolation_callback::apply(box, cache, temporal_id);
}

template <typename T, typename DbTags, typename Metavariables,
          typename TemporalId>
auto apply_callback(
    const gsl::not_null<db::DataBox<DbTags>*> box,
    const gsl::not_null<Parallel::ConstGlobalCache<Metavariables>*> cache,
    const TemporalId& temporal_id) noexcept
    -> decltype(T::post_interpolation_callback::apply(*box, *cache,
                                                      temporal_id),
                bool()) {
  T::post_interpolation_callback::apply(*box, *cache, temporal_id);
  // For the simpler callback function, we will always clean up volume data, so
  // we return true here.
  return true;
}

// type trait testing if the class has a fill_invalid_points_with member.
template <typename T, typename = void>
struct has_fill_invalid_points_with : std::false_type {};

template <typename T>
struct has_fill_invalid_points_with<
    T, cpp17::void_t<decltype(
           T::post_interpolation_callback::fill_invalid_points_with)>>
    : std::true_type {};

template <typename T>
constexpr bool has_fill_invalid_points_with_v =
    has_fill_invalid_points_with<T>::value;

// Fills invalid points with some constant value.
template <typename InterpolationTargetTag, typename TemporalId, typename DbTags,
          Requires<not has_fill_invalid_points_with_v<InterpolationTargetTag>> =
              nullptr>
void fill_invalid_points(const gsl::not_null<db::DataBox<DbTags>*> /*box*/,
                         const TemporalId& /*temporal_id*/) noexcept {}

template <
    typename InterpolationTargetTag, typename TemporalId, typename DbTags,
    Requires<has_fill_invalid_points_with_v<InterpolationTargetTag>> = nullptr>
void fill_invalid_points(const gsl::not_null<db::DataBox<DbTags>*> box,
                         const TemporalId& temporal_id) noexcept {
  const auto& invalid_indices =
      db::get<Tags::IndicesOfInvalidInterpPoints<TemporalId>>(*box);
  if (invalid_indices.find(temporal_id) != invalid_indices.end() and
      not invalid_indices.at(temporal_id).empty()) {
    db::mutate<Tags::IndicesOfInvalidInterpPoints<TemporalId>,
               Tags::InterpolatedVars<InterpolationTargetTag, TemporalId>>(
        box, [&temporal_id](
                 const gsl::not_null<std::unordered_map<
                     TemporalId, std::unordered_set<size_t>>*>
                     indices_of_invalid_points,
                 const gsl::not_null<std::unordered_map<
                     TemporalId, Variables<typename InterpolationTargetTag::
                                               vars_to_interpolate_to_target>>*>
                     vars_dest_all_times) noexcept {
          auto& vars_dest = vars_dest_all_times->at(temporal_id);
          const size_t npts_dest = vars_dest.number_of_grid_points();
          const size_t nvars = vars_dest.number_of_independent_components;
          for (auto index : indices_of_invalid_points->at(temporal_id)) {
            for (size_t v = 0; v < nvars; ++v) {
              // clang-tidy: no pointer arithmetic
              vars_dest.data()[index + v * npts_dest] =  // NOLINT
                  InterpolationTargetTag::post_interpolation_callback::
                      fill_invalid_points_with;
            }
          }
          // Further functions may test if there are invalid points.
          // Clear the invalid points now, since we have filled them.
          indices_of_invalid_points->erase(temporal_id);
        });
  }
}

/// Calls the callback function, tells interpolators to clean up the current
/// temporal_id, and then if there are more temporal_ids to be interpolated,
/// starts the next one.
template <typename InterpolationTargetTag, typename DbTags,
          typename Metavariables, typename TemporalId>
void callback_and_cleanup(
    const gsl::not_null<db::DataBox<DbTags>*> box,
    const gsl::not_null<Parallel::ConstGlobalCache<Metavariables>*> cache,
    const TemporalId& temporal_id) noexcept {

  // Before doing anything else, deal with the possibility that some
  // of the points might be outside of the Domain.
  fill_invalid_points<InterpolationTargetTag>(box, temporal_id);

  // Fill ::Tags::Variables<typename
  //      InterpolationTargetTag::vars_to_interpolate_to_target>
  // with variables from correct temporal_id.
  db::mutate_apply<
      tmpl::list<::Tags::Variables<
          typename InterpolationTargetTag::vars_to_interpolate_to_target>>,
      tmpl::list<Tags::InterpolatedVars<InterpolationTargetTag, TemporalId>>>(
      [&temporal_id](
          const gsl::not_null<Variables<
              typename InterpolationTargetTag::vars_to_interpolate_to_target>*>
              vars,
          const std::unordered_map<
              TemporalId, Variables<typename InterpolationTargetTag::
                                        vars_to_interpolate_to_target>>&
              vars_at_all_times) noexcept {
        *vars = vars_at_all_times.at(temporal_id);
      },
      box);

  // apply_callback should return true if we are done with this
  // temporal_id.  It should return false only if the callback
  // calls another `intrp::Action` that needs the volume data at this
  // same temporal_id.  If it returns false, we exit here and do not
  // clean up.
  const bool done_with_temporal_id =
      apply_callback<InterpolationTargetTag>(box, cache, temporal_id);

  if (not done_with_temporal_id) {
    return;
  }

  // We are now done with this temporal_id, so we can pop it and
  // clean up volume data associated with it.
  db::mutate<Tags::TemporalIds<TemporalId>,
             Tags::CompletedTemporalIds<TemporalId>>(
      box, [](const gsl::not_null<std::deque<TemporalId>*> ids,
              const gsl::not_null<std::deque<TemporalId>*>
                  completed_ids) noexcept {
        completed_ids->push_back(ids->front());
        ids->pop_front();
        // We want to keep track of all completed temporal_ids to deal with
        // the possibility of late calls to
        // AddTemporalIdsToInterpolationTarget.  We could keep all
        // completed_ids forever, but we probably don't want it to get too
        // large, so we limit its size.  We assume that
        // asynchronous calls to AddTemporalIdsToInterpolationTarget do not span
        // more than 1000 temporal_ids.
        if (completed_ids->size() > 1000) {
          completed_ids->pop_front();
        }
      });

  // Tell interpolators to clean up at this temporal_id for this
  // InterpolationTargetTag.
  auto& interpolator_proxy =
      Parallel::get_parallel_component<Interpolator<Metavariables>>(*cache);
  Parallel::simple_action<Actions::CleanUpInterpolator<InterpolationTargetTag>>(
      interpolator_proxy, temporal_id);

  // If we have a sequential target, and there are further
  // temporal_ids, begin interpolation for the next one.
  if (InterpolationTargetTag::compute_target_points::is_sequential::value) {
    const auto& temporal_ids = db::get<Tags::TemporalIds<TemporalId>>(*box);
    if (not temporal_ids.empty()) {
      auto& my_proxy = Parallel::get_parallel_component<
          InterpolationTarget<Metavariables, InterpolationTargetTag>>(*cache);
      Parallel::simple_action<
          typename InterpolationTargetTag::compute_target_points>(
          my_proxy, temporal_ids.front());
    }
  }
}

}  // namespace InterpolationTarget_detail

namespace Actions {
/// \ingroup ActionsGroup
/// \brief Receives interpolated variables from an `Interpolator` on a subset
///  of the target points.
///
/// If interpolated variables for all target points have been received, then
/// - Calls `InterpolationTargetTag::post_interpolation_callback`
/// - Tells `Interpolator`s that the interpolation is complete
///  (by calling
///  `Actions::CleanUpInterpolator<InterpolationTargetTag>`)
/// - Removes the first `temporal_id` from `Tags::TemporalIds<TemporalId>`
/// - If there are more `temporal_id`s, begins interpolation at the next
///  `temporal_id` (by calling `InterpolationTargetTag::compute_target_points`)
///
/// Uses:
/// - DataBox:
///   - `Tags::TemporalIds<TemporalId>`
///   - `Tags::IndicesOfFilledInterpPoints<TemporalId>`
///   - `Tags::InterpolatedVars<InterpolationTargetTag,TemporalId>`
///
/// DataBox changes:
/// - Adds: nothing
/// - Removes: nothing
/// - Modifies:
///   - `Tags::TemporalIds<TemporalId>`
///   - `Tags::CompletedTemporalIds<TemporalId>`
///   - `Tags::IndicesOfFilledInterpPoints<TemporalId>`
///   - `Tags::InterpolatedVars<InterpolationTargetTag,TemporalId>`
///   - `::Tags::Variables<typename
///                   InterpolationTargetTag::vars_to_interpolate_to_target>`
///
/// For requirements on InterpolationTargetTag, see InterpolationTarget
template <typename InterpolationTargetTag>
struct InterpolationTargetReceiveVars {
  /// For requirements on Metavariables, see InterpolationTarget
  template <typename ParallelComponent, typename DbTags, typename Metavariables,
            typename ArrayIndex, typename TemporalId,
            Requires<tmpl::list_contains_v<
                DbTags, Tags::TemporalIds<TemporalId>>> = nullptr>
  static void apply(
      db::DataBox<DbTags>& box,
      Parallel::ConstGlobalCache<Metavariables>& cache,
      const ArrayIndex& /*array_index*/,
      const std::vector<Variables<
          typename InterpolationTargetTag::vars_to_interpolate_to_target>>&
          vars_src,
      const std::vector<std::vector<size_t>>& global_offsets,
      const TemporalId& temporal_id) noexcept {
    db::mutate<Tags::IndicesOfFilledInterpPoints<TemporalId>,
               Tags::InterpolatedVars<InterpolationTargetTag, TemporalId>>(
        make_not_null(&box),
        [&temporal_id, &vars_src, &global_offsets ](
            const gsl::not_null<
                std::unordered_map<TemporalId, std::unordered_set<size_t>>*>
                indices_of_filled,
            const gsl::not_null<std::unordered_map<
                TemporalId, Variables<typename InterpolationTargetTag::
                                          vars_to_interpolate_to_target>>*>
                vars_dest_all_times) noexcept {
          auto& vars_dest = (*vars_dest_all_times)[temporal_id];
          const size_t npts_dest = vars_dest.number_of_grid_points();
          const size_t nvars = vars_dest.number_of_independent_components;
          for (size_t j = 0; j < global_offsets.size(); ++j) {
            const size_t npts_src = global_offsets[j].size();
            for (size_t i = 0; i < npts_src; ++i) {
              // If a point is on the boundary of two (or more)
              // elements, it is possible that we have received data
              // for this point from more than one Interpolator.
              // This will rarely occur, but it does occur, e.g. when
              // a point is exactly on some symmetry
              // boundary (such as the x-y plane) and this symmetry
              // boundary is exactly the boundary between two
              // elements.  If this happens, we accept the first
              // duplicated point, and we ignore subsequent
              // duplicated points.  The points are easy to keep track
              // of because global_offsets uniquely identifies them.
              if ((*indices_of_filled)[temporal_id]
                      .insert(global_offsets[j][i])
                      .second) {
                for (size_t v = 0; v < nvars; ++v) {
                  // clang-tidy: no pointer arithmetic
                  vars_dest.data()[global_offsets[j][i] +    // NOLINT
                                   v * npts_dest] =          // NOLINT
                      vars_src[j].data()[i + v * npts_src];  // NOLINT
                }
              }
            }
          }
        });

    const size_t filled_size =
        db::get<Tags::IndicesOfFilledInterpPoints<TemporalId>>(box)
            .at(temporal_id)
            .size();
    const size_t invalid_size = [&box, &temporal_id ]() noexcept {
      const auto& invalid_indices =
          db::get<Tags::IndicesOfInvalidInterpPoints<TemporalId>>(box);
      if (invalid_indices.count(temporal_id) > 0) {
        return invalid_indices.at(temporal_id).size();
      }
      return 0_st;
    }
    ();
    const size_t interp_size =
        db::get<Tags::InterpolatedVars<InterpolationTargetTag, TemporalId>>(box)
            .at(temporal_id)
            .number_of_grid_points();
    if (invalid_size + filled_size == interp_size) {
      // All the valid points have been interpolated.
      InterpolationTarget_detail::callback_and_cleanup<InterpolationTargetTag>(
          make_not_null(&box), make_not_null(&cache), temporal_id);
    }
  }
};
}  // namespace Actions
}  // namespace intrp
