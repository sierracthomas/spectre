// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "PointwiseFunctions/GeneralRelativity/Christoffel.hpp"

#include "DataStructures/Tensor/Tensor.hpp"  // IWYU pragma: keep
#include "NumericalAlgorithms/LinearOperators/PartialDerivatives.tpp"
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
}  // namespace gr

namespace {
template <size_t Dim, typename Frame, typename DataType>
using variables_tags =
    tmpl::list<gr::Tags::SpatialChristoffelSecondKind<Dim, Frame, DataType>>;
}

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
#undef INDEXTYPE
#undef INSTANTIATE

#define INSTANTIATE(_, data)                                                 \
  template Variables<                                                        \
      tmpl::list<::Tags::deriv<gr::Tags::SpatialChristoffelSecondKind<       \
                                   DIM(data), FRAME(data), DTYPE(data)>,     \
                               tmpl::size_t<DIM(data)>, FRAME(data)>>>       \
  partial_derivatives<tmpl::list<gr::Tags::SpatialChristoffelSecondKind<     \
      DIM(data), FRAME(data), DTYPE(data)>>>(                                \
      const Variables<variables_tags<DIM(data), FRAME(data), DTYPE(data)>>&, \
      const Mesh<DIM(data)>&,                                                \
      const InverseJacobian<DTYPE(data), DIM(data), Frame::Logical,          \
                            FRAME(data)>&) noexcept;

GENERATE_INSTANTIATIONS(INSTANTIATE, (1, 2, 3), (DataVector),
                        (Frame::Grid, Frame::Inertial,
                         Frame::Spherical<Frame::Inertial>))
#undef DIM
#undef DTYPE
#undef FRAME
#undef INSTANTIATE
/// \endcond
