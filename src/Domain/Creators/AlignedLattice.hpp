// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <limits>
#include <vector>

#include "DataStructures/Index.hpp"
#include "Domain/Creators/DomainCreator.hpp"  // IWYU pragma: keep
#include "Domain/Domain.hpp"
#include "Options/Options.hpp"
#include "Utilities/MakeArray.hpp"
#include "Utilities/TMPL.hpp"

/// \cond
namespace domain {
namespace CoordinateMaps {
class Affine;
template <typename Map1, typename Map2>
class ProductOf2Maps;
template <typename Map1, typename Map2, typename Map3>
class ProductOf3Maps;
}  // namespace CoordinateMaps

template <typename SourceFrame, typename TargetFrame, typename... Maps>
class CoordinateMap;
}  // namespace domain
/// \endcond

namespace domain {
namespace creators {

template <size_t VolumeDim>
struct RefinementRegion {
  std::array<size_t, VolumeDim> lower_corner_index;
  std::array<size_t, VolumeDim> upper_corner_index;
  std::array<size_t, VolumeDim> refinement;

  struct LowerCornerIndex {
    using type = std::array<size_t, VolumeDim>;
    static constexpr OptionString help = {"Lower bound of refined region."};
  };

  struct UpperCornerIndex {
    using type = std::array<size_t, VolumeDim>;
    static constexpr OptionString help = {"Upper bound of refined region."};
  };

  struct Refinement {
    using type = std::array<size_t, VolumeDim>;
    static constexpr OptionString help = {"Refinement inside region."};
  };

  static constexpr OptionString help = {
      "A region to be refined differently from the default for the lattice.\n"
      "The region is a box between the block boundaries indexed by the bound\n"
      "options."};
  using options = tmpl::list<LowerCornerIndex, UpperCornerIndex, Refinement>;
  RefinementRegion(const std::array<size_t, VolumeDim>& lower_corner_index_in,
                   const std::array<size_t, VolumeDim>& upper_corner_index_in,
                   const std::array<size_t, VolumeDim>& refinement_in) noexcept
      : lower_corner_index(lower_corner_index_in),
        upper_corner_index(upper_corner_index_in),
        refinement(refinement_in) {}
  RefinementRegion() = default;
};

/// \cond
// This is needed to print the default value for the RefinedGridPoints
// option.  Since the default value is an empty vector, this function
// is never actually called.
template <size_t VolumeDim>
[[noreturn]] std::ostream& operator<<(
    std::ostream& /*s*/,
    const RefinementRegion<VolumeDim>& /*unused*/) noexcept;
/// \endcond

/// \brief Create a Domain consisting of multiple aligned Blocks arrayed in a
/// lattice.
///
/// This is useful for setting up problems with piecewise smooth initial data,
/// problems that specify different boundary conditions on distinct parts of
/// the boundary, or problems that need different length scales initially.
///
/// \note Adaptive mesh refinement can never join Block%s, so use the fewest
/// number of Block%s that your problem needs.  More initial Element%s can be
/// created by specifying a larger `InitialRefinement`.
template <size_t VolumeDim>
class AlignedLattice : public DomainCreator<VolumeDim> {
 public:
  using maps_list = tmpl::list<
      domain::CoordinateMap<Frame::Logical, Frame::Inertial,
                            CoordinateMaps::Affine>,
      domain::CoordinateMap<
          Frame::Logical, Frame::Inertial,
          CoordinateMaps::ProductOf2Maps<CoordinateMaps::Affine,
                                         CoordinateMaps::Affine>>,
      domain::CoordinateMap<Frame::Logical, Frame::Inertial,
                            CoordinateMaps::ProductOf3Maps<
                                CoordinateMaps::Affine, CoordinateMaps::Affine,
                                CoordinateMaps::Affine>>>;

  struct BlockBounds {
    using type = std::array<std::vector<double>, VolumeDim>;
    static constexpr OptionString help = {
        "Coordinates of block boundaries in each dimension."};
  };

  struct IsPeriodicIn {
    using type = std::array<bool, VolumeDim>;
    static constexpr OptionString help = {
        "Whether the domain is periodic in each dimension."};
    static type default_value() noexcept {
      return make_array<VolumeDim>(false);
    }
  };

  struct InitialRefinement {
    using type = std::array<size_t, VolumeDim>;
    static constexpr OptionString help = {
        "Initial refinement level in each dimension."};
  };

  struct InitialGridPoints {
    using type = std::array<size_t, VolumeDim>;
    static constexpr OptionString help = {
        "Initial number of grid points in each dimension."};
  };

  struct RefinedGridPoints {
    using type = std::vector<RefinementRegion<VolumeDim>>;
    static constexpr OptionString help = {
        "Refined regions.  Later entries take priority."};
    static type default_value() noexcept { return {}; }
  };

  struct BlocksToExclude {
    using type = std::vector<std::array<size_t, VolumeDim>>;
    static constexpr OptionString help = {
        "List of Block indices to exclude, if any."};
    static type default_value() noexcept {
      return std::vector<std::array<size_t, VolumeDim>>{};
    }
  };

  using options =
      tmpl::list<BlockBounds, IsPeriodicIn, InitialRefinement,
                 InitialGridPoints, RefinedGridPoints, BlocksToExclude>;

  static constexpr OptionString help = {
      "AlignedLattice creates a regular lattice of blocks whose corners are\n"
      "given by tensor products of the specified BlockBounds. Each Block in\n"
      "the lattice is identified by a VolumeDim-tuple of zero-based indices\n"
      "Supplying a list of these tuples to BlocksToExclude will result in\n"
      "the domain having the corresponding Blocks excluded. See the Domain\n"
      "Creation tutorial in the documentation for more information on Block\n"
      "numberings in rectilinear domains. Note that if any Blocks are\n"
      "excluded, setting the option IsPeriodicIn to `true` in any dimension\n"
      "will trigger an error, as periodic boundary\n"
      "conditions for this domain with holes is not supported."};

  AlignedLattice(typename BlockBounds::type block_bounds,
                 typename IsPeriodicIn::type is_periodic_in,
                 typename InitialRefinement::type initial_refinement_levels,
                 typename InitialGridPoints::type initial_number_of_grid_points,
                 typename RefinedGridPoints::type refined_grid_points,
                 typename BlocksToExclude::type blocks_to_exclude) noexcept;

  AlignedLattice() = default;
  AlignedLattice(const AlignedLattice&) = delete;
  AlignedLattice(AlignedLattice&&) noexcept = default;
  AlignedLattice& operator=(const AlignedLattice&) = delete;
  AlignedLattice& operator=(AlignedLattice&&) noexcept = default;
  ~AlignedLattice() override = default;

  Domain<VolumeDim> create_domain() const noexcept override;

  std::vector<std::array<size_t, VolumeDim>> initial_extents() const
      noexcept override;

  std::vector<std::array<size_t, VolumeDim>> initial_refinement_levels() const
      noexcept override;

 private:
  typename BlockBounds::type block_bounds_{
      make_array<VolumeDim, std::vector<double>>({})};
  typename IsPeriodicIn::type is_periodic_in_{make_array<VolumeDim>(false)};
  typename InitialRefinement::type initial_refinement_levels_{
      make_array<VolumeDim>(std::numeric_limits<size_t>::max())};
  typename InitialGridPoints::type initial_number_of_grid_points_{
      make_array<VolumeDim>(std::numeric_limits<size_t>::max())};
  typename RefinedGridPoints::type refined_grid_points_{};
  typename BlocksToExclude::type blocks_to_exclude_{};
  Index<VolumeDim> number_of_blocks_by_dim_{};
};
}  // namespace creators
}  // namespace domain
