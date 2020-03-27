// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Domain/Block.hpp"

#include <ios>
#include <pup.h>  // IWYU pragma: keep

#include "ErrorHandling/Assert.hpp"
#include "Parallel/PupStlCpp11.hpp"  // IWYU pragma: keep
#include "Utilities/GenerateInstantiations.hpp"

namespace Frame {
struct Inertial;
struct Logical;
}  // namespace Frame

template <size_t VolumeDim>
Block<VolumeDim>::Block(
    std::unique_ptr<domain::CoordinateMapBase<Frame::Logical, Frame::Inertial,
                                              VolumeDim>>&& stationary_map,
    const size_t id,
    DirectionMap<VolumeDim, BlockNeighbor<VolumeDim>> neighbors) noexcept
    : stationary_map_(std::move(stationary_map)),
      id_(id),
      neighbors_(std::move(neighbors)) {
  // Loop over Directions to search which Directions were not set to neighbors_,
  // set these Directions to external_boundaries_.
  for (const auto& direction : Direction<VolumeDim>::all_directions()) {
    if (neighbors_.find(direction) == neighbors_.end()) {
      external_boundaries_.emplace(std::move(direction));
    }
  }
}

template <size_t VolumeDim>
const domain::CoordinateMapBase<Frame::Logical, Frame::Inertial, VolumeDim>&
Block<VolumeDim>::stationary_map() const noexcept {
  ASSERT(stationary_map_ != nullptr,
         "The stationary map is set to nullptr and so cannot be retrieved. "
         "This is because the domain is time-dependent and so there are two "
         "maps: the Logical to Grid map and the Grid to Inertial map.");
  return *stationary_map_;
}

template <size_t VolumeDim>
const domain::CoordinateMapBase<Frame::Logical, Frame::Grid, VolumeDim>&
Block<VolumeDim>::moving_mesh_logical_to_grid_map() const noexcept {
  ASSERT(moving_mesh_grid_map_ != nullptr,
         "The moving mesh Logical to Grid map is set to nullptr and so cannot "
         "be retrieved. This is because the domain is time-independent and so "
         "only the stationary map exists.");
  return *moving_mesh_grid_map_;
}

template <size_t VolumeDim>
const domain::CoordinateMapBase<Frame::Grid, Frame::Inertial, VolumeDim>&
Block<VolumeDim>::moving_mesh_grid_to_inertial_map() const noexcept {
  ASSERT(moving_mesh_inertial_map_ != nullptr,
         "The moving mesh Grid to Inertial map is set to nullptr and so cannot "
         "be retrieved. This is because the domain is time-independent and so "
         "only the stationary map exists.");
  return *moving_mesh_inertial_map_;
}

template <size_t VolumeDim>
void Block<VolumeDim>::inject_time_dependent_map(
    std::unique_ptr<
        domain::CoordinateMapBase<Frame::Grid, Frame::Inertial, VolumeDim>>
        moving_mesh_inertial_map) noexcept {
  ASSERT(stationary_map_ != nullptr,
         "Cannot inject time-dependent map into a block that already has a "
         "time-dependent map.");
  moving_mesh_inertial_map_ = std::move(moving_mesh_inertial_map);
  moving_mesh_grid_map_ = stationary_map_->get_to_grid_frame();
  stationary_map_ = nullptr;
}

template <size_t VolumeDim>
void Block<VolumeDim>::pup(PUP::er& p) noexcept {
  p | stationary_map_;
  p | moving_mesh_grid_map_;
  p | moving_mesh_inertial_map_;
  p | id_;
  p | neighbors_;
  p | external_boundaries_;
}

template <size_t VolumeDim>
std::ostream& operator<<(std::ostream& os,
                         const Block<VolumeDim>& block) noexcept {
  os << "Block " << block.id() << ":\n";
  os << "Neighbors: " << block.neighbors() << '\n';
  os << "External boundaries: " << block.external_boundaries() << '\n';
  os << "Is time dependent: " << std::boolalpha << block.is_time_dependent();
  return os;
}

template <size_t VolumeDim>
bool operator==(const Block<VolumeDim>& lhs,
                const Block<VolumeDim>& rhs) noexcept {
  return lhs.id() == rhs.id() and lhs.neighbors() == rhs.neighbors() and
         lhs.external_boundaries() == rhs.external_boundaries() and
         lhs.is_time_dependent() == rhs.is_time_dependent() and
         (lhs.is_time_dependent()
              ? (lhs.moving_mesh_logical_to_grid_map() ==
                     rhs.moving_mesh_logical_to_grid_map() and
                 lhs.moving_mesh_grid_to_inertial_map() ==
                     rhs.moving_mesh_grid_to_inertial_map())
              : lhs.stationary_map() == rhs.stationary_map());
}

template <size_t VolumeDim>
bool operator!=(const Block<VolumeDim>& lhs,
                const Block<VolumeDim>& rhs) noexcept {
  return not(lhs == rhs);
}

#define GET_DIM(data) BOOST_PP_TUPLE_ELEM(0, data)

#define INSTANTIATION(r, data)                                          \
  template class Block<GET_DIM(data)>;                                  \
  template std::ostream& operator<<(std::ostream& os,                   \
                                    const Block<GET_DIM(data)>& block); \
  template bool operator==(const Block<GET_DIM(data)>& lhs,             \
                           const Block<GET_DIM(data)>& rhs) noexcept;   \
  template bool operator!=(const Block<GET_DIM(data)>& lhs,             \
                           const Block<GET_DIM(data)>& rhs) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATION, (1, 2, 3))

#undef GET_DIM
#undef INSTANTIATION
