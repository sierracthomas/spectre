// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <string>

#include "DataStructures/DataBox/Tag.hpp"
#include "DataStructures/DataVector.hpp"  // IWYU pragma: keep
#include "DataStructures/Tensor/TypeAliases.hpp"

namespace Burgers {
namespace Tags {
struct U : db::SimpleTag {
  using type = Scalar<DataVector>;
};
}  // namespace Tags
}  // namespace Burgers
