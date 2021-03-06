# Distributed under the MIT License.
# See LICENSE.txt for details.

set(LIBRARY "Test_ParallelSchwarz")

set(LIBRARY_SOURCES
  Test_ComputeTags.cpp
  Test_ElementCenteredSubdomainData.cpp
  Test_OverlapHelpers.cpp
  Test_Tags.cpp
  Test_Weighting.cpp
  )

add_test_library(
  ${LIBRARY}
  "ParallelAlgorithms/LinearSolver/Schwarz"
  "${LIBRARY_SOURCES}"
  "DataStructures;Domain;DomainStructure;LinearSolver;ParallelSchwarz;Spectral"
  )

add_distributed_linear_solver_algorithm_test("SchwarzAlgorithm")
target_link_libraries(
  Test_SchwarzAlgorithm
  PRIVATE
  Domain
  DomainStructure
  ParallelSchwarz
)

add_subdirectory(Actions)
