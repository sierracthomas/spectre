// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>

#include "DataStructures/VariablesTag.hpp"
#include "Evolution/Systems/CurvedScalarWave/Equations.hpp"
#include "Evolution/Systems/CurvedScalarWave/Tags.hpp"
#include "Utilities/TMPL.hpp"

/*!
 * \ingroup EvolutionSystemsGroup
 * \brief Items related to evolving a scalar wave on a curved background
 */
namespace CurvedScalarWave {

template <size_t Dim>
struct System {
  static constexpr bool is_in_flux_conservative_form = false;
  static constexpr bool has_primitive_and_conservative_vars = false;
  static constexpr size_t volume_dim = Dim;

  using variables_tag = ::Tags::Variables<tmpl::list<Pi, Phi<Dim>, Psi>>;
  using gradients_tags = tmpl::list<Pi, Phi<Dim>, Psi>;
};
}  // namespace CurvedScalarWave
