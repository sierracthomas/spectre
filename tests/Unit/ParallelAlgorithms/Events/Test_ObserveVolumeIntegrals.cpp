// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "DataStructures/DataBox/DataBox.hpp"
#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataVector.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/Variables.hpp"
#include "Domain/Creators/Brick.hpp"
#include "Domain/Creators/DomainCreator.hpp"
#include "Domain/Creators/Interval.hpp"
#include "Domain/Creators/Rectangle.hpp"
#include "Domain/Domain.hpp"
#include "Domain/ElementMap.hpp"
#include "Domain/LogicalCoordinates.hpp"
#include "Domain/Mesh.hpp"
#include "Domain/Tags.hpp"
#include "Framework/ActionTesting.hpp"
#include "Framework/TestCreation.hpp"
#include "Framework/TestHelpers.hpp"
#include "Helpers/DataStructures/MakeWithRandomValues.hpp"
#include "IO/Observer/ObservationId.hpp"
#include "IO/Observer/ObserverComponent.hpp"
#include "NumericalAlgorithms/LinearOperators/DefiniteIntegral.hpp"
#include "NumericalAlgorithms/Spectral/Spectral.hpp"
#include "Parallel/PhaseDependentActionList.hpp"
#include "Parallel/Reduction.hpp"
#include "Parallel/RegisterDerivedClassesWithCharm.hpp"
#include "ParallelAlgorithms/Events/ObserveVolumeIntegrals.hpp"
#include "ParallelAlgorithms/EventsAndTriggers/Event.hpp"
#include "Utilities/Algorithm.hpp"
#include "Utilities/ConstantExpressions.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/Numeric.hpp"
#include "Utilities/PrettyType.hpp"
#include "Utilities/TMPL.hpp"

namespace Parallel {
template <typename Metavariables>
class ConstGlobalCache;
}  // namespace Parallel
namespace observers {
namespace Actions {
struct ContributeReductionData;
}  // namespace Actions
}  // namespace observers

namespace {

struct ObservationTimeTag : db::SimpleTag {
  using type = double;
};

struct MockContributeReductionData {
  struct Results {
    observers::ObservationId observation_id;
    std::string subfile_name;
    std::vector<std::string> reduction_names{};
    double time;
    double volume;
    std::vector<double> volume_integrals{};
  };
  static Results results;

  template <typename ParallelComponent, typename... DbTags,
            typename Metavariables, typename ArrayIndex, typename... Ts>
  static void apply(db::DataBox<tmpl::list<DbTags...>>& /*box*/,
                    Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const observers::ObservationId& observation_id,
                    const std::string& subfile_name,
                    const std::vector<std::string>& reduction_names,
                    Parallel::ReductionData<Ts...>&& reduction_data) noexcept {
    results.observation_id = observation_id;
    results.subfile_name = subfile_name;
    results.reduction_names = reduction_names;
    results.time = std::get<0>(reduction_data.data());
    results.volume = std::get<1>(reduction_data.data());
    results.volume_integrals = std::get<2>(reduction_data.data());
  }
};

MockContributeReductionData::Results MockContributeReductionData::results{};

template <typename Metavariables>
struct ElementComponent {
  using component_being_mocked = void;

  using metavariables = Metavariables;
  using array_index = int;
  using chare_type = ActionTesting::MockArrayChare;
  using phase_dependent_action_list =
      tmpl::list<Parallel::PhaseActions<typename Metavariables::Phase,
                                        Metavariables::Phase::Initialization,
                                        tmpl::list<>>>;
};

template <typename Metavariables>
struct MockObserverComponent {
  using component_being_mocked = observers::Observer<Metavariables>;
  using replace_these_simple_actions =
      tmpl::list<observers::Actions::ContributeReductionData>;
  using with_these_simple_actions = tmpl::list<MockContributeReductionData>;

  using metavariables = Metavariables;
  using array_index = int;
  using chare_type = ActionTesting::MockArrayChare;
  using phase_dependent_action_list =
      tmpl::list<Parallel::PhaseActions<typename Metavariables::Phase,
                                        Metavariables::Phase::Initialization,
                                        tmpl::list<>>>;
};

struct Metavariables {
  using component_list = tmpl::list<ElementComponent<Metavariables>,
                                    MockObserverComponent<Metavariables>>;
  using const_global_cache_tags = tmpl::list<>;  //  unused
  enum class Phase { Initialization, Testing, Exit };

  struct ObservationType {};
  using element_observation_type = ObservationType;
};

struct ScalarVar : db::SimpleTag {
  using type = Scalar<DataVector>;
};

template <size_t SpatialDim>
struct VectorVar : db::SimpleTag {
  using type = tnsr::I<DataVector, SpatialDim>;
};

template <size_t SpatialDim>
struct TensorVar : db::SimpleTag {
  using type = tnsr::Ij<DataVector, SpatialDim>;
};

template <size_t SpatialDim>
using variables_for_test =
    tmpl::list<ScalarVar, VectorVar<SpatialDim>, TensorVar<SpatialDim>>;

template <size_t SpatialDim>
std::unique_ptr<DomainCreator<SpatialDim>> domain_creator() noexcept;

template <>
std::unique_ptr<DomainCreator<1>> domain_creator() noexcept {
  return std::make_unique<domain::creators::Interval>(
      domain::creators::Interval({{-0.5}}, {{0.5}}, {{false}}, {{0}}, {{4}}));
}

template <>
std::unique_ptr<DomainCreator<2>> domain_creator() noexcept {
  return std::make_unique<domain::creators::Rectangle>(
      domain::creators::Rectangle({{-0.5, -0.5}}, {{0.5, 0.5}},
                                  {{false, false}}, {{0, 0}}, {{4, 4}}));
}

template <>
std::unique_ptr<DomainCreator<3>> domain_creator() noexcept {
  return std::make_unique<domain::creators::Brick>(domain::creators::Brick(
      {{-0.5, -0.5, -0.5}}, {{0.5, 0.5, 0.5}}, {{false, false, false}},
      {{0, 0, 0}}, {{4, 4, 4}}));
}

template <size_t VolumeDim, typename ObserveEvent>
void test_observe(const std::unique_ptr<ObserveEvent> observe) noexcept {
  using metavariables = Metavariables;
  using element_component = ElementComponent<metavariables>;
  using observer_component = MockObserverComponent<metavariables>;

  const typename element_component::array_index array_index(0);
  const ElementId<VolumeDim> element_id{array_index};

  // ObserveVolumeIntegrals requires some domain items in the DataBox.
  // Any domain, mesh and basis should be fine--we'll just check received data
  const auto creator = domain_creator<VolumeDim>();
  const auto domain = creator->create_domain();
  const auto& block = domain.blocks()[element_id.block_id()];

  ElementMap<VolumeDim, Frame::Inertial> map{
      element_id, block.stationary_map().get_clone()};
  Mesh<VolumeDim> mesh{creator->initial_extents()[0],
                       Spectral::Basis::Chebyshev,
                       Spectral::Quadrature::GaussLobatto};
  auto logical_coords = logical_coordinates(mesh);

  // Any data held by tensors to integrate should be fine
  MAKE_GENERATOR(gen);
  using value_type = DataVector::value_type;
  UniformCustomDistribution<tt::get_fundamental_type_t<value_type>> dist{-10.0,
                                                                         10.0};
  Variables<variables_for_test<VolumeDim>> vars(mesh.number_of_grid_points());
  fill_with_random_values(make_not_null(&vars), make_not_null(&gen),
                          make_not_null(&dist));

  // Compute expected data for checks
  size_t num_tensor_components = 0;
  const DataVector det_jacobian =
      get(determinant(map.jacobian(logical_coords)));
  const double expected_volume = definite_integral(det_jacobian, mesh);
  std::vector<double> expected_volume_integrals{};
  std::vector<std::string> expected_reduction_names = {
      db::tag_name<ObservationTimeTag>(), "Volume"};
  tmpl::for_each<typename std::decay_t<decltype(vars)>::tags_list>([
    &num_tensor_components, &vars, &expected_volume_integrals,
    &expected_reduction_names, &det_jacobian, &mesh
  ](auto tag) noexcept {
    using tensor_tag = tmpl::type_from<decltype(tag)>;
    const auto tensor = get<tensor_tag>(vars);
    for (size_t i = 0; i < tensor.size(); ++i) {
      expected_reduction_names.push_back("VolumeIntegral(" +
                                         db::tag_name<tensor_tag>() +
                                         tensor.component_suffix(i) + ")");
      expected_volume_integrals.push_back(
          definite_integral(det_jacobian * tensor[i], mesh));
      ++num_tensor_components;
    }
  });

  const double observation_time = 2.0;
  const auto box = db::create<
      db::AddSimpleTags<ObservationTimeTag, domain::Tags::Mesh<VolumeDim>,
                        domain::Tags::ElementMap<VolumeDim, Frame::Inertial>,
                        domain::Tags::Coordinates<VolumeDim, Frame::Logical>,
                        Tags::Variables<typename decltype(vars)::tags_list>>>(
      observation_time, mesh, std::move(map), logical_coords, vars);

  ActionTesting::MockRuntimeSystem<metavariables> runner{{}};
  ActionTesting::emplace_component<element_component>(make_not_null(&runner),
                                                      0);
  ActionTesting::emplace_component<observer_component>(&runner, 0);

  observe->run(box, runner.cache(), array_index,
               std::add_pointer_t<element_component>{});

  // Process the data
  runner.invoke_queued_simple_action<observer_component>(0);
  CHECK(runner.is_simple_action_queue_empty<observer_component>(0));

  const auto& results = MockContributeReductionData::results;
  CHECK(results.observation_id.value() == observation_time);
  CHECK(results.subfile_name == "/volume_integrals");
  CHECK(results.reduction_names == expected_reduction_names);
  CHECK(results.time == observation_time);
  CHECK(results.volume == expected_volume);
  CHECK(results.volume_integrals.size() == num_tensor_components);
  CHECK(results.volume_integrals == expected_volume_integrals);
}
}  // namespace

template <size_t VolumeDim>
void test_observe_system() noexcept {
  using vars_for_test = variables_for_test<VolumeDim>;

  {
    INFO("Testing observation for Dim = " << VolumeDim);
    test_observe<VolumeDim>(
        std::make_unique<dg::Events::ObserveVolumeIntegrals<
            VolumeDim, ObservationTimeTag, vars_for_test>>());
  }
  {
    INFO("Testing create/serialize for Dim = " << VolumeDim);
    using EventType =
        Event<tmpl::list<dg::Events::Registrars::ObserveVolumeIntegrals<
            VolumeDim, ObservationTimeTag, vars_for_test>>>;
    Parallel::register_derived_classes_with_charm<EventType>();
    const auto factory_event =
        TestHelpers::test_factory_creation<EventType>("ObserveVolumeIntegrals");
    auto serialized_event = serialize_and_deserialize(factory_event);
    test_observe<VolumeDim>(std::move(serialized_event));
  }
}

SPECTRE_TEST_CASE("Unit.Evolution.dG.ObserveVolumeIntegrals",
                  "[Unit][Evolution]") {
  test_observe_system<1>();
  test_observe_system<2>();
  test_observe_system<3>();
}
