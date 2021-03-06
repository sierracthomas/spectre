# Distributed under the MIT License.
# See LICENSE.txt for details.

set(LIBRARY "Test_LinearOperators")

set(LIBRARY_SOURCES
  Test_CoefficientTransforms.cpp
  Test_DefiniteIntegral.cpp
  Test_Divergence.cpp
  Test_Filtering.cpp
  Test_IndefiniteIntegral.cpp
  Test_Linearize.cpp
  Test_MeanValue.cpp
  Test_PartialDerivatives.cpp
  Test_WeakDivergence.cpp
  )

add_test_library(
  ${LIBRARY}
  "NumericalAlgorithms/LinearOperators/"
  "${LIBRARY_SOURCES}"
  "DiscontinuousGalerkin;ErrorHandling;LinearOperators;MathFunctions;Spectral;Utilities"
  )

add_dependencies(
  ${LIBRARY}
  module_GlobalCache
  )
