// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <array>
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "DataStructures/Tensor/Tensor.hpp"
#include "Domain/CoordinateMaps/Affine.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.tpp"
#include "Domain/CoordinateMaps/ProductMaps.hpp"
#include "Domain/CoordinateMaps/ProductMaps.tpp"
#include "Domain/Creators/AlignedLattice.hpp"
#include "Domain/Creators/DomainCreator.hpp"
#include "Domain/Domain.hpp"
#include "Framework/TestCreation.hpp"
#include "Framework/TestHelpers.hpp"
#include "Helpers/Domain/DomainTestHelpers.hpp"
#include "Parallel/RegisterDerivedClassesWithCharm.hpp"

namespace Frame {
struct Inertial;
}  // namespace Frame

namespace domain {
namespace {
template <size_t VolumeDim>
void test_aligned_blocks(
    const creators::AlignedLattice<VolumeDim>& aligned_blocks) noexcept {
  const auto domain = aligned_blocks.create_domain();
  test_initial_domain(domain, aligned_blocks.initial_refinement_levels());

  Parallel::register_classes_in_list<
      typename creators::AlignedLattice<VolumeDim>::maps_list>();
  test_serialization(domain);
}
}  // namespace

SPECTRE_TEST_CASE("Unit.Domain.Creators.AlignedLattice", "[Domain][Unit]") {
  const auto domain_creator_1d =
      TestHelpers::test_factory_creation<DomainCreator<1>>(
          "AlignedLattice:\n"
          "  BlockBounds: [[0.1, 2.6, 5.1, 5.2, 7.2]]\n"
          "  IsPeriodicIn: [false]\n"
          "  InitialGridPoints: [3]\n"
          "  InitialRefinement: [2]\n");
  const auto* aligned_blocks_creator_1d =
      dynamic_cast<const creators::AlignedLattice<1>*>(domain_creator_1d.get());
  test_aligned_blocks(*aligned_blocks_creator_1d);

  const auto domain_creator_2d =
      TestHelpers::test_factory_creation<DomainCreator<2>>(
          "AlignedLattice:\n"
          "  BlockBounds: [[0.1, 2.6, 5.1], [-0.4, 3.2, 6.2, 8.9]]\n"
          "  IsPeriodicIn: [false, true]\n"
          "  InitialGridPoints: [3, 4]\n"
          "  InitialRefinement: [2, 1]\n");
  const auto* aligned_blocks_creator_2d =
      dynamic_cast<const creators::AlignedLattice<2>*>(domain_creator_2d.get());
  test_aligned_blocks(*aligned_blocks_creator_2d);

  const auto domain_creator_3d =
      TestHelpers::test_factory_creation<DomainCreator<3>>(
          "AlignedLattice:\n"
          "  BlockBounds: [[0.1, 2.6, 5.1], [-0.4, 3.2, 6.2], [-0.2, 3.2]]\n"
          "  IsPeriodicIn: [false, true, false]\n"
          "  InitialGridPoints: [3, 4, 5]\n"
          "  InitialRefinement: [2, 1, 0]\n");
  const auto* aligned_blocks_creator_3d =
      dynamic_cast<const creators::AlignedLattice<3>*>(domain_creator_3d.get());
  test_aligned_blocks(*aligned_blocks_creator_3d);

  const auto cubical_shell_domain =
      TestHelpers::test_factory_creation<DomainCreator<3>>(
          "AlignedLattice:\n"
          "  BlockBounds: [[0.1, 2.6, 5.1, 6.0], [-0.4, 3.2, 6.2, 7.0], "
          "[-0.2, 3.2, 4.0, 5.2]]\n"
          "  IsPeriodicIn: [false, false, false]\n"
          "  InitialGridPoints: [3, 4, 5]\n"
          "  InitialRefinement: [2, 1, 0]\n"
          "  BlocksToExclude: [[1, 1, 1]]");
  const auto* cubical_shell_creator_3d =
      dynamic_cast<const creators::AlignedLattice<3>*>(
          cubical_shell_domain.get());
  test_aligned_blocks(*cubical_shell_creator_3d);

  const auto unit_cubical_shell_domain =
      TestHelpers::test_factory_creation<DomainCreator<3>>(
          "AlignedLattice:\n"
          "  BlockBounds: [[-1.5, -0.5, 0.5, 1.5], [-1.5, -0.5, 0.5, 1.5], "
          "[-1.5, -0.5, 0.5, 1.5]]\n"
          "  IsPeriodicIn: [false, false, false]\n"
          "  InitialGridPoints: [5, 5, 5]\n"
          "  InitialRefinement: [1, 1, 1]\n"
          "  BlocksToExclude: [[1, 1, 1]]");
  const auto* unit_cubical_shell_creator_3d =
      dynamic_cast<const creators::AlignedLattice<3>*>(
          unit_cubical_shell_domain.get());
  test_aligned_blocks(*unit_cubical_shell_creator_3d);

  {
    // Expected domain refinement:
    // 23 23 67
    // 23 45 67
    // 23 XX 45
    const auto refined_domain =
        TestHelpers::test_factory_creation<DomainCreator<2>>(
            "AlignedLattice:\n"
            "  BlockBounds: [[70, 71, 72, 73], [90, 91, 92, 93]]\n"
            "  InitialGridPoints: [2, 3]\n"
            "  InitialRefinement: [0, 0]\n"
            "  BlocksToExclude: [[1, 0]]\n"
            "  RefinedGridPoints:\n"
            "  - LowerCornerIndex: [1, 0]\n"
            "    UpperCornerIndex: [3, 2]\n"
            "    Refinement: [4, 5]\n"
            "  - LowerCornerIndex: [2, 1]\n"
            "    UpperCornerIndex: [3, 3]\n"
            "    Refinement: [6, 7]");
    std::unordered_set<
        std::pair<std::vector<double>, std::array<size_t, 2>>,
        boost::hash<std::pair<std::vector<double>, std::array<size_t, 2>>>>
        expected_blocks{{{70.0, 90.0}, {{2, 3}}}, {{72.0, 90.0}, {{4, 5}}},
                        {{70.0, 91.0}, {{2, 3}}}, {{71.0, 91.0}, {{4, 5}}},
                        {{72.0, 91.0}, {{6, 7}}}, {{70.0, 92.0}, {{2, 3}}},
                        {{71.0, 92.0}, {{2, 3}}}, {{72.0, 92.0}, {{6, 7}}}};
    const auto domain = refined_domain->create_domain();
    const auto& blocks = domain.blocks();
    const auto extents = refined_domain->initial_extents();
    REQUIRE(blocks.size() == extents.size());
    for (size_t i = 0; i < blocks.size(); ++i) {
      const auto location =
          blocks[i]
              .stationary_map()(
                  tnsr::I<double, 2, Frame::Logical>{{{-1.0, -1.0}}})
              .get_vector_of_data()
              .second;
      INFO("Unexpected block");
      CAPTURE(location);
      CAPTURE(extents[i]);
      CHECK(expected_blocks.erase({location, extents[i]}) == 1);
    }
    CAPTURE_PRECISE(expected_blocks);
    CHECK(expected_blocks.empty());
  }
}

// [[OutputRegex, Cannot exclude blocks as well as have periodic boundary
// conditions!]]
SPECTRE_TEST_CASE("Unit.Domain.Creators.AlignedLattice.Error",
                  "[Unit][ErrorHandling]") {
  ERROR_TEST();
  const auto failed_cubical_shell_domain =
      TestHelpers::test_factory_creation<DomainCreator<3>>(
          "AlignedLattice:\n"
          "  BlockBounds: [[-1.5, -0.5, 0.5, 1.5], [-1.5, -0.5, 0.5, 1.5], "
          "[-1.5, -0.5, 0.5, 1.5]]\n"
          "  IsPeriodicIn: [true, false, false]\n"
          "  InitialGridPoints: [5, 5, 5]\n"
          "  InitialRefinement: [1, 1, 1]\n"
          "  BlocksToExclude: [[1, 1, 1]]");
}
}  // namespace domain
