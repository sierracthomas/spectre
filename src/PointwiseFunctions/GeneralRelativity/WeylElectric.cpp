// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "PointwiseFunctions/GeneralRelativity/WeylElectric.hpp"

#include "DataStructures/Tensor/Tensor.hpp"
#include "Utilities/GenerateInstantiations.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/MakeWithValue.hpp"

namespace gr {
template <size_t SpatialDim, typename Frame, typename DataType>
tnsr::ii<DataType, SpatialDim, Frame> weyl_electric(
    const tnsr::ii<DataType, SpatialDim, Frame>& spatial_ricci,
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept {
  //tnsr::ii<DataType, SpatialDim, Frame> weyl_electric_part{
  //get<0,0>(inverse_spatial_metric).size()}; //this line causes an error
  auto weyl_electric_part =
      make_with_value<tnsr::ii<DataType, SpatialDim, Frame>>(spatial_ricci, 0.);

  weyl_electric(make_not_null(&weyl_electric_part), spatial_ricci,
                extrinsic_curvature, inverse_spatial_metric);
  return weyl_electric_part;
}

template <size_t SpatialDim, typename Frame, typename DataType>
void weyl_electric(
    const gsl::not_null<tnsr::ii<DataType, SpatialDim, Frame>*>
        weyl_electric_part,
    const tnsr::ii<DataType, SpatialDim, Frame>& spatial_ricci,
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept {
  //  for(auto& tensor_component : *weyl_electric_part) {
  // tensor_component.destructive_resize(
  // get<0,
  // 0>(inverse_spatial_metric).size()); //this piece of code causes an error -
  // does not like checking the size of a double (maybe)

  for (size_t i = 0; i < SpatialDim; ++i) {
    for (size_t j = i; j < SpatialDim; ++j) {
      weyl_electric_part->get(i, j) = spatial_ricci.get(i, j);

      for (size_t k = 0; k < SpatialDim; ++k) {
        for (size_t l = 0; l < SpatialDim; ++l) {
          weyl_electric_part->get(i, j) +=
              extrinsic_curvature.get(k, l) * inverse_spatial_metric.get(k, l) *
                  extrinsic_curvature.get(i, j) -
              extrinsic_curvature.get(i, l) * inverse_spatial_metric.get(k, l) *
                  extrinsic_curvature.get(k, j);
        }
      }
    }
  }
}
}  // namespace gr

// Explicit Instantiations
/// \cond
#define DIM(data) BOOST_PP_TUPLE_ELEM(0, data)
#define DTYPE(data) BOOST_PP_TUPLE_ELEM(1, data)
#define FRAME(data) BOOST_PP_TUPLE_ELEM(2, data)

#define INSTANTIATE(_, data)                                                \
  template tnsr::ii<DTYPE(data), DIM(data), FRAME(data)> gr::weyl_electric( \
      const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>& spatial_ricci,   \
      const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>&                  \
          extrinsic_curvature,                                              \
      const tnsr::II<DTYPE(data), DIM(data), FRAME(data)>&                  \
          inverse_spatial_metric) noexcept;                                 \
  template void gr::weyl_electric(                                          \
      const gsl::not_null<tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>*>   \
          weyl_electric_part,                                               \
      const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>& spatial_ricci,   \
      const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>&                  \
          extrinsic_curvature,                                              \
      const tnsr::II<DTYPE(data), DIM(data), FRAME(data)>&                  \
          inverse_spatial_metric) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATE, (1, 2, 3), (double, DataVector),
                        (Frame::Grid, Frame::Inertial))
#undef DIM
#undef DTYPE
#undef FRAME
#undef INSTANTIATE
/// \endcond
