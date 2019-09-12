// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <utility>

#include "DataStructures/DataBox/DataBox.hpp"
#include "IO/Observer/ObservationId.hpp"
#include "IO/Observer/TypeOfObservation.hpp"
#include "Time/Tags.hpp"

namespace observers {
/// Passed to `RegisterWithObservers` action to register observer event.
template <typename ObsType>
struct RegisterObservers {
  template <typename ParallelComponent, typename DbTagsList,
            typename ArrayIndex>
  static std::pair<observers::TypeOfObservation, observers::ObservationId>
  register_info(const db::DataBox<DbTagsList>& box,
                const ArrayIndex& /*array_index*/) noexcept {
    return {observers::TypeOfObservation::ReductionAndVolume,
            observers::ObservationId{db::get<::Tags::Time>(box), ObsType{}}};
  }
};
}  // namespace observers
