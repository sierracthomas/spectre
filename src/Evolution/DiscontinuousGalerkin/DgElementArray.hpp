// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>
#include <vector>

#include "AlgorithmArray.hpp"
#include "Domain/Block.hpp"
#include "Domain/Creators/DomainCreator.hpp"
#include "Domain/Domain.hpp"
#include "Domain/ElementId.hpp"
#include "Domain/InitialElementIds.hpp"
#include "Domain/OptionTags.hpp"
#include "Domain/Tags.hpp"
#include "Evolution/Protocols.hpp"
#include "Evolution/Tags.hpp"
#include "IO/Importers/VolumeDataReader.hpp"
#include "IO/Importers/VolumeDataReaderActions.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Parallel/Info.hpp"
#include "Parallel/ParallelComponentHelpers.hpp"
#include "ParallelAlgorithms/Actions/SetData.hpp"
#include "Utilities/ProtocolHelpers.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TaggedTuple.hpp"

/*!
 * \brief Don't try to import initial data from a data file
 *
 * \see `DgElementArray`
 */
struct ImportNoInitialData {};

/*!
 * \brief Import numeric initial data in the `ImportPhase`
 *
 * Requires the `InitialData` conforms to
 * `evolution::protocols::NumericInitialData`. The `InitialData::import_fields`
 * will be imported in the `ImportPhase` from the data file that is specified by
 * the options in the `evolution::OptionTags::NumericInitialData` group.
 *
 * \see `DgElementArray`
 */
template <typename PhaseType, PhaseType ImportPhase, typename InitialData>
struct ImportNumericInitialData {
  static_assert(
      tt::conforms_to_v<InitialData, evolution::protocols::NumericInitialData>);
};

namespace DgElementArray_detail {

template <typename DgElementArray, typename InitialData,
          typename ImportFields = typename InitialData::import_fields>
using read_element_data_action = importers::ThreadedActions::ReadVolumeData<
    evolution::OptionTags::NumericInitialData, ImportFields,
    ::Actions::SetData<ImportFields>, DgElementArray>;

template <typename DgElementArray, typename ImportInitialData>
struct import_numeric_data_cache_tags {
  using type = tmpl::list<>;
};

template <typename DgElementArray, typename PhaseType, PhaseType ImportPhase,
          typename InitialData>
struct import_numeric_data_cache_tags<
    DgElementArray,
    ImportNumericInitialData<PhaseType, ImportPhase, InitialData>> {
  using type =
      typename read_element_data_action<DgElementArray,
                                        InitialData>::const_global_cache_tags;
};

template <typename DgElementArray, typename ImportInitialData>
struct try_import_data {
  template <typename Metavariables>
  static void apply(const typename Metavariables::Phase /*next_phase*/,
                    Parallel::CProxy_ConstGlobalCache<
                        Metavariables>& /*global_cache*/) noexcept {}
};

template <typename DgElementArray, typename PhaseType, PhaseType ImportPhase,
          typename InitialData>
struct try_import_data<
    DgElementArray,
    ImportNumericInitialData<PhaseType, ImportPhase, InitialData>> {
  template <typename Metavariables>
  static void apply(
      const typename Metavariables::Phase next_phase,
      Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache) noexcept {
    static_assert(
        std::is_same_v<PhaseType, typename Metavariables::Phase>,
        "Make sure the 'ImportNumericInitialData' type uses a 'Phase' "
        "that is defined in the Metavariables.");
    if (next_phase == ImportPhase) {
      auto& local_cache = *(global_cache.ckLocalBranch());
      Parallel::threaded_action<
          read_element_data_action<DgElementArray, InitialData>>(
          Parallel::get_parallel_component<
              importers::VolumeDataReader<Metavariables>>(local_cache));
    }
  }
};

}  // namespace DgElementArray_detail

/*!
 * \brief The parallel component responsible for managing the DG elements that
 * compose the computational domain
 *
 * This parallel component will perform the actions specified by the
 * `PhaseDepActionList`.
 *
 * This component also supports loading initial for its elements from a file.
 * To do so, set the `ImportInitialData` template parameter to
 * `ImportNumericInitialData`. See the documentation of the
 * `ImportNumericInitialData` for details.
 */
template <class Metavariables, class PhaseDepActionList,
          class ImportInitialData = ImportNoInitialData>
struct DgElementArray {
  static constexpr size_t volume_dim = Metavariables::volume_dim;

  using chare_type = Parallel::Algorithms::Array;
  using metavariables = Metavariables;
  using phase_dependent_action_list = PhaseDepActionList;
  using array_index = ElementId<volume_dim>;

  using const_global_cache_tags = tmpl::flatten<tmpl::list<
      domain::Tags::Domain<volume_dim>,
      tmpl::type_from<DgElementArray_detail::import_numeric_data_cache_tags<
          DgElementArray, ImportInitialData>>>>;

  using array_allocation_tags =
      tmpl::list<domain::Tags::InitialRefinementLevels<volume_dim>>;

  using initialization_tags = Parallel::get_initialization_tags<
      Parallel::get_initialization_actions_list<phase_dependent_action_list>,
      array_allocation_tags>;

  static void allocate_array(
      Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache,
      const tuples::tagged_tuple_from_typelist<initialization_tags>&
          initialization_items) noexcept;

  static void execute_next_phase(
      const typename Metavariables::Phase next_phase,
      Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache) noexcept {
    auto& local_cache = *(global_cache.ckLocalBranch());
    Parallel::get_parallel_component<DgElementArray>(local_cache)
        .start_phase(next_phase);

    DgElementArray_detail::try_import_data<
        DgElementArray, ImportInitialData>::apply(next_phase, global_cache);
  }
};

template <class Metavariables, class PhaseDepActionList,
          class ImportInitialData>
void DgElementArray<Metavariables, PhaseDepActionList, ImportInitialData>::
    allocate_array(
        Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache,
        const tuples::tagged_tuple_from_typelist<initialization_tags>&
            initialization_items) noexcept {
  auto& local_cache = *(global_cache.ckLocalBranch());
  auto& dg_element_array =
      Parallel::get_parallel_component<DgElementArray>(local_cache);
  const auto& domain =
      Parallel::get<domain::Tags::Domain<volume_dim>>(local_cache);
  const auto& initial_refinement_levels =
      get<domain::Tags::InitialRefinementLevels<volume_dim>>(
          initialization_items);
  int which_proc = 0;
  for (const auto& block : domain.blocks()) {
    const auto initial_ref_levs = initial_refinement_levels[block.id()];
    const std::vector<ElementId<volume_dim>> element_ids =
        initial_element_ids(block.id(), initial_ref_levs);
    const int number_of_procs = Parallel::number_of_procs();
    for (size_t i = 0; i < element_ids.size(); ++i) {
      dg_element_array(ElementId<volume_dim>(element_ids[i]))
          .insert(global_cache, initialization_items, which_proc);
      which_proc = which_proc + 1 == number_of_procs ? 0 : which_proc + 1;
    }
  }
  dg_element_array.doneInserting();
}
