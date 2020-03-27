// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines class Interval.

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
}  // namespace CoordinateMaps

template <typename SourceFrame, typename TargetFrame, typename... Maps>
class CoordinateMap;
}  // namespace domain
/// \endcond

namespace domain {
namespace creators {
/// Create a 1D Domain consisting of a single Block.
class Interval : public DomainCreator<1> {
 public:
  using maps_list =
      tmpl::list<domain::CoordinateMap<Frame::Logical, Frame::Inertial,
                                       CoordinateMaps::Affine>>;

  struct LowerBound {
    using type = std::array<double, 1>;
    static constexpr OptionString help = {"Sequence of [x] for lower bounds."};
  };
  struct UpperBound {
    using type = std::array<double, 1>;
    static constexpr OptionString help = {"Sequence of [x] for upper bounds."};
  };
  struct IsPeriodicIn {
    using type = std::array<bool, 1>;
    static constexpr OptionString help = {
        "Sequence for [x], true if periodic."};
    static type default_value() noexcept { return make_array<1>(false); }
  };
  struct InitialRefinement {
    using type = std::array<size_t, 1>;
    static constexpr OptionString help = {"Initial refinement level in [x]."};
  };
  struct InitialGridPoints {
    using type = std::array<size_t, 1>;
    static constexpr OptionString help = {
        "Initial number of grid points in [x]."};
  };
  struct TimeDependence {
    using type =
        std::unique_ptr<domain::creators::time_dependence::TimeDependence<1>>;
    static constexpr OptionString help = {
        "The time dependence of the moving mesh domain."};
    static type default_value() noexcept;
  };

  using options =
      tmpl::list<LowerBound, UpperBound, IsPeriodicIn, InitialRefinement,
                 InitialGridPoints, TimeDependence>;

  static constexpr OptionString help = {"Creates a 1D interval."};

  Interval(typename LowerBound::type lower_x, typename UpperBound::type upper_x,
           typename IsPeriodicIn::type is_periodic_in_x,
           typename InitialRefinement::type initial_refinement_level_x,
           typename InitialGridPoints::type initial_number_of_grid_points_in_x,
           std::unique_ptr<domain::creators::time_dependence::TimeDependence<1>>
               time_dependence = nullptr) noexcept;

  Interval() = default;
  Interval(const Interval&) = delete;
  Interval(Interval&&) noexcept = default;
  Interval& operator=(const Interval&) = delete;
  Interval& operator=(Interval&&) noexcept = default;
  ~Interval() override = default;

  Domain<1> create_domain() const noexcept override;

  std::vector<std::array<size_t, 1>> initial_extents() const noexcept override;

  std::vector<std::array<size_t, 1>> initial_refinement_levels() const
      noexcept override;

  auto functions_of_time() const noexcept -> std::unordered_map<
      std::string,
      std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>> override;

 private:
  typename LowerBound::type lower_x_{};
  typename UpperBound::type upper_x_{};
  typename IsPeriodicIn::type is_periodic_in_x_{};
  typename InitialRefinement::type initial_refinement_level_x_{};
  typename InitialGridPoints::type initial_number_of_grid_points_in_x_{};
  std::unique_ptr<domain::creators::time_dependence::TimeDependence<1>>
      time_dependence_;
};
}  // namespace creators
}  // namespace domain
