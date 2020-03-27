// Distributed under the MIT License.
// See LICENSE.txt for details.

#include <pybind11/pybind11.h>
#include <string>

#include "PointwiseFunctions/Hydro/EquationsOfState/PolytropicFluid.hpp"

namespace py = pybind11;

namespace EquationsOfState {
namespace py_bindings {

void bind_polytropic_fluid(py::module& m) {  // NOLINT
  // We can't expose any member functions without wrapping tensors in Python,
  // so we only expose the initializer for now.
  py::class_<PolytropicFluid<true>, EquationOfState<true, 1>>(
      m, "RelativisticPolytropicFluid")
      .def(py::init<double, double>(), py::arg("polytropic_constant"),
           py::arg("polytropic_exponent"));
}

}  // namespace py_bindings
}  // namespace EquationsOfState
