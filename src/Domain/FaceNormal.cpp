// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Domain/FaceNormal.hpp"

#include "DataStructures/DataVector.hpp"            // IWYU pragma: keep
#include "DataStructures/Tensor/Tensor.hpp"         // IWYU pragma: keep
#include "Domain/CoordinateMaps/CoordinateMap.hpp"  // IWYU pragma: keep
#include "Domain/Direction.hpp"                     // IWYU pragma: keep
#include "Domain/ElementMap.hpp"                    // IWYU pragma: keep
#include "Domain/LogicalCoordinates.hpp"
#include "Domain/Mesh.hpp"
#include "Utilities/GenerateInstantiations.hpp"

namespace {
template <size_t VolumeDim, typename TargetFrame>
tnsr::i<DataVector, VolumeDim, TargetFrame> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const InverseJacobian<DataVector, VolumeDim, Frame::Logical, TargetFrame>&
        inv_jacobian_on_interface,
    const Direction<VolumeDim>& direction) noexcept {
  const auto sliced_away_dim = direction.dimension();
  const double sign = direction.sign();

  tnsr::i<DataVector, VolumeDim, TargetFrame> face_normal(
      interface_mesh.number_of_grid_points());

  for (size_t d = 0; d < VolumeDim; ++d) {
    face_normal.get(d) =
        sign * inv_jacobian_on_interface.get(sliced_away_dim, d);
  }
  return face_normal;
}
}  // namespace

template <size_t VolumeDim, typename TargetFrame>
tnsr::i<DataVector, VolumeDim, TargetFrame> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const ElementMap<VolumeDim, TargetFrame>& map,
    const Direction<VolumeDim>& direction) noexcept {
  return unnormalized_face_normal(
      interface_mesh,
      map.inv_jacobian(
          interface_logical_coordinates(interface_mesh, direction)),
      direction);
}

template <size_t VolumeDim, typename TargetFrame>
tnsr::i<DataVector, VolumeDim, TargetFrame> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const domain::CoordinateMapBase<Frame::Logical, TargetFrame, VolumeDim>&
        map,
    const Direction<VolumeDim>& direction) noexcept {
  return unnormalized_face_normal(
      interface_mesh,
      map.inv_jacobian(
          interface_logical_coordinates(interface_mesh, direction)),
      direction);
}

template <size_t VolumeDim>
tnsr::i<DataVector, VolumeDim, Frame::Inertial> unnormalized_face_normal(
    const Mesh<VolumeDim - 1>& interface_mesh,
    const ElementMap<VolumeDim, Frame::Grid>& logical_to_grid_map,
    const domain::CoordinateMapBase<Frame::Grid, Frame::Inertial, VolumeDim>&
        grid_to_inertial_map,
    const double time,
    const std::unordered_map<
        std::string, std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>>&
        functions_of_time,
    const Direction<VolumeDim>& direction) noexcept {
  auto logical_to_grid_inv_jac = logical_to_grid_map.inv_jacobian(
      interface_logical_coordinates(interface_mesh, direction));
  ::InverseJacobian<DataVector, VolumeDim, Frame::Logical, Frame::Inertial>
      logical_to_inertial_inv_jac{};

  if (grid_to_inertial_map.is_identity()) {
    for (size_t i = 0; i < logical_to_inertial_inv_jac.size(); ++i) {
      logical_to_inertial_inv_jac[i] = std::move(logical_to_grid_inv_jac[i]);
    }
  } else {
    const auto grid_to_inertial_inv_jac = grid_to_inertial_map.inv_jacobian(
        logical_to_grid_map(
            interface_logical_coordinates(interface_mesh, direction)),
        time, functions_of_time);

    for (size_t logical_i = 0; logical_i < VolumeDim; ++logical_i) {
      for (size_t inertial_i = 0; inertial_i < VolumeDim; ++inertial_i) {
        logical_to_inertial_inv_jac.get(logical_i, inertial_i) =
            logical_to_grid_inv_jac.get(logical_i, 0) *
            grid_to_inertial_inv_jac.get(0, inertial_i);
        for (size_t grid_i = 1; grid_i < VolumeDim; ++grid_i) {
          logical_to_inertial_inv_jac.get(logical_i, inertial_i) +=
              logical_to_grid_inv_jac.get(logical_i, grid_i) *
              grid_to_inertial_inv_jac.get(grid_i, inertial_i);
        }
      }
    }
  }

  return unnormalized_face_normal(interface_mesh, logical_to_inertial_inv_jac,
                                  direction);
}

#define GET_DIM(data) BOOST_PP_TUPLE_ELEM(0, data)
#define GET_FRAME(data) BOOST_PP_TUPLE_ELEM(1, data)

#define INSTANTIATION(_, data)                                                \
  template tnsr::i<DataVector, GET_DIM(data), GET_FRAME(data)>                \
  unnormalized_face_normal(const Mesh<GET_DIM(data) - 1>&,                    \
                           const ElementMap<GET_DIM(data), GET_FRAME(data)>&, \
                           const Direction<GET_DIM(data)>&) noexcept;         \
  template tnsr::i<DataVector, GET_DIM(data), GET_FRAME(data)>                \
  unnormalized_face_normal(                                                   \
      const Mesh<GET_DIM(data) - 1>&,                                         \
      const domain::CoordinateMapBase<Frame::Logical, GET_FRAME(data),        \
                                      GET_DIM(data)>&,                        \
      const Direction<GET_DIM(data)>&) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATION, (1, 2, 3),
                        (Frame::Inertial, Frame::Grid))

#undef INSTANTIATION

#define INSTANTIATION(_, data)                                              \
  template tnsr::i<DataVector, GET_DIM(data), Frame::Inertial>              \
  unnormalized_face_normal(                                                 \
      const Mesh<GET_DIM(data) - 1>& interface_mesh,                        \
      const ElementMap<GET_DIM(data), Frame::Grid>& logical_to_grid_map,    \
      const domain::CoordinateMapBase<Frame::Grid, Frame::Inertial,         \
                                      GET_DIM(data)>& grid_to_inertial_map, \
      const double time,                                                    \
      const std::unordered_map<                                             \
          std::string,                                                      \
          std::unique_ptr<domain::FunctionsOfTime::FunctionOfTime>>&        \
          functions_of_time,                                                \
      const Direction<GET_DIM(data)>& direction) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATION, (1, 2, 3))

#undef GET_DIM
#undef GET_FRAME
#undef INSTANTIATION
