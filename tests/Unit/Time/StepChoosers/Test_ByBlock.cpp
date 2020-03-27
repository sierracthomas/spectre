// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <cstddef>
#include <memory>

#include "DataStructures/DataBox/DataBox.hpp"
#include "Domain/Element.hpp"
#include "Domain/ElementId.hpp"
#include "Domain/Tags.hpp"
#include "Framework/TestCreation.hpp"
#include "Framework/TestHelpers.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Parallel/RegisterDerivedClassesWithCharm.hpp"
#include "Time/StepChoosers/ByBlock.hpp"
#include "Time/StepChoosers/StepChooser.hpp"
#include "Time/Time.hpp"
#include "Utilities/TMPL.hpp"

// IWYU pragma: no_include <pup.h>

// IWYU pragma: no_include "Parallel/PupStlCpp11.hpp"

namespace {
struct Metavariables {
  using component_list = tmpl::list<>;
};
}  // namespace

SPECTRE_TEST_CASE("Unit.Time.StepChoosers.ByBlock", "[Unit][Time]") {
  constexpr size_t volume_dim = 2;

  using StepChooserType =
      StepChooser<tmpl::list<StepChoosers::Registrars::ByBlock<volume_dim>>>;
  using ByBlock = StepChoosers::ByBlock<volume_dim>;

  Parallel::register_derived_classes_with_charm<StepChooserType>();

  const ByBlock by_block({2.5, 3.0, 3.5});
  const std::unique_ptr<StepChooserType> by_block_base =
      std::make_unique<ByBlock>(by_block);

  const double current_step = std::numeric_limits<double>::infinity();
  const Parallel::ConstGlobalCache<Metavariables> cache{{}};
  for (size_t block = 0; block < 3; ++block) {
    const Element<volume_dim> element(ElementId<volume_dim>(block), {});
    const auto box =
        db::create<db::AddSimpleTags<domain::Tags::Element<volume_dim>>>(
            element);
    const double expected = 0.5 * (block + 5);

    CHECK(by_block(element, current_step, cache) == expected);
    CHECK(by_block_base->desired_step(current_step, box, cache) == expected);
    CHECK(serialize_and_deserialize(by_block)(element, current_step, cache) ==
          expected);
    CHECK(serialize_and_deserialize(by_block_base)
              ->desired_step(current_step, box, cache) == expected);
  }

  TestHelpers::test_factory_creation<StepChooserType>(
      "ByBlock:\n"
      "  Sizes: [1.0, 2.0]");
}
