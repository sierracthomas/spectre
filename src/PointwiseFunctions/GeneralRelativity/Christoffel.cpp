// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "PointwiseFunctions/GeneralRelativity/Christoffel.hpp"

#include "DataStructures/Tensor/Tensor.hpp"  // IWYU pragma: keep
#include "PointwiseFunctions/GeneralRelativity/IndexManipulation.hpp"
#include "PointwiseFunctions/GeneralRelativity/SpatialMetric.hpp"
#include "Utilities/GenerateInstantiations.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/MakeWithValue.hpp"

/// \cond
namespace gr {
template <size_t SpatialDim, typename Frame, IndexType Index, typename DataType>
void christoffel_first_kind(
    const gsl::not_null<tnsr::abb<DataType, SpatialDim, Frame, Index>*>
        christoffel,
    const tnsr::abb<DataType, SpatialDim, Frame, Index>& d_metric) noexcept {
  constexpr auto dimensionality =
      Index == IndexType::Spatial ? SpatialDim : SpatialDim + 1;
  for (size_t k = 0; k < dimensionality; ++k) {
    for (size_t i = 0; i < dimensionality; ++i) {
      for (size_t j = i; j < dimensionality; ++j) {
        christoffel->get(k, i, j) =
            0.5 * (d_metric.get(i, j, k) + d_metric.get(j, i, k) -
                   d_metric.get(k, i, j));
      }
    }
  }
}

template <size_t SpatialDim, typename Frame, IndexType Index, typename DataType>
tnsr::abb<DataType, SpatialDim, Frame, Index> christoffel_first_kind(
    const tnsr::abb<DataType, SpatialDim, Frame, Index>& d_metric) noexcept {
  auto christoffel =
      make_with_value<tnsr::abb<DataType, SpatialDim, Frame, Index>>(d_metric,
                                                                     0.);
  christoffel_first_kind(make_not_null(&christoffel), d_metric);
  return christoffel;
}

template <size_t SpatialDim, typename Frame, IndexType Index, typename DataType>
void spatial_christoffel_second_kind(
    const gsl::not_null<tnsr::Ijj<DataType, SpatialDim, Frame>*>
        christoffel_second_kind_pointer,
    const tnsr::ijj<DataType, SpatialDim, Frame>& christoffel_first_kind,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept {
  christoffel_first_kind =
      &raise_or_lower_first_index<DataType,
                                  SpatialIndex<SpatialDim, UpLo::Lo, Frame>,
                                  SpatialIndex<SpatialDim, UpLo::Lo, Frame>>;
  *christoffel_second_kind_pointer = christoffel_first_kind;
}

template <size_t SpatialDim, typename Frame, IndexType Index, typename DataType>
tnsr::Ijj<DataType, SpatialDim, Frame> spatial_christoffel_second_kind(
    const tnsr::ijj<DataType, SpatialDim, Frame>& christoffel_first_kind,
    const tnsr::II<DataType, SpatialDim, Frame>&
        inverse_spatial_metric) noexcept {
  auto christoffel_second_kind_pointer =
      make_with_value<tnsr::Ijj<DataType, SpatialDim, Frame>>(
          inverse_spatial_metric, 0.);
  spatial_christoffel_second_kind(
      make_not_null(&christoffel_second_kind_pointer), inverse_spatial_metric);
  return christoffel_second_kind_pointer;
}
}  // namespace gr

#define DIM(data) BOOST_PP_TUPLE_ELEM(0, data)
#define DTYPE(data) BOOST_PP_TUPLE_ELEM(1, data)
#define FRAME(data) BOOST_PP_TUPLE_ELEM(2, data)
#define INDEXTYPE(data) BOOST_PP_TUPLE_ELEM(3, data)

// The not_null versions are instantiated during instantiation of the
// return-by-value versions.
#define INSTANTIATE(_, data)                                                 \
  template tnsr::abb<DTYPE(data), DIM(data), FRAME(data), INDEXTYPE(data)>   \
  gr::christoffel_first_kind(                                                \
      const tnsr::abb<DTYPE(data), DIM(data), FRAME(data), INDEXTYPE(data)>& \
          d_metric) noexcept;                                                \
  template void gr::christoffel_first_kind(                                  \
      const gsl::not_null<                                                   \
          tnsr::abb<DTYPE(data), DIM(data), FRAME(data), INDEXTYPE(data)>*>  \
          christoffel,                                                       \
      const tnsr::abb<DTYPE(data), DIM(data), FRAME(data), INDEXTYPE(data)>& \
          d_metric) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATE, (1, 2, 3), (double, DataVector),
                        (Frame::Grid, Frame::Inertial,
                         Frame::Spherical<Frame::Inertial>),
                        (IndexType::Spatial, IndexType::Spacetime))

#undef DIM
#undef DTYPE
#undef FRAME
#undef INDEXTYPE
#undef INSTANTIATE
/// \endcond
