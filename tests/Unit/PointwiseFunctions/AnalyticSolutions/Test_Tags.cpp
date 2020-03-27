// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <string>

#include "DataStructures/DataBox/Tag.hpp"
#include "Helpers/DataStructures/DataBox/TestHelpers.hpp"
#include "PointwiseFunctions/AnalyticSolutions/Tags.hpp"

namespace {
struct DummyType {};
struct DummyTag : db::SimpleTag {
  using type = DummyType;
};
}  // namespace

SPECTRE_TEST_CASE("Unit.AnalyticSolutions.Tags", "[Unit][PointwiseFunctions]") {
  TestHelpers::db::test_base_tag<Tags::AnalyticSolutionBase>(
      "AnalyticSolutionBase");
  TestHelpers::db::test_simple_tag<Tags::AnalyticSolution<DummyType>>(
      "AnalyticSolution");
  TestHelpers::db::test_base_tag<Tags::BoundaryConditionBase>(
      "BoundaryConditionBase");
  TestHelpers::db::test_simple_tag<Tags::BoundaryCondition<DummyType>>(
      "BoundaryCondition");
}
