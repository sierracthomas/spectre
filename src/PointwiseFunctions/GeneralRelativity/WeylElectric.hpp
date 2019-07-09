// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>

#include "DataStructures/Tensor/TypeAliases.hpp"

namespace gr {

/*!
 * \ingroup GeneralRelativityGroup
 * \brief Computes the electric part of the Weyl tensor in vacuum.
 *
 * \details Computes the electric part of the Weyl tensor in vacuum \f$E_{ij}\f$
 * as: \f$ E_{ij} = R_{ij} + KK_{ij} - K^m_{i}K_{mj}\f$ where \f$\R_{ij}\f$ is
 * the spatial Ricci tensor, \f$K_{ij}\f$ is the extrinsic curvature, and
 * \f$K\f$ is the trace of \f$K_{ij}\f$.
 */
template <size_t SpatialDim, typename Frame, typename DataType>
tnsr::ii<DataType, SpatialDim, Frame> weyl_electric(
    const tnsr::ii<DataType, SpatialDim, Frame>& spatial_ricci;
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature;
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept;

}  // namespace gr
