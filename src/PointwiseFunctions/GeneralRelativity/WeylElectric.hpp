// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>

#include "DataStructures/Tensor/TypeAliases.hpp"
#include "PointwiseFunctions/GeneralRelativity/Tags.hpp"

#include "Utilities/Gsl.hpp"

namespace gr {

/*!
 * \ingroup GeneralRelativityGroup
 * \brief Computes the electric part of the Weyl tensor in vacuum.
 *
 * \details Computes the electric part of the Weyl tensor in vacuum \f$E_{ij}\f$
 * as: \f$ E_{ij} = R_{ij} + KK_{ij} - K^m_{i}K_{mj}\f$ where \f$R_{ij}\f$ is
 * the spatial Ricci tensor, \f$K_{ij}\f$ is the extrinsic curvature, and
 * \f$K\f$ is the trace of \f$K_{ij}\f$. An additional definition is E_{ij} =
 * n^a n^b C_{a i b j}, where n is the unit-normal to the hypersurface and C is
 * the Weyl tensor consistent with the conventions in \cite Boyle2019kee.
 * \note This needs additional terms for computations in a non-vacuum.
 */
template <size_t SpatialDim, typename Frame, typename DataType>
tnsr::ii<DataType, SpatialDim, Frame> weyl_electric(
    const tnsr::ii<DataType, SpatialDim, Frame>& spatial_ricci,
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept;

template <size_t SpatialDim, typename Frame, typename DataType>
void weyl_electric(
    const gsl::not_null<tnsr::ii<DataType, SpatialDim, Frame>*>
        weyl_electric_part,
    const tnsr::ii<DataType, SpatialDim, Frame>& spatial_ricci,
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept;

namespace Tags {
/// Compute item for the electric part of the weyl tensor in vacuum
/// Computed from the RicciTensor, ExtrinsicCurvature, and InverseSpatialMetric
///
/// Can be retrieved using gr::Tags::WeylElectric
template <size_t SpatialDim, typename Frame, typename DataType>
struct WeylElectricCompute : WeylElectric<SpatialDim, Frame, DataType>,
                             db::ComputeTag {
  static constexpr tnsr::ii<DataType, SpatialDim, Frame> (*function)(
      const tnsr::ii<DataType, SpatialDim, Frame>&,
      const tnsr::ii<DataType, SpatialDim, Frame>&,
      const tnsr::II<DataType, SpatialDim, Frame>&) =
          &weyl_electric<SpatialDim, Frame, DataType>;
  using argument_tags = tmpl::list<
      gr::Tags::RicciTensor<SpatialDim, Frame, DataType>,
      gr::Tags::ExtrinsicCurvature<SpatialDim, Frame, DataType>,
      gr::Tags::InverseSpatialMetric<SpatialDim, Frame, DataType>>;
};
}  // namespace Tags
}  // namespace gr
