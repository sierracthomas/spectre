// Distributed under the MIT License.
// See LICENSE.txt for details.

#define CATCH_CONFIG_RUNNER

#include "tests/Unit/TestingFramework.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AlgorithmArray.hpp"
#include "AlgorithmSingleton.hpp"
#include "DataStructures/DataBox/DataBox.hpp"  // IWYU pragma: keep
#include "ErrorHandling/Error.hpp"
#include "ErrorHandling/FloatingPointExceptions.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Parallel/Info.hpp"
#include "Parallel/InitializationFunctions.hpp"
#include "Parallel/Invoke.hpp"
#include "Parallel/Main.hpp"
#include "Parallel/ParallelComponentHelpers.hpp"
#include "Parallel/PhaseDependentActionList.hpp"  // IWYU pragma: keep
#include "Parallel/Reduction.hpp"
#include "Utilities/ConstantExpressions.hpp"
#include "Utilities/EqualWithinRoundoff.hpp"
#include "Utilities/Functional.hpp"
#include "Utilities/TMPL.hpp"

namespace db {
template <typename TagsList>
class DataBox;
}  // namespace db
struct TestMetavariables;

namespace {
size_t throw_darts(const size_t number_of_throws) {
  std::random_device
      device;  // Will be used to obtain a seed for the random number engine
  std::mt19937 generator(
      device());  // Standard mersenne_twister_engine seeded with device()
  std::uniform_real_distribution<> distribution(0.0, 1.0);
  size_t dart_hits = 0;
  for (size_t i = 0; i < number_of_throws; ++i) {
    const double x = distribution(generator);
    const double y = distribution(generator);
    if (x * x + y * y <= 1) {
      ++dart_hits;
    }
  }
  return dart_hits;
}
}  // namespace

static constexpr int number_of_1d_array_elements = 240;
static constexpr size_t darts = 1000000000000;
static constexpr size_t darts_thrown = darts / number_of_1d_array_elements;

/// [reduce_sum_int_action]
struct ProcessReducedSumOfInts {
  template <typename ParallelComponent, typename DbTags, typename Metavariables,
            typename ArrayIndex>
  static void apply(db::DataBox<DbTags>& /*box*/,
                    const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const size_t& value) noexcept {
    double pi = (value * 4.0) / (number_of_1d_array_elements * darts_thrown);
    std::cout << std::setprecision(16) << pi << std::endl;
  }
};
/// [reduce_sum_int_action]

template <class Metavariables>
struct SingletonParallelComponent {
  using chare_type = Parallel::Algorithms::Singleton;
  using metavariables = Metavariables;
  using phase_dependent_action_list =
      tmpl::list<Parallel::PhaseActions<typename Metavariables::Phase,
                                        Metavariables::Phase::Initialization,
                                        tmpl::list<>>>;
  using initialization_tags = Parallel::get_initialization_tags<
      Parallel::get_initialization_actions_list<phase_dependent_action_list>>;

  static void execute_next_phase(
      const typename Metavariables::Phase /*next_phase*/,
      const Parallel::CProxy_ConstGlobalCache<
          Metavariables>& /*global_cache*/) {}
};

template <class Metavariables>
struct ArrayParallelComponent;

struct ArrayReduce {
  template <typename ParallelComponent, typename DbTags, typename Metavariables,
            typename ArrayIndex>
  static void apply(const db::DataBox<DbTags>& /*box*/,
                    const Parallel::ConstGlobalCache<Metavariables>& cache,
                    const ArrayIndex& array_index) noexcept {
    static_assert(cpp17::is_same_v<ParallelComponent,
                                   ArrayParallelComponent<TestMetavariables>>,
                  "The ParallelComponent is not deduced to be the right type");
    const auto& my_proxy =
        Parallel::get_parallel_component<ArrayParallelComponent<Metavariables>>(
            cache)[array_index];
    const auto& singleton_proxy = Parallel::get_parallel_component<
        SingletonParallelComponent<Metavariables>>(cache);
    /// [contribute_to_reduction_example]
    Parallel::ReductionData<Parallel::ReductionDatum<size_t, funcl::Plus<>>>
        my_send_int{throw_darts(darts_thrown)};
    Parallel::contribute_to_reduction<ProcessReducedSumOfInts>(
        my_send_int, my_proxy, singleton_proxy);
    /// [contribute_to_reduction_example]
  }
};

template <class Metavariables>
struct ArrayParallelComponent {
  using chare_type = Parallel::Algorithms::Array;
  using metavariables = Metavariables;
  using array_index = int;
  using phase_dependent_action_list =
      tmpl::list<Parallel::PhaseActions<typename Metavariables::Phase,
                                        Metavariables::Phase::Initialization,
                                        tmpl::list<>>>;
  using initialization_tags = Parallel::get_initialization_tags<
      Parallel::get_initialization_actions_list<phase_dependent_action_list>>;

  static void allocate_array(
      Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache,
      const tuples::tagged_tuple_from_typelist<initialization_tags>&
      /*initialization_items*/) noexcept {
    auto& local_cache = *(global_cache.ckLocalBranch());
    auto& array_proxy =
        Parallel::get_parallel_component<ArrayParallelComponent>(local_cache);

    for (int i = 0, which_proc = 0,
             number_of_procs = Parallel::number_of_procs();
         i < number_of_1d_array_elements; ++i) {
      array_proxy[i].insert(global_cache, {}, which_proc);
      which_proc = which_proc + 1 == number_of_procs ? 0 : which_proc + 1;
    }
    array_proxy.doneInserting();
  }

  static void execute_next_phase(
      const typename Metavariables::Phase next_phase,
      Parallel::CProxy_ConstGlobalCache<Metavariables>& global_cache) noexcept {
    auto& local_cache = *(global_cache.ckLocalBranch());
    if (next_phase == Metavariables::Phase::CallArrayReduce) {
      Parallel::simple_action<ArrayReduce>(
          Parallel::get_parallel_component<ArrayParallelComponent>(
              local_cache));
    }
  }
};

struct TestMetavariables {
  using component_list =
      tmpl::list<SingletonParallelComponent<TestMetavariables>,
                 ArrayParallelComponent<TestMetavariables>>;

  static constexpr const char* const help{"Test reductions using Algorithm"};
  static constexpr bool ignore_unrecognized_command_line_options = false;

  enum class Phase { Initialization, CallArrayReduce, Exit };
  static Phase determine_next_phase(const Phase& current_phase,
                                    const Parallel::CProxy_ConstGlobalCache<
                                        TestMetavariables>& /*cache_proxy*/) {
    return current_phase == Phase::Initialization ? Phase::CallArrayReduce
                                                  : Phase::Exit;
  }
};

static const std::vector<void (*)()> charm_init_node_funcs{
    &setup_error_handling};
static const std::vector<void (*)()> charm_init_proc_funcs{
    &enable_floating_point_exceptions};

using charmxx_main_component = Parallel::Main<TestMetavariables>;

#include "Parallel/CharmMain.tpp"  // IWYU pragma: keep
