// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <string>

#include "Helpers/DataStructures/DataBox/TestHelpers.hpp"
#include "ParallelAlgorithms/EventsAndTriggers/Tags.hpp"

namespace {
struct DummyType {};
}  // namespace

SPECTRE_TEST_CASE("Unit.ParallelAlgorithms.EventsAndTriggers.Tags",
                  "[Unit][ParallelAlgorithms]") {
  TestHelpers::db::test_base_tag<Tags::EventsAndTriggersBase>(
      "EventsAndTriggersBase");
  TestHelpers::db::test_simple_tag<
      Tags::EventsAndTriggers<DummyType, DummyType>>("EventsAndTriggers");
}
