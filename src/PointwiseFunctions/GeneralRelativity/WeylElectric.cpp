// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "PointwiseFunctions/GeneralRelativity/WeylElectric.hpp"

#include "DataStructures/Tensor/Tensor.hpp"
#include "Utilities/GenerateInstantiations.hpp"
#include "Utilities/MakeWithValue.hpp"

namespace gr {
template <size_t SpatialDim, typename Frame, typename DataType>
tnsr::ii<DataType, SpatialDim, Frame> weyl_electric(
    const tnsr::ii<DataType, SpatialDim, Frame>& spatial_ricci;
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature;
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept;
auto weyl = make_with_value<tnsr::ii<DataType, SpatialDim, Frame>>(
    christoffel_2nd_kind, 0.);
constexpr auto dimensionality = index_dim<0>(weyl);
for (size_t i = 0; i < dimensionality; ++i) {
  for (size_t j = i; j < dimensionality; ++j) {
    weyl.get(i, j) += spatial_ricci.get(i, j)

                          for (size_t k = 0; k < dimensionality; ++k) {
      for (size_t l = k; l < dimensionality; ++l) {
        weyl.get(i, j) += extrinsic_curvature.get(k, l) *
                          inverse_spatial_metric.get(k, l) *
                          extrinsic_curvature.get(i, j)

                              for (size_t m = 0; m < dimensionality; ++m) {
          weyl.get(i, j) += extrinsic_curvature.get(i, l) *
                            inverse_spatial_metric.get(m, l) *
                            extrinsic_curvature.get(m, j);
        }
      }
    }
  }
}
}  // namespace gr
}  // namespace gr

// Explicit Instantiations
/// \cond
#define DIM(data) BOOST_PP_TUPLE_ELEM(0, data)
#define DTYPE(data) BOOST_PP_TUPLE_ELEM(1, data)
#define FRAME(data) BOOST_PP_TUPLE_ELEM(2, data)

#define INSTANTIATE(_, data)
template tnsr::ii<DTYPE(data), DIM(data), FRAME(data)> gr::weyl_electric(
    const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>& spatial_ricci;
    const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>& extrinsic_curvature;
    const tnsr::II<DTYPE(data), DIM(data), FRAME(data)>&
        inverse_spatial_metric) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATE, (1, 2), (double, DataVector),
                        (Frame::Grid, Frame::Inertial),
                        (IndexType::Spatial)
#undef DIM
#undef DTYPE
#undef FRAME
#undef INSTANTIATE
/// \endcond
