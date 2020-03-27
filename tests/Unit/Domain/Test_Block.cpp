// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <pup.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "DataStructures/Tensor/Tensor.hpp"
#include "Domain/Block.hpp"
#include "Domain/BlockNeighbor.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.tpp"
#include "Domain/CoordinateMaps/Identity.hpp"
#include "Domain/CoordinateMaps/ProductMapsTimeDep.hpp"
#include "Domain/CoordinateMaps/ProductMapsTimeDep.tpp"
#include "Domain/CoordinateMaps/Translation.hpp"
#include "Domain/Direction.hpp"
#include "Domain/DirectionMap.hpp"
#include "Domain/FunctionsOfTime/PiecewisePolynomial.hpp"
#include "Domain/OrientationMap.hpp"
#include "Framework/TestHelpers.hpp"
#include "Utilities/GetOutput.hpp"

namespace domain {
namespace {
template <size_t Dim>
void test_block_time_independent() {
  PUPable_reg(SINGLE_ARG(CoordinateMap<Frame::Logical, Frame::Inertial,
                                       CoordinateMaps::Identity<Dim>>));

  using coordinate_map = CoordinateMap<Frame::Logical, Frame::Inertial,
                                       CoordinateMaps::Identity<Dim>>;
  const coordinate_map identity_map{CoordinateMaps::Identity<Dim>{}};
  Block<Dim> original_block(identity_map.get_clone(), 7, {});
  CHECK_FALSE(original_block.is_time_dependent());

  const auto check_block = [](const Block<Dim>& block) {
    // Test external boundaries:
    CHECK((block.external_boundaries().size()) == 2 * Dim);

    // Test neighbors:
    CHECK((block.neighbors().size()) == 0);

    // Test id:
    CHECK((block.id()) == 7);

    // Test that the block's coordinate_map is Identity:
    const auto& map = block.stationary_map();
    const tnsr::I<double, Dim, Frame::Logical> xi(1.0);
    const tnsr::I<double, Dim, Frame::Inertial> x(1.0);
    CHECK(map(xi) == x);
    CHECK(map.inverse(x).get() == xi);
  };

  check_block(original_block);
  check_block(serialize_and_deserialize(original_block));

  // Test PUP
  test_serialization(original_block);

  // Test move semantics:
  const Block<Dim> block_copy(identity_map.get_clone(), 7, {});
  test_move_semantics(std::move(original_block), block_copy);
}

using Translation = domain::CoordMapsTimeDependent::Translation;
template <size_t Dim>
using TranslationDimD = tmpl::conditional_t<
    Dim == 2,
    domain::CoordMapsTimeDependent::ProductOf2Maps<Translation, Translation>,
    domain::CoordMapsTimeDependent::ProductOf3Maps<Translation, Translation,
                                                   Translation>>;

template <size_t VolumeDim>
auto make_translation_map() noexcept;

template <>
auto make_translation_map<1>() noexcept {
  return domain::make_coordinate_map<Frame::Grid, Frame::Inertial>(
      Translation{"Translation"});
}

template <>
auto make_translation_map<2>() noexcept {
  return domain::make_coordinate_map<Frame::Grid, Frame::Inertial>(
      TranslationDimD<2>{Translation{"Translation"},
                         Translation{"Translation"}});
}

template <>
auto make_translation_map<3>() noexcept {
  return domain::make_coordinate_map<Frame::Grid, Frame::Inertial>(
      TranslationDimD<3>{Translation{"Translation"}, Translation{"Translation"},
                         Translation{"Translation"}});
}

template <size_t Dim>
void test_block_time_dependent() {
  using TranslationDimD = tmpl::conditional_t<
      Dim == 1,
      domain::CoordinateMap<Frame::Grid, Frame::Inertial, Translation>,
      tmpl::conditional_t<
          Dim == 2,
          domain::CoordinateMap<Frame::Grid, Frame::Inertial,
                                domain::CoordMapsTimeDependent::ProductOf2Maps<
                                    Translation, Translation>>,
          domain::CoordinateMap<Frame::Grid, Frame::Inertial,
                                domain::CoordMapsTimeDependent::ProductOf3Maps<
                                    Translation, Translation, Translation>>>>;
  using logical_to_grid_coordinate_map =
      CoordinateMap<Frame::Logical, Frame::Inertial,
                    CoordinateMaps::Identity<Dim>>;
  using grid_to_inertial_coordinate_map = TranslationDimD;
  PUPable_reg(logical_to_grid_coordinate_map);
  PUPable_reg(SINGLE_ARG(CoordinateMap<Frame::Logical, Frame::Grid,
                                       CoordinateMaps::Identity<Dim>>));
  PUPable_reg(grid_to_inertial_coordinate_map);
  const logical_to_grid_coordinate_map identity_map{
      CoordinateMaps::Identity<Dim>{}};
  const grid_to_inertial_coordinate_map translation_map =
      make_translation_map<Dim>();
  Block<Dim> original_block(identity_map.get_clone(), 7, {});
  CHECK_FALSE(original_block.is_time_dependent());
  original_block.inject_time_dependent_map(translation_map.get_clone());
  CHECK(original_block.is_time_dependent());

  const auto check_block = [](const Block<Dim>& block) {
    const double time = 2.0;

    std::unordered_map<std::string,
                       std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>>
        functions_of_time{};

    functions_of_time["Translation"] =
        std::make_unique<FunctionsOfTime::PiecewisePolynomial<2>>(
            0.0, std::array<DataVector, 3>{{{0.0}, {1.0}, {0.0}}});

    // Test external boundaries:
    CHECK((block.external_boundaries().size()) == 2 * Dim);

    // Test neighbors:
    CHECK((block.neighbors().size()) == 0);

    // Test id:
    CHECK((block.id()) == 7);

    // Test that the block's coordinate_map is Identity:
    const auto& grid_to_inertial_map = block.moving_mesh_grid_to_inertial_map();
    const auto& logical_to_grid_map = block.moving_mesh_logical_to_grid_map();
    const tnsr::I<double, Dim, Frame::Logical> xi(1.0);
    const tnsr::I<double, Dim, Frame::Inertial> x(1.0 + time);
    CHECK(grid_to_inertial_map(logical_to_grid_map(xi), time,
                               functions_of_time) == x);
    CHECK(
        logical_to_grid_map
            .inverse(
                grid_to_inertial_map.inverse(x, time, functions_of_time).get())
            .get() == xi);
  };

  check_block(original_block);
  check_block(serialize_and_deserialize(original_block));

  // Test PUP
  test_serialization(original_block);

  // Test move semantics:
  Block<Dim> block_copy(identity_map.get_clone(), 7, {});
  block_copy.inject_time_dependent_map(translation_map.get_clone());
  test_move_semantics(std::move(original_block), block_copy);
}
}  // namespace

SPECTRE_TEST_CASE("Unit.Domain.Block", "[Domain][Unit]") {
  test_block_time_independent<1>();
  test_block_time_independent<2>();
  test_block_time_independent<3>();

  test_block_time_dependent<1>();
  test_block_time_dependent<2>();
  test_block_time_dependent<3>();

  // Create DirectionMap<VolumeDim, BlockNeighbor<VolumeDim>>

  // Each BlockNeighbor is an id and an OrientationMap:
  const BlockNeighbor<2> block_neighbor1(
      1, OrientationMap<2>(std::array<Direction<2>, 2>{
             {Direction<2>::upper_xi(), Direction<2>::upper_eta()}}));
  const BlockNeighbor<2> block_neighbor2(
      2, OrientationMap<2>(std::array<Direction<2>, 2>{
             {Direction<2>::lower_xi(), Direction<2>::upper_eta()}}));
  DirectionMap<2, BlockNeighbor<2>> neighbors = {
      {Direction<2>::upper_xi(), block_neighbor1},
      {Direction<2>::lower_eta(), block_neighbor2}};
  using coordinate_map = CoordinateMap<Frame::Logical, Frame::Inertial,
                                       CoordinateMaps::Identity<2>>;
  const coordinate_map identity_map{CoordinateMaps::Identity<2>{}};
  const Block<2> block(identity_map.get_clone(), 3, std::move(neighbors));

  // Test external boundaries:
  CHECK((block.external_boundaries().size()) == 2);

  // Test neighbors:
  CHECK((block.neighbors().size()) == 2);

  // Test id:
  CHECK((block.id()) == 3);

  // Test output:
  CHECK(get_output(block) ==
        "Block 3:\n"
        "Neighbors: "
        "([+0,Id = 1; orientation = (+0, +1)],"
        "[-1,Id = 2; orientation = (-0, +1)])\n"
        "External boundaries: (+1,-0)\n"
        "Is time dependent: false");

  // Test comparison:
  const Block<2> neighborless_block(identity_map.get_clone(), 7, {});
  CHECK(block == block);
  CHECK(block != neighborless_block);
  CHECK(neighborless_block == neighborless_block);
}
}  // namespace domain
