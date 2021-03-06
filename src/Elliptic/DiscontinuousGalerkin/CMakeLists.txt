# Distributed under the MIT License.
# See LICENSE.txt for details.

set(LIBRARY EllipticDg)

add_spectre_library(${LIBRARY})

spectre_target_sources(
  ${LIBRARY}
  PRIVATE
  Penalty.cpp
)

spectre_target_headers(
  ${LIBRARY}
  INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/src
  HEADERS
  DgElementArray.hpp
  DiscontinuousGalerkin.hpp
  ImposeBoundaryConditions.hpp
  ImposeInhomogeneousBoundaryConditionsOnSource.hpp
  InitializeFirstOrderOperator.hpp
  Penalty.hpp
  Tags.hpp
  )

target_link_libraries(
  ${LIBRARY}
  PUBLIC
  DataStructures
  INTERFACE
  DiscontinuousGalerkin
  Domain
  DomainCreators
  DomainStructure
  Elliptic
  Initialization
  LinearOperators
  Options
  Parallel
  Spectral
  SystemUtilities
  Utilities
  )

add_subdirectory(NumericalFluxes)
