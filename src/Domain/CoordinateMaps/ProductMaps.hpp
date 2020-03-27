// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines the class templates ProductOf2Maps and ProductOf3Maps.

#pragma once

#include <array>
#include <boost/optional.hpp>
#include <cstddef>
#include <functional>
#include <utility>

#include "DataStructures/Tensor/Tensor.hpp"
#include "Utilities/DereferenceWrapper.hpp"
#include "Utilities/MakeWithValue.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TypeTraits/RemoveReferenceWrapper.hpp"

/// \cond
namespace PUP {
class er;
}  // namespace PUP
/// \endcond

namespace domain {
namespace CoordinateMaps {
/// \ingroup CoordinateMapsGroup
/// \brief Product of two codimension=0 CoordinateMaps.
///
/// \tparam Map1 the map for the first coordinate(s)
/// \tparam Map2 the map for the second coordinate(s)
template <typename Map1, typename Map2>
class ProductOf2Maps {
 public:
  static constexpr size_t dim = Map1::dim + Map2::dim;
  using map_list = tmpl::list<Map1, Map2>;
  static_assert(dim == 2 or dim == 3,
                "Only 2D and 3D maps are supported by ProductOf2Maps");

  // Needed for Charm++ serialization
  ProductOf2Maps() = default;

  ProductOf2Maps(Map1 map1, Map2 map2) noexcept;

  template <typename T>
  std::array<tt::remove_cvref_wrap_t<T>, dim> operator()(
      const std::array<T, dim>& source_coords) const noexcept;

  boost::optional<std::array<double, dim>> inverse(
      const std::array<double, dim>& target_coords) const noexcept;

  template <typename T>
  tnsr::Ij<tt::remove_cvref_wrap_t<T>, dim, Frame::NoFrame> inv_jacobian(
      const std::array<T, dim>& source_coords) const noexcept;

  template <typename T>
  tnsr::Ij<tt::remove_cvref_wrap_t<T>, dim, Frame::NoFrame> jacobian(
      const std::array<T, dim>& source_coords) const noexcept;

  // clang-tidy: google-runtime-references
  void pup(PUP::er& p);  // NOLINT

  bool is_identity() const noexcept { return is_identity_; }

 private:
  friend bool operator==(const ProductOf2Maps& lhs,
                         const ProductOf2Maps& rhs) noexcept {
    return lhs.map1_ == rhs.map1_ and lhs.map2_ == rhs.map2_ and
           lhs.is_identity_ == rhs.is_identity_;
  }

  Map1 map1_;
  Map2 map2_;
  bool is_identity_ = false;
};

template <typename Map1, typename Map2>
bool operator!=(const ProductOf2Maps<Map1, Map2>& lhs,
                const ProductOf2Maps<Map1, Map2>& rhs) noexcept;

/// \ingroup CoordinateMapsGroup
/// \brief Product of three one-dimensional CoordinateMaps.
template <typename Map1, typename Map2, typename Map3>
class ProductOf3Maps {
 public:
  static constexpr size_t dim = Map1::dim + Map2::dim + Map3::dim;
  using map_list = tmpl::list<Map1, Map2, Map3>;
  static_assert(dim == 3, "Only 3D maps are implemented for ProductOf3Maps");

  // Needed for Charm++ serialization
  ProductOf3Maps() = default;

  ProductOf3Maps(Map1 map1, Map2 map2, Map3 map3) noexcept;

  template <typename T>
  std::array<tt::remove_cvref_wrap_t<T>, dim> operator()(
      const std::array<T, dim>& source_coords) const noexcept;

  boost::optional<std::array<double, dim>> inverse(
      const std::array<double, dim>& target_coords) const noexcept;

  template <typename T>
  tnsr::Ij<tt::remove_cvref_wrap_t<T>, dim, Frame::NoFrame> inv_jacobian(
      const std::array<T, dim>& source_coords) const noexcept;

  template <typename T>
  tnsr::Ij<tt::remove_cvref_wrap_t<T>, dim, Frame::NoFrame> jacobian(
      const std::array<T, dim>& source_coords) const noexcept;

  // clang-tidy: google-runtime-references
  void pup(PUP::er& p) noexcept;  // NOLINT

  bool is_identity() const noexcept { return is_identity_; }

 private:
  friend bool operator==(const ProductOf3Maps& lhs,
                         const ProductOf3Maps& rhs) noexcept {
    return lhs.map1_ == rhs.map1_ and lhs.map2_ == rhs.map2_ and
           lhs.map3_ == rhs.map3_ and lhs.is_identity_ == rhs.is_identity_;
  }

  Map1 map1_;
  Map2 map2_;
  Map3 map3_;
  bool is_identity_ = false;
};

template <typename Map1, typename Map2, typename Map3>
bool operator!=(const ProductOf3Maps<Map1, Map2, Map3>& lhs,
                const ProductOf3Maps<Map1, Map2, Map3>& rhs) noexcept;
}  // namespace CoordinateMaps
}  // namespace domain
