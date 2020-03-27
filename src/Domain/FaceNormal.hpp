// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Declares function unnormalized_face_normal

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>

#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataBox/Tag.hpp"
#include "DataStructures/Tensor/TypeAliases.hpp"
#include "Domain/CoordinateMaps/Tags.hpp"
#include "Domain/FunctionsOfTime/Tags.hpp"
#include "Domain/InterfaceComputeTags.hpp"
#include "Domain/Tags.hpp"  // IWYU pragma: keep
#include "Time/Tags.hpp"
#include "Utilities/TMPL.hpp"

/// \cond
namespace domain {
template <typename, typename, size_t>
class CoordinateMapBase;
}  // namespace domain
class DataVector;
template <size_t>
class Direction;
template <size_t Dim, typename Frame>
class ElementMap;
template <size_t>
class Mesh;
/// \endcond

// @{
/*!
 * \ingroup ComputationalDomainGroup
 * \brief Compute the outward grid normal on a face of an Element
 *
 * \returns outward grid-frame one-form holding the normal
 *
 * \details
 * Computes the grid-frame normal by taking the logical-frame unit
 * one-form in the given Direction and mapping it to the grid frame
 * with the given map.
 *
 * \example
 * \snippet Test_FaceNormal.cpp face_normal_example
 */
template <size_t VolumeDim, typename TargetFrame>
tnsr::i<DataVector, VolumeDim, TargetFrame> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const ElementMap<VolumeDim, TargetFrame>& map,
    const Direction<VolumeDim>& direction) noexcept;

template <size_t VolumeDim, typename TargetFrame>
tnsr::i<DataVector, VolumeDim, TargetFrame> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const domain::CoordinateMapBase<Frame::Logical, TargetFrame, VolumeDim>&
        map,
    const Direction<VolumeDim>& direction) noexcept;

template <size_t VolumeDim>
tnsr::i<DataVector, VolumeDim, Frame::Inertial> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const ElementMap<VolumeDim, Frame::Grid>& logical_to_grid_map,
    const domain::CoordinateMapBase<Frame::Grid, Frame::Inertial, VolumeDim>&
        grid_to_inertial_map,
    double time,
    const std::unordered_map<
        std::string, std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>>&
        functions_of_time,
    const Direction<VolumeDim>& direction) noexcept;
// @}

namespace domain {
namespace Tags {
/// \ingroup DataBoxTagsGroup
/// \ingroup ComputationalDomainGroup
/// The unnormalized face normal one form
template <size_t VolumeDim, typename Frame = ::Frame::Inertial>
struct UnnormalizedFaceNormal : db::SimpleTag {
  static std::string name() noexcept { return "UnnormalizedFaceNormal"; }
  using type = tnsr::i<DataVector, VolumeDim, Frame>;
};

template <size_t VolumeDim, typename Frame = ::Frame::Inertial>
struct UnnormalizedFaceNormalCompute
    : db::ComputeTag, UnnormalizedFaceNormal<VolumeDim, Frame> {
  using base = UnnormalizedFaceNormal<VolumeDim, Frame>;
  static constexpr tnsr::i<DataVector, VolumeDim, Frame> (*function)(
      const ::Mesh<VolumeDim - 1>&, const ::ElementMap<VolumeDim, Frame>&,
      const ::Direction<VolumeDim>&) = unnormalized_face_normal;
  using argument_tags =
      tmpl::list<Mesh<VolumeDim - 1>, ElementMap<VolumeDim, Frame>,
                 Direction<VolumeDim>>;
  using volume_tags = tmpl::list<ElementMap<VolumeDim, Frame>>;
};

template <size_t VolumeDim>
struct UnnormalizedFaceNormalMovingMeshCompute
    : db::ComputeTag,
      UnnormalizedFaceNormal<VolumeDim, Frame::Inertial> {
  using base = UnnormalizedFaceNormal<VolumeDim, Frame::Inertial>;
  static constexpr tnsr::i<DataVector, VolumeDim, Frame::Inertial> (*function)(
      const ::Mesh<VolumeDim - 1>&, const ::ElementMap<VolumeDim, Frame::Grid>&,
      const domain::CoordinateMapBase<Frame::Grid, Frame::Inertial, VolumeDim>&,
      double,
      const std::unordered_map<
          std::string,
          std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>>&,
      const ::Direction<VolumeDim>&) = unnormalized_face_normal;
  using argument_tags =
      tmpl::list<Mesh<VolumeDim - 1>, ElementMap<VolumeDim, Frame::Grid>,
                 CoordinateMaps::Tags::CoordinateMap<VolumeDim, Frame::Grid,
                                                     Frame::Inertial>,
                 ::Tags::Time, Tags::FunctionsOfTime, Direction<VolumeDim>>;
  using volume_tags =
      tmpl::list<ElementMap<VolumeDim, Frame::Grid>,
                 CoordinateMaps::Tags::CoordinateMap<VolumeDim, Frame::Grid,
                                                     Frame::Inertial>,
                 ::Tags::Time, Tags::FunctionsOfTime>;
};

/// \ingroup DataBoxTagsGroup
/// \ingroup ComputationalDomainGroup
/// Specialisation of UnnormalizedFaceNormal for the external boundaries which
/// inverts the normals. Since ExternalBoundariesDirections are meant to
/// represent ghost elements, the normals should correspond to the normals in
/// said element, which are inverted with respect to the current element.
template <size_t VolumeDim, typename Frame>
struct InterfaceCompute<Tags::BoundaryDirectionsExterior<VolumeDim>,
                        UnnormalizedFaceNormalCompute<VolumeDim, Frame>>
    : db::PrefixTag,
      db::ComputeTag,
      Tags::Interface<Tags::BoundaryDirectionsExterior<VolumeDim>,
                      Tags::UnnormalizedFaceNormal<VolumeDim, Frame>> {
  using dirs = BoundaryDirectionsExterior<VolumeDim>;

  static std::string name() noexcept {
    return "BoundaryDirectionsExterior<UnnormalizedFaceNormal>";
  }

  static auto function(
      const db::const_item_type<Tags::Interface<dirs, Mesh<VolumeDim - 1>>>&
          meshes,
      const db::const_item_type<Tags::ElementMap<VolumeDim, Frame>>&
          map) noexcept {
    std::unordered_map<::Direction<VolumeDim>,
                       tnsr::i<DataVector, VolumeDim, Frame>>
        normals{};
    for (const auto& direction_and_mesh : meshes) {
      const auto& direction = direction_and_mesh.first;
      const auto& mesh = direction_and_mesh.second;
      auto internal_face_normal =
          unnormalized_face_normal(mesh, map, direction);
      std::transform(internal_face_normal.begin(), internal_face_normal.end(),
                     internal_face_normal.begin(), std::negate<>());
      normals[direction] = std::move(internal_face_normal);
    }
    return normals;
  }

  using argument_tags = tmpl::list<Tags::Interface<dirs, Mesh<VolumeDim - 1>>,
                                   Tags::ElementMap<VolumeDim, Frame>>;
};

template <size_t VolumeDim>
struct InterfaceCompute<Tags::BoundaryDirectionsExterior<VolumeDim>,
                        UnnormalizedFaceNormalMovingMeshCompute<VolumeDim>>
    : db::PrefixTag,
      db::ComputeTag,
      Tags::Interface<
          Tags::BoundaryDirectionsExterior<VolumeDim>,
          Tags::UnnormalizedFaceNormal<VolumeDim, Frame::Inertial>> {
  using dirs = BoundaryDirectionsExterior<VolumeDim>;

  static std::string name() noexcept {
    return "BoundaryDirectionsExterior<UnnormalizedFaceNormal>";
  }

  static auto function(
      const db::const_item_type<Tags::Interface<dirs, Mesh<VolumeDim - 1>>>&
          meshes,
      const ::ElementMap<VolumeDim, Frame::Grid>& logical_to_grid_map,
      const domain::CoordinateMapBase<Frame::Grid, Frame::Inertial, VolumeDim>&
          grid_to_inertial_map,
      const double time,
      const std::unordered_map<
          std::string,
          std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>>&
          functions_of_time) noexcept {
    std::unordered_map<::Direction<VolumeDim>,
                       tnsr::i<DataVector, VolumeDim, Frame::Inertial>>
        normals{};
    for (const auto& direction_and_mesh : meshes) {
      const auto& direction = direction_and_mesh.first;
      const auto& mesh = direction_and_mesh.second;
      auto internal_face_normal = unnormalized_face_normal(
          mesh, logical_to_grid_map, grid_to_inertial_map, time,
          functions_of_time, direction);
      std::transform(internal_face_normal.begin(), internal_face_normal.end(),
                     internal_face_normal.begin(), std::negate<>());
      normals[direction] = std::move(internal_face_normal);
    }
    return normals;
  }

  using argument_tags =
      tmpl::list<Tags::Interface<dirs, Mesh<VolumeDim - 1>>,
                 Tags::ElementMap<VolumeDim, Frame::Grid>,
                 CoordinateMaps::Tags::CoordinateMap<VolumeDim, Frame::Grid,
                                                     Frame::Inertial>,
                 ::Tags::Time, Tags::FunctionsOfTime>;
};

}  // namespace Tags
}  // namespace domain
