// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Evolution/Systems/CurvedScalarWave/Characteristics.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include "DataStructures/DataVector.hpp"
#include "DataStructures/Tensor/EagerMath/DotProduct.hpp"
#include "Evolution/Systems/CurvedScalarWave/Tags.hpp"
#include "PointwiseFunctions/GeneralRelativity/IndexManipulation.hpp"
#include "Utilities/GenerateInstantiations.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/MakeWithValue.hpp"

namespace CurvedScalarWave {
template <size_t SpatialDim>
void characteristic_speeds(
    const gsl::not_null<std::array<DataVector, 4>*> char_speeds,
    const Scalar<DataVector>& gamma_1, const Scalar<DataVector>& lapse,
    const tnsr::I<DataVector, SpatialDim, Frame::Inertial>& shift,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>&
        unit_normal_one_form) noexcept {
  if (UNLIKELY(get_size((*char_speeds)[0]) != get_size(get(gamma_1)))) {
    *char_speeds = make_with_value<std::array<DataVector, 4>>(
        get(gamma_1), std::numeric_limits<double>::signaling_NaN());
  }
  const auto shift_dot_normal = get(dot_product(shift, unit_normal_one_form));
  (*char_speeds)[0] = -(1. + get(gamma_1)) * shift_dot_normal;  // v(VPsi)
  (*char_speeds)[1] = -shift_dot_normal;                        // v(VZero)
  (*char_speeds)[2] = -shift_dot_normal + get(lapse);           // v(VPlus)
  (*char_speeds)[3] = -shift_dot_normal - get(lapse);           // v(VMinus)
}

template <size_t SpatialDim>
std::array<DataVector, 4> characteristic_speeds(
    const Scalar<DataVector>& gamma_1, const Scalar<DataVector>& lapse,
    const tnsr::I<DataVector, SpatialDim, Frame::Inertial>& shift,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>&
        unit_normal_one_form) noexcept {
  auto char_speeds = make_with_value<std::array<DataVector, 4>>(
      get<0>(unit_normal_one_form),
      std::numeric_limits<double>::signaling_NaN());
  characteristic_speeds(make_not_null(&char_speeds), gamma_1, lapse, shift,
                        unit_normal_one_form);
  return char_speeds;
}

template <size_t SpatialDim>
void characteristic_fields(
    const gsl::not_null<Variables<tmpl::list<
        Tags::VPsi, Tags::VZero<SpatialDim>, Tags::VPlus, Tags::VMinus>>*>
        char_fields,
    const Scalar<DataVector>& gamma_2,
    const tnsr::II<DataVector, SpatialDim, Frame::Inertial>&
        inverse_spatial_metric,
    const Scalar<DataVector>& psi, const Scalar<DataVector>& pi,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>& phi,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>&
        unit_normal_one_form) noexcept {
  if (UNLIKELY(get_size(get(get<Tags::VPsi>(*char_fields))) !=
               get_size(get(gamma_2)))) {
    *char_fields =
        Variables<tmpl::list<Tags::VPsi, Tags::VZero<SpatialDim>, Tags::VPlus,
                             Tags::VMinus>>(get_size(get(gamma_2)));
  }
  const auto phi_dot_normal = dot_product(
      raise_or_lower_index(unit_normal_one_form, inverse_spatial_metric), phi);

  // Eq.(34) of Holst+ (2004) for VZero
  for (size_t i = 0; i < SpatialDim; ++i) {
    get<Tags::VZero<SpatialDim>>(*char_fields).get(i) =
        phi.get(i) - unit_normal_one_form.get(i) * get(phi_dot_normal);
  }

  // Eq.(33) of Holst+ (2004) for VPsi
  get<Tags::VPsi>(*char_fields) = psi;

  // Eq.(35) of Holst+ (2004) for VPlus and VMinus
  get(get<Tags::VPlus>(*char_fields)) =
      get(pi) + get(phi_dot_normal) - get(gamma_2) * get(psi);
  get(get<Tags::VMinus>(*char_fields)) =
      get(pi) - get(phi_dot_normal) - get(gamma_2) * get(psi);
}

template <size_t SpatialDim>
Variables<
    tmpl::list<Tags::VPsi, Tags::VZero<SpatialDim>, Tags::VPlus, Tags::VMinus>>
characteristic_fields(
    const Scalar<DataVector>& gamma_2,
    const tnsr::II<DataVector, SpatialDim, Frame::Inertial>&
        inverse_spatial_metric,
    const Scalar<DataVector>& psi, const Scalar<DataVector>& pi,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>& phi,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>&
        unit_normal_one_form) noexcept {
  auto char_fields =
      make_with_value<Variables<tmpl::list<Tags::VPsi, Tags::VZero<SpatialDim>,
                                           Tags::VPlus, Tags::VMinus>>>(
          get(gamma_2), std::numeric_limits<double>::signaling_NaN());
  characteristic_fields(make_not_null(&char_fields), gamma_2,
                        inverse_spatial_metric, psi, pi, phi,
                        unit_normal_one_form);
  return char_fields;
}

template <size_t SpatialDim>
void evolved_fields_from_characteristic_fields(
    const gsl::not_null<Variables<tmpl::list<Psi, Pi, Phi<SpatialDim>>>*>
        evolved_fields,
    const Scalar<DataVector>& gamma_2, const Scalar<DataVector>& v_psi,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>& v_zero,
    const Scalar<DataVector>& v_plus, const Scalar<DataVector>& v_minus,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>&
        unit_normal_one_form) noexcept {
  if (UNLIKELY(get_size(get(get<Psi>(*evolved_fields))) !=
               get_size(get(gamma_2)))) {
    *evolved_fields =
        Variables<tmpl::list<Psi, Pi, Phi<SpatialDim>>>(get_size(get(gamma_2)));
  }
  // Eq.(36) of Holst+ (2005) for Psi
  get<Psi>(*evolved_fields) = v_psi;

  // Eq.(37) - (38) of Holst+ (2004) for Pi and Phi
  get<Pi>(*evolved_fields).get() =
      0.5 * (get(v_plus) + get(v_minus)) + get(gamma_2) * get(v_psi);
  for (size_t i = 0; i < SpatialDim; ++i) {
    get<Phi<SpatialDim>>(*evolved_fields).get(i) =
        0.5 * (get(v_plus) - get(v_minus)) * unit_normal_one_form.get(i) +
        v_zero.get(i);
  }
}

template <size_t SpatialDim>
Variables<tmpl::list<Psi, Pi, Phi<SpatialDim>>>
evolved_fields_from_characteristic_fields(
    const Scalar<DataVector>& gamma_2, const Scalar<DataVector>& v_psi,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>& v_zero,
    const Scalar<DataVector>& v_plus, const Scalar<DataVector>& v_minus,
    const tnsr::i<DataVector, SpatialDim, Frame::Inertial>&
        unit_normal_one_form) noexcept {
  auto evolved_fields =
      make_with_value<Variables<tmpl::list<Psi, Pi, Phi<SpatialDim>>>>(
          get(gamma_2), std::numeric_limits<double>::signaling_NaN());
  evolved_fields_from_characteristic_fields(make_not_null(&evolved_fields),
                                            gamma_2, v_psi, v_zero, v_plus,
                                            v_minus, unit_normal_one_form);
  return evolved_fields;
}

template <size_t SpatialDim>
double ComputeLargestCharacteristicSpeed<SpatialDim>::apply(
    const std::array<DataVector, 4>& char_speeds) noexcept {
  std::array<double, 4> max_speeds{
      {max(abs(char_speeds[0])), max(abs(char_speeds[1])),
       max(abs(char_speeds[2])), max(abs(char_speeds[3]))}};
  return *std::max_element(max_speeds.begin(), max_speeds.end());
}
}  // namespace CurvedScalarWave

#define DIM(data) BOOST_PP_TUPLE_ELEM(0, data)

#define INSTANTIATE(_, data)                                                  \
  template void CurvedScalarWave::characteristic_speeds(                      \
      const gsl::not_null<std::array<DataVector, 4>*> char_speeds,            \
      const Scalar<DataVector>& gamma_1, const Scalar<DataVector>& lapse,     \
      const tnsr::I<DataVector, DIM(data), Frame::Inertial>& shift,           \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>&                  \
          unit_normal_one_form) noexcept;                                     \
  template std::array<DataVector, 4> CurvedScalarWave::characteristic_speeds( \
      const Scalar<DataVector>& gamma_1, const Scalar<DataVector>& lapse,     \
      const tnsr::I<DataVector, DIM(data), Frame::Inertial>& shift,           \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>&                  \
          unit_normal_one_form) noexcept;                                     \
  template struct CurvedScalarWave::CharacteristicSpeedsCompute<DIM(data)>;   \
  template void CurvedScalarWave::characteristic_fields(                      \
      const gsl::not_null<Variables<tmpl::list<                               \
          CurvedScalarWave::Tags::VPsi,                                       \
          CurvedScalarWave::Tags::VZero<DIM(data)>,                           \
          CurvedScalarWave::Tags::VPlus, CurvedScalarWave::Tags::VMinus>>*>   \
          char_fields,                                                        \
      const Scalar<DataVector>& gamma_2,                                      \
      const tnsr::II<DataVector, DIM(data), Frame::Inertial>&                 \
          inverse_spatial_metric,                                             \
      const Scalar<DataVector>& psi, const Scalar<DataVector>& pi,            \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>& phi,             \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>&                  \
          unit_normal_one_form) noexcept;                                     \
  template Variables<tmpl::list<                                              \
      CurvedScalarWave::Tags::VPsi, CurvedScalarWave::Tags::VZero<DIM(data)>, \
      CurvedScalarWave::Tags::VPlus, CurvedScalarWave::Tags::VMinus>>         \
  CurvedScalarWave::characteristic_fields(                                    \
      const Scalar<DataVector>& gamma_2,                                      \
      const tnsr::II<DataVector, DIM(data), Frame::Inertial>&                 \
          inverse_spatial_metric,                                             \
      const Scalar<DataVector>& psi, const Scalar<DataVector>& pi,            \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>& phi,             \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>&                  \
          unit_normal_one_form) noexcept;                                     \
  template struct CurvedScalarWave::CharacteristicFieldsCompute<DIM(data)>;   \
  template void CurvedScalarWave::evolved_fields_from_characteristic_fields(  \
      const gsl::not_null<                                                    \
          Variables<tmpl::list<CurvedScalarWave::Psi, CurvedScalarWave::Pi,   \
                               CurvedScalarWave::Phi<DIM(data)>>>*>           \
          evolved_fields,                                                     \
      const Scalar<DataVector>& gamma_2, const Scalar<DataVector>& v_psi,     \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>& v_zero,          \
      const Scalar<DataVector>& v_plus, const Scalar<DataVector>& v_minus,    \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>&                  \
          unit_normal_one_form) noexcept;                                     \
  template Variables<tmpl::list<CurvedScalarWave::Psi, CurvedScalarWave::Pi,  \
                                CurvedScalarWave::Phi<DIM(data)>>>            \
  CurvedScalarWave::evolved_fields_from_characteristic_fields(                \
      const Scalar<DataVector>& gamma_2, const Scalar<DataVector>& v_psi,     \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>& v_zero,          \
      const Scalar<DataVector>& v_plus, const Scalar<DataVector>& v_minus,    \
      const tnsr::i<DataVector, DIM(data), Frame::Inertial>&                  \
          unit_normal_one_form) noexcept;                                     \
  template struct CurvedScalarWave::                                          \
      EvolvedFieldsFromCharacteristicFieldsCompute<DIM(data)>;                \
  template struct CurvedScalarWave::ComputeLargestCharacteristicSpeed<DIM(    \
      data)>;

GENERATE_INSTANTIATIONS(INSTANTIATE, (1, 2, 3))

#undef INSTANTIATE
#undef DIM
