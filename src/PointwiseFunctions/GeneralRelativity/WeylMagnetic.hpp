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
 * \brief Computes the magnetic part of the Weyl tensor in vacuum.
 *
 * \details Computes the magnetic part of the Weyl tensor in vacuum \f$B_{ij}\f$
 * as : \f$ B_{ij} = -\frac{1}{2}C_{abc}{}_{(i}\epsilon_{j)d}^{ba}n^{c}n^{d}\f$
 * \f$ B_{ij} = \frac{1}{2}\epsilon^{bc}{}_{(i}C_{j)abc}n^{a}\f$
 * \f$ B_{ij} = D_{a}K_{b}{}_{(i}\epsilon_{j)}^{ba}\f$
 *
 * Where \f$n^a\f$ is the unit normal to the slice, \f$\epsilon_{abcd}\f$
 * is the Levi-Civita tensor (i.e. 4-volume form) defined so that
 * \f$\epsilon_{0123} = +1\f$, \f$\epsilon_{abc}\f$ is the spatial Levi-Civita
 * tensor (i.e. 3-volume form) defined so that \f$\epsilon_{abc}=
 * n^{d}\epsilon_{dabc}\f$ and \f$\epsilon_{123}=+1\f$, \f$K_{ij}\f$ is the
 * extrinsic curvature, and \f$D_{i}\f$ is the spatial covariant derivative.
 *
 *
 * \note In the sign conventions used in this code, the north pole of
 * a Kerr black hole has negative vorticity. This convention allows us
 * to write Maxwell-like equations for the propagation of the electric and
 * magnetic Weyl tensors with the more familiar sign convention from
 * from electromagnetism.
 */
template <size_t SpatialDim, typename Frame, typename DataType>
tnsr::ii<DataType, SpatialDim, Frame> weyl_magnetic(
    const tnsr::ii<DataType, SpatialDim, Frame>& extrinsic_curvature,
    const tnsr::i<double, 1>& unit_normal) noexcept;

}  // namespace gr
