// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <string>

#include "DataStructures/DataBox/Tag.hpp"
#include "Helpers/DataStructures/DataBox/TestHelpers.hpp"
#include "Time/Actions/SelfStartActions.hpp"
#include "Time/Tags.hpp"

namespace {
struct DummyType {};
struct DummyTag : db::SimpleTag {
  using type = DummyType;
};
}  // namespace

SPECTRE_TEST_CASE("Unit.Time.Tags", "[Unit][Time]") {
  TestHelpers::db::test_base_tag<Tags::HistoryEvolvedVariables<>>(
      "HistoryEvolvedVariables");
  TestHelpers::db::test_base_tag<Tags::TimeStepper<>>("TimeStepper");
  TestHelpers::db::test_compute_tag<Tags::SubstepTimeCompute>("SubstepTime");
  TestHelpers::db::test_prefix_tag<SelfStart::Tags::InitialValue<DummyTag>>(
      "InitialValue(DummyTag)");
  TestHelpers::db::test_simple_tag<Tags::TimeStepId>("TimeStepId");
  TestHelpers::db::test_simple_tag<Tags::TimeStep>("TimeStep");
  TestHelpers::db::test_simple_tag<Tags::SubstepTime>("SubstepTime");
  TestHelpers::db::test_simple_tag<Tags::Time>("Time");
  TestHelpers::db::test_simple_tag<Tags::HistoryEvolvedVariables<DummyType>>(
      "HistoryEvolvedVariables");
  TestHelpers::db::test_simple_tag<
      Tags::BoundaryHistory<DummyType, DummyType, DummyType>>(
      "BoundaryHistory");
  TestHelpers::db::test_simple_tag<Tags::TimeStepper<DummyType>>("TimeStepper");
  TestHelpers::db::test_simple_tag<Tags::StepChoosers<DummyType>>(
      "StepChoosers");
  TestHelpers::db::test_simple_tag<Tags::StepController>("StepController");
}
