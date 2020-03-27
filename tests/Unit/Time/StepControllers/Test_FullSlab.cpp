// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <limits>
#include <memory>

#include "Framework/TestCreation.hpp"
#include "Framework/TestHelpers.hpp"
#include "Parallel/RegisterDerivedClassesWithCharm.hpp"
#include "Time/Slab.hpp"
#include "Time/StepControllers/FullSlab.hpp"
#include "Time/StepControllers/StepController.hpp"
#include "Time/Time.hpp"

// IWYU pragma: no_include "Parallel/PupStlCpp11.hpp"

SPECTRE_TEST_CASE("Unit.Time.StepControllers.FullSlab", "[Unit][Time]") {
  Parallel::register_derived_classes_with_charm<StepController>();
  const auto check = [](const auto& fs) noexcept {
    const Slab slab(1., 4.);
    CHECK(fs.choose_step(slab.start(), 4.) == slab.duration());
    CHECK(fs.choose_step(slab.start(), 10.) == slab.duration());
    CHECK(fs.choose_step(slab.start(), 2.) == slab.duration());
    CHECK(fs.choose_step(slab.start(), 1.4) == slab.duration());

    CHECK(fs.choose_step(slab.end(), -2.) == -slab.duration());

    CHECK(
        fs.choose_step(slab.start(), std::numeric_limits<double>::infinity()) ==
        slab.duration());
  };
  check(StepControllers::FullSlab{});
  check(*serialize_and_deserialize(
      TestHelpers::test_factory_creation<StepController>("FullSlab")));
}
