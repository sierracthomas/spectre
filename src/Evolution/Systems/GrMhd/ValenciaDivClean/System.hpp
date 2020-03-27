// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>

#include "DataStructures/Tensor/EagerMath/Magnitude.hpp"
#include "DataStructures/VariablesTag.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/Characteristics.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/ConservativeFromPrimitive.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/Fluxes.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/NewmanHamlin.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/PrimitiveFromConservative.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/Sources.hpp"
#include "Evolution/Systems/GrMhd/ValenciaDivClean/Tags.hpp"
#include "PointwiseFunctions/GeneralRelativity/Tags.hpp"
#include "PointwiseFunctions/Hydro/Tags.hpp"
#include "Utilities/TMPL.hpp"

/// \ingroup EvolutionSystemsGroup
/// \brief Items related to general relativistic magnetohydrodynamics (GRMHD)
namespace grmhd {
/// The Valencia formulation of ideal GRMHD with divergence cleaning.
///
/// References:
/// - [Numerical 3+1 General Relativistic Magnetohydrodynamics: A Local
/// Characteristic Approach](http://iopscience.iop.org/article/10.1086/498238)
/// - [GRHydro: a new open-source general-relativistic magnetohydrodynamics code
/// for the Einstein toolkit]
/// (http://iopscience.iop.org/article/10.1088/0264-9381/31/1/015005)
namespace ValenciaDivClean {

template <typename EquationOfStateType>
struct System {
  static constexpr bool is_in_flux_conservative_form = true;
  static constexpr bool has_primitive_and_conservative_vars = true;
  static constexpr size_t volume_dim = 3;
  static constexpr size_t thermodynamic_dim =
      EquationOfStateType::thermodynamic_dim;

  using primitive_variables_tag =
      ::Tags::Variables<hydro::grmhd_tags<DataVector>>;

  using variables_tag =
      ::Tags::Variables<tmpl::list<Tags::TildeD, Tags::TildeTau, Tags::TildeS<>,
                                   Tags::TildeB<>, Tags::TildePhi>>;

  using spacetime_variables_tag =
      ::Tags::Variables<gr::tags_for_hydro<volume_dim, DataVector>>;

  template <typename Tag>
  using magnitude_tag = ::Tags::NonEuclideanMagnitude<
      Tag, gr::Tags::InverseSpatialMetric<3, Frame::Inertial, DataVector>>;

  using char_speeds_tag =
      Tags::CharacteristicSpeedsCompute<EquationOfStateType>;
  using compute_largest_characteristic_speed =
      ComputeLargestCharacteristicSpeed;

  using conservative_from_primitive = ConservativeFromPrimitive;

  template <typename OrderedListOfPrimitiveRecoverySchemes>
  using primitive_from_conservative =
      PrimitiveFromConservative<OrderedListOfPrimitiveRecoverySchemes,
                                EquationOfStateType::thermodynamic_dim>;

  using volume_fluxes = ComputeFluxes;

  using volume_sources = ComputeSources;

  // skip TildeD as its source is zero.
  using sourced_variables = tmpl::list<Tags::TildeTau, Tags::TildeS<>,
                                       Tags::TildeB<>, Tags::TildePhi>;
};
}  // namespace ValenciaDivClean
}  // namespace grmhd
