// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>
#include <tuple>
#include <utility>

#include "DataStructures/DataBox/DataBox.hpp"
#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataBox/PrefixHelpers.hpp"
#include "DataStructures/Variables.hpp"
#include "DataStructures/VariablesTag.hpp"
#include "Evolution/Systems/Cce/OptionTags.hpp"
#include "NumericalAlgorithms/Spectral/SwshInterpolation.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "ParallelAlgorithms/Initialization/MergeIntoDataBox.hpp"
#include "Utilities/Requires.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TaggedTuple.hpp"

namespace Cce {
/// \brief The set of actions for use in the CCE evolution system
namespace Actions {

/*!
 * \ingroup ActionsGroup
 * \brief Initializes the main data storage for the  `CharacteristicEvolution`
 * component, which is the singleton that handles the main evolution system for
 * CCE computations.
 *
 * \details Sets up the \ref DataBoxGroup to be ready to take data from the
 * worldtube component, calculate initial data, and start the hypersurface
 * computations.
 *
 * \ref DataBoxGroup changes:
 * - Modifies: nothing
 * - Adds:
 *  - `metavariables::evolved_coordinates_variables_tag`
 *  -
 * ```
 * db::add_tag_prefix<Tags::dt,
 * metavariables::evolved_coordinates_variables_tag>
 * ```
 *  - `Tags::Variables<metavariables::cce_angular_coordinate_tags>`
 *  - `Tags::Variables<metavariables::cce_scri_tags>`
 *  -
 * ```
 * Tags::Variables<tmpl::append<
 * metavariables::cce_integrand_tags,
 * metavariables::cce_integration_independent_tags,
 * metavariables::cce_temporary_equations_tags>>
 * ```
 *  - `Tags::Variables<metavariables::cce_pre_swsh_derivatives_tags>`
 *  - `Tags::Variables<metavariables::cce_transform_buffer_tags>`
 *  - `Tags::Variables<metavariables::cce_swsh_derivative_tags>`
 *  - `Spectral::Swsh::Tags::SwshInterpolator< Tags::CauchyAngularCoords>`
 * - Removes: nothing
 */
struct InitializeCharacteristicEvolutionVariables {
  using const_global_cache_tags =
      tmpl::list<Tags::LMax, Tags::NumberOfRadialPoints>;

  template <
      typename DbTags, typename... InboxTags, typename Metavariables,
      typename ArrayIndex, typename ActionList, typename ParallelComponent,
      Requires<not tmpl::list_contains_v<
          DbTags, ::Tags::Variables<tmpl::list<
                      typename Metavariables::evolved_swsh_tag>>>> = nullptr>
  static auto apply(db::DataBox<DbTags>& box,
                    const tuples::TaggedTuple<InboxTags...>& /*inboxes*/,
                    const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const ActionList /*meta*/,
                    const ParallelComponent* const /*meta*/) noexcept {
    using boundary_value_variables_tag = ::Tags::Variables<
        tmpl::append<typename Metavariables::cce_boundary_communication_tags,
                     typename Metavariables::cce_gauge_boundary_tags>>;
    using scri_variables_tag =
        ::Tags::Variables<typename Metavariables::cce_scri_tags>;
    using volume_variables_tag = ::Tags::Variables<
        tmpl::append<typename Metavariables::cce_integrand_tags,
                     typename Metavariables::cce_integration_independent_tags,
                     typename Metavariables::cce_temporary_equations_tags>>;
    using pre_swsh_derivatives_variables_tag = ::Tags::Variables<
        typename Metavariables::cce_pre_swsh_derivatives_tags>;
    using transform_buffer_variables_tag =
        ::Tags::Variables<typename Metavariables::cce_transform_buffer_tags>;
    using swsh_derivative_variables_tag =
        ::Tags::Variables<typename Metavariables::cce_swsh_derivative_tags>;
    using angular_coordinates_variables_tag =
        ::Tags::Variables<typename Metavariables::cce_angular_coordinate_tags>;
    using coordinate_variables_tag =
        typename Metavariables::evolved_coordinates_variables_tag;
    using dt_coordinate_variables_tag =
        db::add_tag_prefix<::Tags::dt, coordinate_variables_tag>;
    using evolved_swsh_variables_tag =
        ::Tags::Variables<tmpl::list<typename Metavariables::evolved_swsh_tag>>;
    using evolved_swsh_dt_variables_tag =
        db::add_tag_prefix<::Tags::dt, evolved_swsh_variables_tag>;

    const size_t l_max = db::get<Spectral::Swsh::Tags::LMaxBase>(box);
    const size_t number_of_radial_points =
        db::get<Spectral::Swsh::Tags::NumberOfRadialPointsBase>(box);
    const size_t boundary_size =
        Spectral::Swsh::number_of_swsh_collocation_points(l_max);
    const size_t volume_size = boundary_size * number_of_radial_points;
    const size_t transform_buffer_size =
        number_of_radial_points *
        Spectral::Swsh::size_of_libsharp_coefficient_vector(l_max);
    return std::make_tuple(
        Initialization::merge_into_databox<
            InitializeCharacteristicEvolutionVariables,
            db::AddSimpleTags<
                boundary_value_variables_tag, coordinate_variables_tag,
                dt_coordinate_variables_tag, evolved_swsh_variables_tag,
                evolved_swsh_dt_variables_tag,
                angular_coordinates_variables_tag, scri_variables_tag,
                volume_variables_tag, pre_swsh_derivatives_variables_tag,
                transform_buffer_variables_tag, swsh_derivative_variables_tag,
                Spectral::Swsh::Tags::SwshInterpolator<
                    Tags::CauchyAngularCoords>>,
            db::AddComputeTags<>, Initialization::MergePolicy::Overwrite>(
            std::move(box),
            db::item_type<boundary_value_variables_tag>{boundary_size},
            db::item_type<coordinate_variables_tag>{boundary_size},
            db::item_type<dt_coordinate_variables_tag>{boundary_size},
            db::item_type<evolved_swsh_variables_tag>{volume_size},
            db::item_type<evolved_swsh_dt_variables_tag>{volume_size},
            db::item_type<angular_coordinates_variables_tag>{boundary_size},
            db::item_type<scri_variables_tag>{boundary_size},
            db::item_type<volume_variables_tag>{volume_size},
            db::item_type<pre_swsh_derivatives_variables_tag>{volume_size, 0.0},
            db::item_type<transform_buffer_variables_tag>{transform_buffer_size,
                                                          0.0},
            db::item_type<swsh_derivative_variables_tag>{volume_size, 0.0},
            Spectral::Swsh::SwshInterpolator{}));
  }

  template <
      typename DbTags, typename... InboxTags, typename Metavariables,
      typename ArrayIndex, typename ActionList, typename ParallelComponent,
      Requires<tmpl::list_contains_v<
          DbTags, ::Tags::Variables<tmpl::list<
                      typename Metavariables::evolved_swsh_tag>>>> = nullptr>
  static std::tuple<db::DataBox<DbTags>&&> apply(
      const db::DataBox<DbTags>& /*box*/,
      const tuples::TaggedTuple<InboxTags...>& /*inboxes*/,
      const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
      const ArrayIndex& /*array_index*/, const ActionList /*meta*/,
      const ParallelComponent* const /*meta*/) noexcept {
    ERROR(
        "The DataBox has already been initialized with Cce characteristic "
        "evolution variables. Only initialize the Cce databox once.");
  }
};

}  // namespace Actions
}  // namespace Cce
