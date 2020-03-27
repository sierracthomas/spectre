// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <pup.h>
#include <utility>
#include <vector>

#include "DataStructures/DataBox/DataBox.hpp"
#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataBox/PrefixHelpers.hpp"
#include "DataStructures/DataBox/Prefixes.hpp"
#include "DataStructures/DataVector.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "Domain/CoordinateMaps/Affine.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.tpp"
#include "Domain/Direction.hpp"
#include "Domain/Element.hpp"
#include "Domain/ElementId.hpp"
#include "Domain/ElementIndex.hpp"
#include "Domain/ElementMap.hpp"
#include "Domain/FaceNormal.hpp"
#include "Domain/InterfaceComputeTags.hpp"
#include "Domain/Mesh.hpp"
#include "Domain/Tags.hpp"
#include "Evolution/Actions/ComputeTimeDerivative.hpp"  // IWYU pragma: keep
#include "Evolution/Systems/ScalarWave/Equations.hpp"
#include "Evolution/Systems/ScalarWave/System.hpp"
#include "Framework/ActionTesting.hpp"
#include "Framework/TestCreation.hpp"
#include "Framework/TestHelpers.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/Actions/ApplyFluxes.hpp"  // IWYU pragma: keep
#include "NumericalAlgorithms/DiscontinuousGalerkin/Actions/ComputeNonconservativeBoundaryFluxes.hpp"  // IWYU pragma: keep
#include "NumericalAlgorithms/DiscontinuousGalerkin/Actions/FluxCommunication.hpp"  // IWYU pragma: keep
#include "NumericalAlgorithms/DiscontinuousGalerkin/FluxCommunicationTypes.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/Tags.hpp"
#include "NumericalAlgorithms/LinearOperators/PartialDerivatives.hpp"
#include "NumericalAlgorithms/Spectral/Spectral.hpp"
#include "PointwiseFunctions/AnalyticSolutions/WaveEquation/SemidiscretizedDg.hpp"
#include "Time/Slab.hpp"
#include "Time/Tags.hpp"
#include "Time/Time.hpp"
#include "Time/TimeStepId.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/TMPL.hpp"

namespace {

template <typename Metavariables>
struct Component {
  using metavariables = Metavariables;
  using chare_type = ActionTesting::MockArrayChare;
  using array_index = ElementIndex<1>;
  using const_global_cache_tags = tmpl::list<>;

  using variables_tag = typename metavariables::system::variables_tag;
  using flux_comm_types = dg::FluxCommunicationTypes<Metavariables>;
  using normal_dot_fluxes_tag =
      domain::Tags::Interface<domain::Tags::InternalDirections<1>,
                              typename flux_comm_types::normal_dot_fluxes_tag>;
  using mortar_data_tag = typename flux_comm_types::simple_mortar_data_tag;

  using simple_tags = db::AddSimpleTags<
      Tags::TimeStepId, Tags::Next<Tags::TimeStepId>, domain::Tags::Mesh<1>,
      domain::Tags::Element<1>, domain::Tags::ElementMap<1>, variables_tag,
      db::add_tag_prefix<Tags::dt, variables_tag>, normal_dot_fluxes_tag,
      mortar_data_tag, Tags::Mortars<Tags::Next<Tags::TimeStepId>, 1>,
      Tags::Mortars<domain::Tags::Mesh<0>, 1>,
      Tags::Mortars<Tags::MortarSize<0>, 1>>;

  using inverse_jacobian = domain::Tags::InverseJacobianCompute<
      domain::Tags::ElementMap<1>,
      domain::Tags::Coordinates<1, Frame::Logical>>;

  template <typename Tag>
  using interface_compute_tag =
      domain::Tags::InterfaceCompute<domain::Tags::InternalDirections<1>, Tag>;

  using compute_tags = db::AddComputeTags<
      domain::Tags::LogicalCoordinates<1>,
      domain::Tags::MappedCoordinates<
          domain::Tags::ElementMap<1>,
          domain::Tags::Coordinates<1, Frame::Logical>>,
      inverse_jacobian,
      Tags::DerivCompute<variables_tag, inverse_jacobian,
                         typename metavariables::system::gradients_tags>,
      domain::Tags::InternalDirections<1>,
      domain::Tags::Slice<domain::Tags::InternalDirections<1>,
                          typename metavariables::system::variables_tag>,
      interface_compute_tag<domain::Tags::Direction<1>>,
      interface_compute_tag<domain::Tags::InterfaceMesh<1>>,
      interface_compute_tag<domain::Tags::UnnormalizedFaceNormalCompute<1>>,
      interface_compute_tag<
          Tags::EuclideanMagnitude<domain::Tags::UnnormalizedFaceNormal<1>>>,
      interface_compute_tag<
          Tags::NormalizedCompute<domain::Tags::UnnormalizedFaceNormal<1>>>>;

  using phase_dependent_action_list = tmpl::list<
      Parallel::PhaseActions<
          typename Metavariables::Phase, Metavariables::Phase::Initialization,
          tmpl::list<
              ActionTesting::InitializeDataBox<simple_tags, compute_tags>>>,

      Parallel::PhaseActions<
          typename Metavariables::Phase, Metavariables::Phase::Testing,
          tmpl::list<dg::Actions::ComputeNonconservativeBoundaryFluxes<
                         domain::Tags::InternalDirections<1>>,
                     dg::Actions::SendDataForFluxes<Metavariables>,
                     Actions::ComputeTimeDerivative<ScalarWave::ComputeDuDt<1>>,
                     dg::Actions::ReceiveDataForFluxes<Metavariables>,
                     dg::Actions::ApplyFluxes>>>;
};

struct Metavariables {
  using system = ScalarWave::System<1>;
  using component_list = tmpl::list<Component<Metavariables>>;
  using temporal_id = Tags::TimeStepId;
  using normal_dot_numerical_flux =
      Tags::NumericalFlux<ScalarWave::UpwindFlux<1>>;
  enum class Phase { Initialization, Testing, Exit };
};

using system = Metavariables::system;
using EvolvedVariables = db::item_type<system::variables_tag>;

std::pair<tnsr::I<DataVector, 1>, EvolvedVariables> evaluate_rhs(
    const double time,
    const ScalarWave::Solutions::SemidiscretizedDg& solution) noexcept {
  using component = Component<Metavariables>;

  ActionTesting::MockRuntimeSystem<Metavariables> runner{
      {ScalarWave::UpwindFlux<1>{}}};

  const Slab slab(time, time + 1.0);
  const TimeStepId current_time(true, 0, slab.start());
  const TimeStepId next_time(true, 0, slab.end());
  const Mesh<1> mesh{2, Spectral::Basis::Legendre,
                     Spectral::Quadrature::GaussLobatto};

  PUPable_reg(
      SINGLE_ARG(domain::CoordinateMap<Frame::Logical, Frame::Inertial,
                                       domain::CoordinateMaps::Affine>));
  const auto block_map =
      domain::make_coordinate_map_base<Frame::Logical, Frame::Inertial>(
          domain::CoordinateMaps::Affine(-1.0, 1.0, 0.0, 2.0 * M_PI));

  const auto emplace_element =
      [&block_map, &current_time, &mesh, &next_time, &runner, &solution, &time](
          const ElementId<1>& id,
          const std::vector<std::pair<Direction<1>, ElementId<1>>>&
              mortars) noexcept {
        Element<1>::Neighbors_t neighbors;
        for (const auto& mortar_id : mortars) {
          neighbors.insert({mortar_id.first, {{mortar_id.second}, {}}});
        }
        const Element<1> element(id, std::move(neighbors));

        auto map = ElementMap<1, Frame::Inertial>(id, block_map->get_clone());

        db::item_type<system::variables_tag> variables(2);
        db::item_type<db::add_tag_prefix<Tags::dt, system::variables_tag>>
            dt_variables(2);

        db::item_type<component::normal_dot_fluxes_tag> normal_dot_fluxes;
        db::item_type<typename component::mortar_data_tag> mortar_history{};
        db::item_type<Tags::Mortars<Tags::Next<Tags::TimeStepId>, 1>>
            mortar_next_temporal_ids{};
        db::item_type<Tags::Mortars<domain::Tags::Mesh<0>, 1>> mortar_meshes{};
        db::item_type<Tags::Mortars<Tags::MortarSize<0>, 1>> mortar_sizes{};
        for (const auto& mortar_id : mortars) {
          normal_dot_fluxes[mortar_id.first].initialize(1, 0.0);
          mortar_history.insert({mortar_id, {}});
          mortar_next_temporal_ids.insert({mortar_id, current_time});
          mortar_meshes.insert({mortar_id, mesh.slice_away(0)});
          mortar_sizes.insert({mortar_id, {}});
        }

        ActionTesting::emplace_component_and_initialize<component>(
            &runner, id,
            {current_time, next_time, mesh, element, std::move(map),
             std::move(variables), std::move(dt_variables),
             std::move(normal_dot_fluxes), std::move(mortar_history),
             std::move(mortar_next_temporal_ids), std::move(mortar_meshes),
             std::move(mortar_sizes)});

        auto& box = ActionTesting::get_databox<
            component,
            tmpl::append<component::simple_tags, component::compute_tags>>(
            make_not_null(&runner), id);
        db::mutate<system::variables_tag>(
            make_not_null(&box),
            [&solution, &time](
                const gsl::not_null<EvolvedVariables*> vars,
                const db::const_item_type<domain::Tags::Coordinates<
                    1, Frame::Inertial>>& coords) noexcept {
              vars->assign_subset(solution.variables(
                  coords, time, system::variables_tag::tags_list{}));
            },
            db::get<domain::Tags::Coordinates<1, Frame::Inertial>>(box));
      };

  const ElementId<1> self_id(0, {{{4, 1}}});
  const ElementId<1> left_id(0, {{{4, 0}}});
  const ElementId<1> right_id(0, {{{4, 2}}});

  emplace_element(self_id, {{Direction<1>::lower_xi(), left_id},
                            {Direction<1>::upper_xi(), right_id}});
  emplace_element(left_id, {{Direction<1>::upper_xi(), self_id}});
  emplace_element(right_id, {{Direction<1>::lower_xi(), self_id}});
  ActionTesting::set_phase(make_not_null(&runner),
                           Metavariables::Phase::Testing);

  // The neighbors only have to get as far as sending data
  for (size_t i = 0; i < 2; ++i) {
    runner.next_action<component>(left_id);
    runner.next_action<component>(right_id);
  }

  for (size_t i = 0; i < 5; ++i) {
    runner.next_action<component>(self_id);
  }

  return {
      ActionTesting::get_databox_tag<
          component, domain::Tags::Coordinates<1, Frame::Inertial>>(runner,
                                                                    self_id),
      db::const_item_type<system::variables_tag>(
          ActionTesting::get_databox_tag<
              component, db::add_tag_prefix<Tags::dt, system::variables_tag>>(
              runner, self_id))};
}

void check_solution(
    const ScalarWave::Solutions::SemidiscretizedDg& solution) noexcept {
  const double time = 1.23;

  const auto coords_and_deriv_from_system = evaluate_rhs(time, solution);
  const auto& coords = coords_and_deriv_from_system.first;
  const auto& deriv_from_system = coords_and_deriv_from_system.second;
  const auto numerical_deriv = numerical_derivative(
      [&coords, &solution](const std::array<double, 1>& t) noexcept {
        EvolvedVariables result(2);
        result.assign_subset(solution.variables(
            coords, t[0], system::variables_tag::tags_list{}));
        return result;
      },
      std::array<double, 1>{{time}}, 0, 1e-3);

  CHECK_VARIABLES_CUSTOM_APPROX(numerical_deriv, deriv_from_system,
                                approx.epsilon(1.0e-12));
}
}  // namespace

SPECTRE_TEST_CASE(
    "Unit.PointwiseFunctions.AnalyticSolutions.WaveEquation.SemidiscretizedDg",
    "[Unit][PointwiseFunctions]") {
  // We use 16 elements, so there are 16 independent harmonics.
  for (int harmonic = 0; harmonic < 16; ++harmonic) {
    check_solution({harmonic, {{1.2, 2.3, 3.4, 4.5}}});
  }

  check_solution(
      TestHelpers::test_creation<ScalarWave::Solutions::SemidiscretizedDg>(
          "Harmonic: 1\n"
          "Amplitudes: [1.2, 2.3, 3.4, 4.5]"));
}
