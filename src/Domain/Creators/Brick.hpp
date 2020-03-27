// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines class Brick.

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "Domain/Creators/DomainCreator.hpp"  // IWYU pragma: keep
#include "Domain/Creators/TimeDependence/TimeDependence.hpp"
#include "Domain/Domain.hpp"
#include "Options/Options.hpp"
#include "Utilities/MakeArray.hpp"
#include "Utilities/TMPL.hpp"

/// \cond
namespace domain {
namespace CoordinateMaps {
class Affine;
template <typename Map1, typename Map2, typename Map3>
class ProductOf3Maps;
}  // namespace CoordinateMaps

template <typename SourceFrame, typename TargetFrame, typename... Maps>
class CoordinateMap;
}  // namespace domain
/// \endcond

namespace domain {
namespace creators {

/// Create a 3D Domain consisting of a single Block.
class Brick : public DomainCreator<3> {
 public:
  using maps_list = tmpl::list<
      domain::CoordinateMap<Frame::Logical, Frame::Inertial,
                            CoordinateMaps::ProductOf3Maps<
                                CoordinateMaps::Affine, CoordinateMaps::Affine,
                                CoordinateMaps::Affine>>>;

  struct LowerBound {
    using type = std::array<double, 3>;
    static constexpr OptionString help = {
        "Sequence of [x,y,z] for lower bounds."};
  };

  struct UpperBound {
    using type = std::array<double, 3>;
    static constexpr OptionString help = {
        "Sequence of [x,y,z] for upper bounds."};
  };
  struct IsPeriodicIn {
    using type = std::array<bool, 3>;
    static constexpr OptionString help = {
        "Sequence for [x,y,z], true if periodic."};
    static type default_value() noexcept { return make_array<3>(false); }
  };

  struct InitialRefinement {
    using type = std::array<size_t, 3>;
    static constexpr OptionString help = {
        "Initial refinement level in [x,y,z]."};
  };

  struct InitialGridPoints {
    using type = std::array<size_t, 3>;
    static constexpr OptionString help = {
        "Initial number of grid points in [x,y,z]."};
  };

  struct TimeDependence {
    using type =
        std::unique_ptr<domain::creators::time_dependence::TimeDependence<3>>;
    static constexpr OptionString help = {
        "The time dependence of the moving mesh domain."};
    static type default_value() noexcept;
  };

  using options =
      tmpl::list<LowerBound, UpperBound, IsPeriodicIn, InitialRefinement,
                 InitialGridPoints, TimeDependence>;

  static constexpr OptionString help{"Creates a 3D brick."};

  Brick(typename LowerBound::type lower_xyz,
        typename UpperBound::type upper_xyz,
        typename IsPeriodicIn::type is_periodic_in_xyz,
        typename InitialRefinement::type initial_refinement_level_xyz,
        typename InitialGridPoints::type initial_number_of_grid_points_in_xyz,
        std::unique_ptr<domain::creators::time_dependence::TimeDependence<3>>
            time_dependence = nullptr) noexcept;

  Brick() = default;
  Brick(const Brick&) = delete;
  Brick(Brick&&) noexcept = default;
  Brick& operator=(const Brick&) = delete;
  Brick& operator=(Brick&&) noexcept = default;
  ~Brick() noexcept override = default;

  Domain<3> create_domain() const noexcept override;

  std::vector<std::array<size_t, 3>> initial_extents() const noexcept override;

  std::vector<std::array<size_t, 3>> initial_refinement_levels() const
      noexcept override;

  auto functions_of_time() const noexcept -> std::unordered_map<
      std::string,
      std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>> override;

 private:
  typename LowerBound::type lower_xyz_{};
  typename UpperBound::type upper_xyz_{};
  typename IsPeriodicIn::type is_periodic_in_xyz_{};
  typename InitialRefinement::type initial_refinement_level_xyz_{};
  typename InitialGridPoints::type initial_number_of_grid_points_in_xyz_{};
  std::unique_ptr<domain::creators::time_dependence::TimeDependence<3>>
      time_dependence_;
};
}  // namespace creators
}  // namespace domain
