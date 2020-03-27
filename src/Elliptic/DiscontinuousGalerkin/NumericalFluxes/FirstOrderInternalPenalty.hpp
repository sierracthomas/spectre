// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>
#include <pup.h>
#include <string>

#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataBox/Prefixes.hpp"
#include "DataStructures/DataBox/Tag.hpp"
#include "DataStructures/Tensor/Metafunctions.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/Variables.hpp"
#include "Domain/FaceNormal.hpp"
#include "Domain/Tags.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/NormalDotFlux.hpp"
#include "NumericalAlgorithms/LinearOperators/Divergence.hpp"
#include "Options/Options.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/TMPL.hpp"

/// \cond
class DataVector;
namespace Tags {
template <typename>
struct Normalized;
}  // namespace Tags
/// \endcond

namespace elliptic {
namespace dg {
namespace NumericalFluxes {

/*!
 * \ingroup DiscontinuousGalerkinGroup
 * \ingroup NumericalFluxesGroup
 * \brief The internal penalty flux for first-order elliptic equations.
 *
 * \details Computes the internal penalty numerical flux (see e.g.
 * \cite HesthavenWarburton, section 7.2) dotted with the interface unit normal.
 *
 * We implement here a suggested generalization of the internal penalty flux
 * for any set of elliptic PDEs up to second order in derivatives. It is
 * designed for fluxes (i.e. principal parts of the PDEs) that may depend on the
 * dynamic fields, but do so at most linearly. This is the case for the velocity
 * potential equation of binary neutron stars, for example. The linearity is
 * only necessary to impose inhomogeneous boundary conditions as contributions
 * to the fixed source (see
 * `elliptic::dg::Actions::ImposeInhomogeneousBoundaryConditionsOnSource`).
 *
 * We reduce the second-order elliptic PDEs to first-order form by introducing
 * an _auxiliary_ variable \f$v\f$ for each _primal_ variable \f$u\f$ (i.e. for
 * each variable whose second derivative appears in the equations). Then, the
 * equations take the _flux-form_
 *
 * \f{align}
 * -\partial_i F^i_A + S_A = f_A
 * \f}
 *
 * where the fluxes \f$F^i_A(u,v)\f$ and sources \f$S_A(u,v)\f$ are indexed
 * (with capital letters) by the variables and we have defined \f$f_A(x)\f$ as
 * those sources that are independent of the variables. Note that the fluxes are
 * functions of the primal and auxiliary variables; e.g. for a Poisson system
 * \f$\{u,v_i\}\f$ on a spatial metric \f$\gamma_{ij}\f$ they are simply
 * \f$F^i_u(v)=\sqrt{\gamma}\gamma^{ij} v_j\f$ and
 * \f$F^i_{v_j}(u)=u\delta^i_j\f$ (see `Poisson::FirstOrderSystem`) or for an
 * Elasticity system \f$\{\xi^i,S_{ij}\}\f$ with Young's tensor
 * \f$Y^{ijkl}\f$ they are \f$F^i_{\xi^j}(S)=Y^{ijkl}S_{kl}\f$ and
 * \f$F^i_{S_{jk}}(\xi)=\delta^i_{(j}\xi_{k)}\f$. Since each primal flux is
 * commonly only a function of the corresponding auxiliary variable and
 * vice-versa, we omit the other function arguments here to lighten the
 * notational load.
 *
 * We now choose the internal penalty numerical fluxes \f$(n_i F^i_A)^*\f$ as
 * follows for each primal variable \f$u\f$ and their corresponding auxiliary
 * variable \f$v\f$:
 *
 * \f{align}
 * (n_i F^i_u)^* &= \frac{1}{2} n_i \left( F^i_u(\partial_j
 * F^j_v(u^\mathrm{int})) + F^i_u(\partial_j F^j_v(u^\mathrm{ext}))
 * \right) - \sigma n_i \left(F^i_u(n_j F^j_v(u^\mathrm{int})) - F^i_u(
 * n_j F^j_v(u^\mathrm{ext}))\right) \\
 * (n_i F^i_v)^* &= \frac{1}{2} n_i \left(F^i_v(u^\mathrm{int}) +
 * F^i_v(u^\mathrm{ext})\right)
 * \f}
 *
 * Note that we have assumed \f$n^\mathrm{ext}_i=-n_i\f$ here, i.e. face normals
 * don't depend on the dynamic variables (which may be discontinuous on element
 * faces). This is the case for the problems we are expecting to solve, because
 * those will be on fixed background metrics (e.g. a conformal metric for the
 * XCTS system).
 *
 * Also note that the numerical fluxes intentionally don't depend on the
 * auxiliary field values \f$v\f$. This property allows us to use the numerical
 * fluxes also for the second-order (or _primal_) DG formulation, where we
 * remove the need for an auxiliary variable. For the first-order system we
 * could replace the divergence in \f$(n_i F^i_u)^*\f$ with \f$v\f$, which would
 * result in a generalized "stabilized central flux" that is slightly less
 * sparse than the internal penalty flux (see e.g. \cite HesthavenWarburton,
 * section 7.2). We could also choose to ignore the fluxes in the penalty term,
 * but preliminary tests suggest that this may hurt convergence.
 *
 * For a Poisson system (see above) this numeric flux reduces to the standard
 * internal penalty flux (see e.g. \cite HesthavenWarburton, section 7.2, or
 * \cite Arnold2002)
 *
 * \f{align}
 * (n_i F^i_u)^* &= n_i v_i^* = \frac{1}{2} n_i \left(\partial_i u^\mathrm{int}
 * + \partial_i u^\mathrm{ext}\right) - \sigma \left(u^\mathrm{int} -
 * u^\mathrm{ext}\right) \\
 * (n_i F^i_{v_j})^* &= n_j u^* = \frac{1}{2} n_j \left(u^\mathrm{int} +
 * u^\mathrm{ext}\right)
 * \f}
 *
 * where a sum over repeated indices is assumed, since the equation is
 * formulated on a Euclidean geometry.
 *
 * This generalization of the internal penalty flux is based on unpublished work
 * by Nils L. Fischer (nils.fischer@aei.mpg.de).
 *
 * The penalty factor \f$\sigma\f$ is responsible for removing zero eigenmodes
 * and impacts the conditioning of the linear operator to be solved. It can be
 * chosen as \f$\sigma=C\frac{N_\mathrm{points}^2}{h}\f$ where
 * \f$N_\mathrm{points}\f$ is the number of collocation points (i.e. the
 * polynomial degree plus 1), \f$h\f$ is a measure of the element size in
 * inertial coordinates (both measured perpendicular to the interface under
 * consideration) and \f$C\geq 1\f$ is a free parameter (see e.g.
 * \cite HesthavenWarburton, section 7.2).
 */
template <size_t Dim, typename FluxesComputerTag, typename FieldTagsList,
          typename AuxiliaryFieldTagsList>
struct FirstOrderInternalPenalty;

template <size_t Dim, typename FluxesComputerTag, typename... FieldTags,
          typename... AuxiliaryFieldTags>
struct FirstOrderInternalPenalty<Dim, FluxesComputerTag,
                                 tmpl::list<FieldTags...>,
                                 tmpl::list<AuxiliaryFieldTags...>> {
 private:
  using fluxes_computer_tag = FluxesComputerTag;
  using FluxesComputer = db::const_item_type<fluxes_computer_tag>;

  template <typename Tag>
  struct NormalDotDivFlux : db::PrefixTag, db::SimpleTag {
    using tag = Tag;
    using type = TensorMetafunctions::remove_first_index<db::item_type<Tag>>;
  };

  template <typename Tag>
  struct NormalDotNormalDotFlux : db::PrefixTag, db::SimpleTag {
    using tag = Tag;
    using type = TensorMetafunctions::remove_first_index<db::item_type<Tag>>;
  };

 public:
  struct PenaltyParameter {
    using type = double;
    // Currently this is used as the full prefactor `sigma` to the penalty term.
    // This means it needs to be chosen large enough so that the scheme is
    // stable everywhere. A good estimate is the the largest
    // `sigma > N_points^2 / h` (see class documentation) over all elements in
    // the domain. Choosing `sigma` the same everywhere means it is an
    // overestimate on non-uniform meshes where elements are large or polynomial
    // degrees are small, but this only affects the conditioning of the scheme,
    // i.e. it will converge slower but to the same solution. When it becomes
    // possible to communicate non-tensors (the element size and the number of
    // collocation points on either side of the mortar), this option will be
    // changed to be just the free parameter `C` multiplying `N_points^2 / h`.
    // This will improve conditioning on non-uniform meshes. Currently, the
    // `packaged_data` is a `Variables` which can only hold tensors.
    static constexpr OptionString help = {
        "The prefactor to the penalty term of the flux."};
  };
  using options = tmpl::list<PenaltyParameter>;
  static constexpr OptionString help = {
      "The internal penalty flux for elliptic systems."};
  static std::string name() noexcept { return "InternalPenalty"; }

  FirstOrderInternalPenalty() = default;
  explicit FirstOrderInternalPenalty(const double penalty_parameter) noexcept
      : penalty_parameter_(penalty_parameter) {}

  // clang-tidy: non-const reference
  void pup(PUP::er& p) noexcept { p | penalty_parameter_; }  // NOLINT

  // These tags are sliced to the interface of the element and passed to
  // `package_data` to provide the data needed to compute the numerical fluxes.
  using argument_tags = tmpl::push_front<
      typename FluxesComputer::argument_tags,
      ::Tags::NormalDotFlux<AuxiliaryFieldTags>...,
      ::Tags::div<::Tags::Flux<AuxiliaryFieldTags, tmpl::size_t<Dim>,
                               Frame::Inertial>>...,
      ::Tags::Normalized<domain::Tags::UnnormalizedFaceNormal<Dim>>,
      fluxes_computer_tag>;
  using volume_tags =
      tmpl::push_front<get_volume_tags<FluxesComputer>, fluxes_computer_tag>;

  // This is the data needed to compute the numerical flux.
  // `SendBoundaryFluxes` calls `package_data` to store these tags in a
  // Variables. Local and remote values of this data are then combined in the
  // `()` operator.
  using package_tags =
      tmpl::list<::Tags::NormalDotFlux<AuxiliaryFieldTags>...,
                 NormalDotDivFlux<AuxiliaryFieldTags>...,
                 NormalDotNormalDotFlux<AuxiliaryFieldTags>...>;

  // Following the packaged_data pointer, this function expects as arguments the
  // types in `argument_tags`.
  template <typename... FluxesArgs>
  void package_data(
      const gsl::not_null<Variables<package_tags>*> packaged_data,
      const db::const_item_type<::Tags::NormalDotFlux<
          AuxiliaryFieldTags>>&... normal_dot_auxiliary_field_fluxes,
      const db::const_item_type<::Tags::div<
          ::Tags::Flux<AuxiliaryFieldTags, tmpl::size_t<Dim>,
                       Frame::Inertial>>>&... div_auxiliary_field_fluxes,
      const tnsr::i<DataVector, Dim, Frame::Inertial>& interface_unit_normal,
      const FluxesComputer& fluxes_computer,
      const FluxesArgs&... fluxes_args) const noexcept {
    auto principal_div_aux_field_fluxes = make_with_value<Variables<tmpl::list<
        ::Tags::Flux<FieldTags, tmpl::size_t<Dim>, Frame::Inertial>...>>>(
        interface_unit_normal, std::numeric_limits<double>::signaling_NaN());
    auto principal_ndot_aux_field_fluxes = make_with_value<Variables<tmpl::list<
        ::Tags::Flux<FieldTags, tmpl::size_t<Dim>, Frame::Inertial>...>>>(
        interface_unit_normal, std::numeric_limits<double>::signaling_NaN());
    fluxes_computer.apply(
        make_not_null(
            &get<::Tags::Flux<FieldTags, tmpl::size_t<Dim>, Frame::Inertial>>(
                principal_div_aux_field_fluxes))...,
        fluxes_args..., div_auxiliary_field_fluxes...);
    fluxes_computer.apply(
        make_not_null(
            &get<::Tags::Flux<FieldTags, tmpl::size_t<Dim>, Frame::Inertial>>(
                principal_ndot_aux_field_fluxes))...,
        fluxes_args..., normal_dot_auxiliary_field_fluxes...);
    const auto apply_normal_dot =
        [
          &packaged_data, &principal_div_aux_field_fluxes,
          &principal_ndot_aux_field_fluxes, &interface_unit_normal
        ](auto field_tag_v, auto auxiliary_field_tag_v,
          const auto& normal_dot_auxiliary_field_flux) noexcept {
      using field_tag = decltype(field_tag_v);
      using auxiliary_field_tag = decltype(auxiliary_field_tag_v);
      // Compute n.F_v(u)
      get<::Tags::NormalDotFlux<auxiliary_field_tag>>(*packaged_data) =
          normal_dot_auxiliary_field_flux;
      // Compute n.F_u(div(F_v(u))) and n.F_u(n.F_v(u))
      normal_dot_flux(
          make_not_null(
              &get<NormalDotDivFlux<auxiliary_field_tag>>(*packaged_data)),
          interface_unit_normal,
          get<::Tags::Flux<field_tag, tmpl::size_t<Dim>, Frame::Inertial>>(
              principal_div_aux_field_fluxes));
      normal_dot_flux(
          make_not_null(&get<NormalDotNormalDotFlux<auxiliary_field_tag>>(
              *packaged_data)),
          interface_unit_normal,
          get<::Tags::Flux<field_tag, tmpl::size_t<Dim>, Frame::Inertial>>(
              principal_ndot_aux_field_fluxes));
    };
    EXPAND_PACK_LEFT_TO_RIGHT(apply_normal_dot(
        FieldTags{}, AuxiliaryFieldTags{}, normal_dot_auxiliary_field_fluxes));
  }

  // This function combines local and remote data to the numerical fluxes.
  // The numerical fluxes as not-null pointers are the first arguments. The
  // other arguments are the packaged types for the interior side followed by
  // the packaged types for the exterior side.
  void operator()(
      const gsl::not_null<db::item_type<::Tags::NormalDotNumericalFlux<
          FieldTags>>*>... numerical_flux_for_fields,
      const gsl::not_null<db::item_type<::Tags::NormalDotNumericalFlux<
          AuxiliaryFieldTags>>*>... numerical_flux_for_auxiliary_fields,
      const db::item_type<::Tags::NormalDotFlux<
          AuxiliaryFieldTags>>&... normal_dot_auxiliary_flux_interiors,
      const db::item_type<NormalDotDivFlux<
          AuxiliaryFieldTags>>&... normal_dot_div_auxiliary_flux_interiors,
      const db::item_type<NormalDotNormalDotFlux<
          AuxiliaryFieldTags>>&... ndot_ndot_aux_flux_interiors,
      const db::item_type<::Tags::NormalDotFlux<
          AuxiliaryFieldTags>>&... minus_normal_dot_auxiliary_flux_exteriors,
      const db::item_type<NormalDotDivFlux<
          AuxiliaryFieldTags>>&... minus_normal_dot_div_aux_flux_exteriors,
      const db::item_type<NormalDotDivFlux<
          AuxiliaryFieldTags>>&... ndot_ndot_aux_flux_exteriors) const
      noexcept {
    // Need polynomial degrees and element size to compute this dynamically
    const double penalty = penalty_parameter_;

    const auto assemble_numerical_fluxes = [&penalty](
        const auto numerical_flux_for_field,
        const auto numerical_flux_for_auxiliary_field,
        const auto& normal_dot_auxiliary_flux_interior,
        const auto& normal_dot_div_auxiliary_flux_interior,
        const auto& ndot_ndot_aux_flux_interior,
        const auto& minus_normal_dot_auxiliary_flux_exterior,
        const auto& minus_normal_dot_div_aux_flux_exterior,
        const auto& ndot_ndot_aux_flux_exterior) noexcept {
      for (auto it = numerical_flux_for_auxiliary_field->begin();
           it != numerical_flux_for_auxiliary_field->end(); it++) {
        const auto index =
            numerical_flux_for_auxiliary_field->get_tensor_index(it);
        // We are working with the n.F_v(u) computed on either side of the
        // interface, so (assuming the normal is independent of the dynamic
        // variables) the data we get from the other element contains _minus_
        // the normal from this element. So we cancel the minus sign when
        // computing the average here.
        *it = 0.5 * (normal_dot_auxiliary_flux_interior.get(index) -
                     minus_normal_dot_auxiliary_flux_exterior.get(index));
      }
      for (auto it = numerical_flux_for_field->begin();
           it != numerical_flux_for_field->end(); it++) {
        const auto index = numerical_flux_for_field->get_tensor_index(it);
        *it = 0.5 * (normal_dot_div_auxiliary_flux_interior.get(index) -
                     minus_normal_dot_div_aux_flux_exterior.get(index)) -
              penalty * (ndot_ndot_aux_flux_interior.get(index) -
                         ndot_ndot_aux_flux_exterior.get(index));
      }
    };
    EXPAND_PACK_LEFT_TO_RIGHT(assemble_numerical_fluxes(
        numerical_flux_for_fields, numerical_flux_for_auxiliary_fields,
        normal_dot_auxiliary_flux_interiors,
        normal_dot_div_auxiliary_flux_interiors, ndot_ndot_aux_flux_interiors,
        minus_normal_dot_auxiliary_flux_exteriors,
        minus_normal_dot_div_aux_flux_exteriors, ndot_ndot_aux_flux_exteriors));
  }

  // This function computes the boundary contributions from Dirichlet boundary
  // conditions. This data is what remains to be added to the boundaries when
  // homogeneous (i.e. zero) boundary conditions are assumed in the calculation
  // of the numerical fluxes, but we wish to impose inhomogeneous (i.e. nonzero)
  // boundary conditions. Since this contribution does not depend on the
  // numerical field values, but only on the Dirichlet boundary data, it may be
  // added as contribution to the source of the elliptic systems. Then, it
  // remains to solve the homogeneous problem with the modified source.
  template <typename... FluxesArgs>
  void compute_dirichlet_boundary(
      const gsl::not_null<db::item_type<::Tags::NormalDotNumericalFlux<
          FieldTags>>*>... numerical_flux_for_fields,
      const gsl::not_null<db::item_type<::Tags::NormalDotNumericalFlux<
          AuxiliaryFieldTags>>*>... numerical_flux_for_auxiliary_fields,
      const db::const_item_type<FieldTags>&... dirichlet_fields,
      const tnsr::i<DataVector, Dim, Frame::Inertial>& interface_unit_normal,
      const FluxesComputer& fluxes_computer,
      const FluxesArgs&... fluxes_args) const noexcept {
    // Need polynomial degrees and element size to compute this dynamically
    const double penalty = penalty_parameter_;

    // Compute n.F_v(u)
    auto dirichlet_auxiliary_field_fluxes =
        make_with_value<Variables<tmpl::list<::Tags::Flux<
            AuxiliaryFieldTags, tmpl::size_t<Dim>, Frame::Inertial>...>>>(
            interface_unit_normal,
            std::numeric_limits<double>::signaling_NaN());
    fluxes_computer.apply(
        make_not_null(&get<::Tags::Flux<AuxiliaryFieldTags, tmpl::size_t<Dim>,
                                        Frame::Inertial>>(
            dirichlet_auxiliary_field_fluxes))...,
        fluxes_args..., dirichlet_fields...);
    const auto apply_normal_dot_aux =
        [&interface_unit_normal, &dirichlet_auxiliary_field_fluxes ](
            auto auxiliary_field_tag_v,
            const auto numerical_flux_for_auxiliary_field) noexcept {
      using auxiliary_field_tag = decltype(auxiliary_field_tag_v);
      normal_dot_flux(
          numerical_flux_for_auxiliary_field, interface_unit_normal,
          get<::Tags::Flux<auxiliary_field_tag, tmpl::size_t<Dim>,
                           Frame::Inertial>>(dirichlet_auxiliary_field_fluxes));
    };
    EXPAND_PACK_LEFT_TO_RIGHT(apply_normal_dot_aux(
        AuxiliaryFieldTags{}, numerical_flux_for_auxiliary_fields));

    // Compute 2 * sigma * n.F_u(n.F_v(u))
    auto principal_dirichlet_auxiliary_field_fluxes =
        make_with_value<Variables<tmpl::list<
            ::Tags::Flux<FieldTags, tmpl::size_t<Dim>, Frame::Inertial>...>>>(
            interface_unit_normal,
            std::numeric_limits<double>::signaling_NaN());
    fluxes_computer.apply(
        make_not_null(
            &get<::Tags::Flux<FieldTags, tmpl::size_t<Dim>, Frame::Inertial>>(
                principal_dirichlet_auxiliary_field_fluxes))...,
        fluxes_args..., *numerical_flux_for_auxiliary_fields...);
    const auto assemble_dirichlet_penalty = [
      &interface_unit_normal, &penalty,
      &principal_dirichlet_auxiliary_field_fluxes
    ](auto field_tag_v, const auto numerical_flux_for_field) noexcept {
      using field_tag = decltype(field_tag_v);
      normal_dot_flux(
          numerical_flux_for_field, interface_unit_normal,
          get<::Tags::Flux<field_tag, tmpl::size_t<Dim>, Frame::Inertial>>(
              principal_dirichlet_auxiliary_field_fluxes));
      for (auto it = numerical_flux_for_field->begin();
           it != numerical_flux_for_field->end(); it++) {
        *it *= 2 * penalty;
      }
    };
    EXPAND_PACK_LEFT_TO_RIGHT(
        assemble_dirichlet_penalty(FieldTags{}, numerical_flux_for_fields));
  }

 private:
  double penalty_parameter_ = std::numeric_limits<double>::signaling_NaN();
};

}  // namespace NumericalFluxes
}  // namespace dg
}  // namespace elliptic
