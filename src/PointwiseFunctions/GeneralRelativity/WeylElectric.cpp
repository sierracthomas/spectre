// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "PointwiseFunctions/GeneralRelativity/WeylElectric.hpp"

#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/VectorImpl.hpp"
#include "Utilities/ContainerHelpers.hpp"
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
  tnsr::ii<DataType, SpatialDim, Frame> weyl_electric_part{};
  weyl_electric<SpatialDim, Frame, DataType>(
      make_not_null(&weyl_electric_part), spatial_ricci,
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
  *weyl_electric_part = spatial_ricci;
  for (size_t i = 0; i < SpatialDim; ++i) {
    for (size_t j = i; j < SpatialDim; ++j) {
      for (size_t k = 0; k < SpatialDim; ++k) {
        for (size_t l = 0; l < SpatialDim; ++l) {
          weyl_electric_part->get(i, j) +=
              extrinsic_curvature.get(k, l) *
                  (inverse_spatial_metric.get(k, l)) *
                  extrinsic_curvature.get(i, j) -
              extrinsic_curvature.get(i, l) *
                  (inverse_spatial_metric.get(k, l)) *
                  extrinsic_curvature.get(k, j);
        }
      }
    }
  }
}

namespace {
template <size_t SpatialDim, typename Frame, typename DataType>
void weyl_electric_scalar_impl(
    const gsl::not_null<Scalar<DataType>*> weyl_electric_scalar_result,
    const gsl::not_null<tnsr::ii<DataType, SpatialDim, Frame>*>
        weyl_electric_up_down,
    const tnsr::ii<DataType, SpatialDim, Frame>& weyl_electric,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept {
  for (size_t i = 0; i < SpatialDim; ++i) {
    for (size_t j = 0; j < SpatialDim; ++j) {
      for (size_t k = j; k < SpatialDim; ++k) {
        weyl_electric_up_down->get(j, k) +=
            weyl_electric.get(i, j) * inverse_spatial_metric.get(i, k);
      }
    }
  }
  for (size_t j = 0; j < SpatialDim; ++j) {
    for (size_t k = 0; k < SpatialDim; ++k) {
      if (UNLIKELY(j == 0 and k == 0)) {
        get(*weyl_electric_scalar_result) =
            weyl_electric_up_down->get(j, k) * weyl_electric_up_down->get(j, k);
      } else {
        get(*weyl_electric_scalar_result) +=
            weyl_electric_up_down->get(j, k) * weyl_electric_up_down->get(j, k);
      }
    }
  }
}
}  // namespace

template <size_t SpatialDim, typename Frame, typename DataType>
void weyl_electric_scalar(
    const gsl::not_null<Scalar<DataType>*> weyl_electric_scalar_result,
    const tnsr::ii<DataType, SpatialDim, Frame>& weyl_electric,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept {
  *weyl_electric_scalar_result =
      make_with_value<Scalar<DataType>>(get<0, 0>(inverse_spatial_metric), 0.0);

  tnsr::ii<DataType, 3> weyl_electric_up_down{
      get_size(get<0, 0>(inverse_spatial_metric)), 0.0};

  weyl_electric_scalar_impl(weyl_electric_scalar_result,
                            make_not_null(&weyl_electric_up_down),
                            weyl_electric, inverse_spatial_metric);
  }
  template <size_t SpatialDim, typename Frame, typename DataType>
  Scalar<DataType> weyl_electric_scalar(
      const tnsr::ii<DataType, SpatialDim, Frame>& weyl_electric,
      const tnsr::II<DataType, SpatialDim, Frame>&
          inverse_spatial_metric) noexcept {
    Scalar<DataType> weyl_electric_scalar_result{};
    weyl_electric_scalar<SpatialDim>(
        make_not_null(&weyl_electric_scalar_result), weyl_electric,
        inverse_spatial_metric);
    return weyl_electric_scalar_result;
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
          inverse_spatial_metric) noexcept;                                 \
  template Scalar<DTYPE(data)> gr::weyl_electric_scalar(                    \
      const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>& weyl_electric,   \
      const tnsr::II<DTYPE(data), DIM(data), FRAME(data)>&                  \
          inverse_spatial_metric) noexcept;                                 \
  template void gr::weyl_electric_scalar(                                   \
      const gsl::not_null<Scalar<DTYPE(data)>*> weyl_electric_scalar_result,  \
      const tnsr::ii<DTYPE(data), DIM(data), FRAME(data)>& weyl_electric,   \
      const tnsr::II<DTYPE(data), DIM(data), FRAME(data)>&                  \
          inverse_spatial_metric) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATE, (1, 2, 3), (double, DataVector),
                        (Frame::Grid, Frame::Inertial))

#undef DIM
#undef DTYPE
#undef FRAME
#undef INSTANTIATE
/// \endcond
