// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include "AlgorithmNodegroup.hpp"
#include "IO/Importers/Tags.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Parallel/ParallelComponentHelpers.hpp"
#include "Parallel/PhaseDependentActionList.hpp"
#include "ParallelAlgorithms/Initialization/MergeIntoDataBox.hpp"

namespace importers {

namespace detail {
struct InitializeVolumeDataReader;
}  // namespace detail

/*!
 * \brief A nodegroup parallel component that reads in a volume data file and
 * distributes its data to elements of an array parallel component.
 *
 * Each element of the array parallel component must register itself before
 * data can be sent to it. To do so, invoke
 * `importers::Actions::RegisterWithVolumeDataReader` on each element. In a
 * subsequent phase you can then invoke
 * `importers::ThreadedActions::ReadVolumeData` on the `VolumeDataReader`
 * component to read in the file and distribute its data to the registered
 * elements.
 */
template <typename Metavariables>
struct VolumeDataReader {
  using chare_type = Parallel::Algorithms::Nodegroup;
  using metavariables = Metavariables;
  using phase_dependent_action_list = tmpl::list<Parallel::PhaseActions<
      typename Metavariables::Phase, Metavariables::Phase::Initialization,
      tmpl::list<detail::InitializeVolumeDataReader>>>;
  using initialization_tags = Parallel::get_initialization_tags<
      Parallel::get_initialization_actions_list<phase_dependent_action_list>>;

  static void initialize(Parallel::CProxy_ConstGlobalCache<
                         Metavariables>& /*global_cache*/) noexcept {}

  static void execute_next_phase(
      const typename Metavariables::Phase next_phase,
      Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache) noexcept {
    auto& local_cache = *(global_cache.ckLocalBranch());
    Parallel::get_parallel_component<VolumeDataReader>(local_cache)
        .start_phase(next_phase);
  }
};

namespace detail {
struct InitializeVolumeDataReader {
  template <typename DbTagsList, typename... InboxTags, typename Metavariables,
            typename ArrayIndex, typename ActionList,
            typename ParallelComponent>
  static auto apply(db::DataBox<DbTagsList>& box,
                    const tuples::TaggedTuple<InboxTags...>& /*inboxes*/,
                    const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const ActionList /*meta*/,
                    const ParallelComponent* const /*meta*/) noexcept {
    using simple_tags = db::AddSimpleTags<Tags::RegisteredElements>;
    using compute_tags = db::AddComputeTags<>;

    return std::make_tuple(
        ::Initialization::merge_into_databox<InitializeVolumeDataReader,
                                             simple_tags, compute_tags>(
            std::move(box), db::item_type<Tags::RegisteredElements>{}),
        true);
  }
};
}  // namespace detail

}  // namespace importers
