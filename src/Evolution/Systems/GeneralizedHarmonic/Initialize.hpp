// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>  // IWYU pragma: keep
#include <vector>

// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <cstddef>
#include <vector>

#include "AlgorithmSingleton.hpp"
#include "ApparentHorizons/ComputeItems.hpp"
#include "ApparentHorizons/Tags.hpp"
#include "DataStructures/DataBox/DataBox.hpp"
#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataBox/PrefixHelpers.hpp"
#include "DataStructures/DataBox/Prefixes.hpp"
#include "DataStructures/DataBox/Tag.hpp"
#include "DataStructures/Tensor/EagerMath/DotProduct.hpp"
#include "DataStructures/Tensor/EagerMath/Norms.hpp"
#include "DataStructures/Variables.hpp"
#include "Domain/Creators/RegisterDerivedWithCharm.hpp"
#include "Domain/Creators/TimeDependence/RegisterDerivedWithCharm.hpp"
#include "Domain/FunctionsOfTime/RegisterDerivedWithCharm.hpp"
#include "Domain/Mesh.hpp"
#include "Domain/Tags.hpp"
#include "Domain/TagsCharacteresticSpeeds.hpp"
#include "ErrorHandling/Assert.hpp"
#include "ErrorHandling/Error.hpp"
#include "ErrorHandling/FloatingPointExceptions.hpp"
#include "Evolution/Actions/AddMeshVelocityNonconservative.hpp"
#include "Evolution/Actions/ComputeTimeDerivative.hpp"
#include "Evolution/ComputeTags.hpp"
#include "Evolution/DiscontinuousGalerkin/DgElementArray.hpp"
#include "Evolution/Initialization/DgDomain.hpp"
#include "Evolution/Initialization/DiscontinuousGalerkin.hpp"
#include "Evolution/Initialization/Evolution.hpp"
#include "Evolution/Initialization/NonconservativeSystem.hpp"
#include "Evolution/Initialization/SetVariables.hpp"
#include "Evolution/NumericInitialData.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/Characteristics.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/Constraints.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/Equations.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/GaugeSourceFunctions/DampedHarmonic.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/GaugeSourceFunctions/InitializeDampedHarmonic.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/Initialize.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/System.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/Tags.hpp"
#include "Evolution/Systems/GeneralizedHarmonic/UpwindPenaltyCorrection.hpp"
#include "Evolution/TypeTraits.hpp"
#include "IO/Importers/ElementActions.hpp"
#include "IO/Importers/VolumeDataReader.hpp"
#include "IO/Observer/Actions.hpp"
#include "IO/Observer/Helpers.hpp"
#include "IO/Observer/ObserverComponent.hpp"
#include "IO/Observer/RegisterObservers.hpp"
#include "IO/Observer/Tags.hpp"
#include "Informer/Tags.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/Actions/ComputeNonconservativeBoundaryFluxes.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/Actions/ImposeBoundaryConditions.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/BoundarySchemes/FirstOrder/FirstOrderScheme.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/BoundarySchemes/FirstOrder/FirstOrderSchemeLts.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/Tags.hpp"
#include "NumericalAlgorithms/Interpolation/AddTemporalIdsToInterpolationTarget.hpp"
#include "NumericalAlgorithms/Interpolation/Callbacks/FindApparentHorizon.hpp"
#include "NumericalAlgorithms/Interpolation/Callbacks/ObserveTimeSeriesOnSurface.hpp"
#include "NumericalAlgorithms/Interpolation/CleanUpInterpolator.hpp"
#include "NumericalAlgorithms/Interpolation/InitializeInterpolationTarget.hpp"
#include "NumericalAlgorithms/Interpolation/Interpolate.hpp"
#include "NumericalAlgorithms/Interpolation/InterpolationTarget.hpp"
#include "NumericalAlgorithms/Interpolation/InterpolationTargetApparentHorizon.hpp"
#include "NumericalAlgorithms/Interpolation/InterpolationTargetReceiveVars.hpp"
#include "NumericalAlgorithms/Interpolation/Interpolator.hpp"
#include "NumericalAlgorithms/Interpolation/InterpolatorReceivePoints.hpp"
#include "NumericalAlgorithms/Interpolation/InterpolatorReceiveVolumeData.hpp"
#include "NumericalAlgorithms/Interpolation/InterpolatorRegisterElement.hpp"
#include "NumericalAlgorithms/Interpolation/Tags.hpp"
#include "NumericalAlgorithms/Interpolation/TryToInterpolate.hpp"
#include "NumericalAlgorithms/LinearOperators/PartialDerivatives.hpp"
#include "NumericalAlgorithms/LinearOperators/PartialDerivatives.tpp"
#include "Options/Options.hpp"
#include "Parallel/Actions/TerminatePhase.hpp"
#include "Parallel/ConstGlobalCache.hpp"
#include "Parallel/InitializationFunctions.hpp"
#include "Parallel/PhaseDependentActionList.hpp"
#include "Parallel/Reduction.hpp"
#include "Parallel/RegisterDerivedClassesWithCharm.hpp"
#include "ParallelAlgorithms/Actions/MutateApply.hpp"
#include "ParallelAlgorithms/DiscontinuousGalerkin/CollectDataForFluxes.hpp"
#include "ParallelAlgorithms/DiscontinuousGalerkin/FluxCommunication.hpp"
#include "ParallelAlgorithms/DiscontinuousGalerkin/InitializeDomain.hpp"
#include "ParallelAlgorithms/DiscontinuousGalerkin/InitializeInterfaces.hpp"
#include "ParallelAlgorithms/DiscontinuousGalerkin/InitializeMortars.hpp"
#include "ParallelAlgorithms/Events/ObserveErrorNorms.hpp"
#include "ParallelAlgorithms/Events/ObserveFields.hpp"
#include "ParallelAlgorithms/EventsAndTriggers/Actions/RunEventsAndTriggers.hpp"
#include "ParallelAlgorithms/EventsAndTriggers/Event.hpp"
#include "ParallelAlgorithms/EventsAndTriggers/EventsAndTriggers.hpp"
#include "ParallelAlgorithms/EventsAndTriggers/Tags.hpp"
#include "ParallelAlgorithms/Initialization/Actions/AddComputeTags.hpp"
#include "ParallelAlgorithms/Initialization/Actions/RemoveOptionsAndTerminatePhase.hpp"
#include "ParallelAlgorithms/Initialization/MergeIntoDataBox.hpp"
#include "PointwiseFunctions/AnalyticData/Tags.hpp"
#include "PointwiseFunctions/AnalyticSolutions/GeneralRelativity/KerrSchild.hpp"
#include "PointwiseFunctions/AnalyticSolutions/GeneralRelativity/WrappedGr.hpp"
#include "PointwiseFunctions/AnalyticSolutions/Tags.hpp"
#include "PointwiseFunctions/GeneralRelativity/Christoffel.hpp"
#include "PointwiseFunctions/GeneralRelativity/DerivativesOfSpacetimeMetric.hpp"
#include "PointwiseFunctions/GeneralRelativity/DetAndInverseSpatialMetric.hpp"
#include "PointwiseFunctions/GeneralRelativity/GeneralizedHarmonic/ConstraintGammas.hpp"
#include "PointwiseFunctions/GeneralRelativity/GeneralizedHarmonic/SpatialDerivOfLapse.hpp"
#include "PointwiseFunctions/GeneralRelativity/GeneralizedHarmonic/SpatialDerivOfShift.hpp"
#include "PointwiseFunctions/GeneralRelativity/GeneralizedHarmonic/TimeDerivOfLapse.hpp"
#include "PointwiseFunctions/GeneralRelativity/GeneralizedHarmonic/TimeDerivOfShift.hpp"
#include "PointwiseFunctions/GeneralRelativity/GeneralizedHarmonic/TimeDerivOfSpatialMetric.hpp"
#include "PointwiseFunctions/GeneralRelativity/IndexManipulation.hpp"
#include "PointwiseFunctions/GeneralRelativity/InverseSpacetimeMetric.hpp"
#include "PointwiseFunctions/GeneralRelativity/Ricci.hpp"
#include "PointwiseFunctions/GeneralRelativity/SpacetimeNormalOneForm.hpp"
#include "PointwiseFunctions/GeneralRelativity/SpatialMetric.hpp"
#include "PointwiseFunctions/GeneralRelativity/Tags.hpp"
#include "PointwiseFunctions/GeneralRelativity/WeylElectric.hpp"
#include "Time/Actions/AdvanceTime.hpp"
#include "Time/Actions/ChangeSlabSize.hpp"
#include "Time/Actions/ChangeStepSize.hpp"
#include "Time/Actions/RecordTimeStepperData.hpp"
#include "Time/Actions/SelfStartActions.hpp"
#include "Time/Actions/UpdateU.hpp"
#include "Time/StepChoosers/Cfl.hpp"
#include "Time/StepChoosers/Constant.hpp"
#include "Time/StepChoosers/Increase.hpp"
#include "Time/StepChoosers/PreventRapidIncrease.hpp"
#include "Time/StepChoosers/StepChooser.hpp"
#include "Time/StepChoosers/StepToTimes.hpp"
#include "Time/StepControllers/StepController.hpp"
#include "Time/Tags.hpp"
#include "Time/TimeSteppers/TimeStepper.hpp"
#include "Time/Triggers/TimeTriggers.hpp"
#include "Utilities/Functional.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/MakeWithValue.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TypeTraits.hpp"
//#include "src/Domain/Tags.hpp"

/// \cond
namespace Parallel {
template <typename Metavariables>
class ConstGlobalCache;
}  // namespace Parallel
/// \endcond

namespace GeneralizedHarmonic {
namespace Actions {
template <size_t Dim>
struct InitializeConstraints {
  using frame = Frame::Inertial;

  template <typename DbTagsList, typename... InboxTags, typename Metavariables,
            typename ArrayIndex, typename ActionList,
            typename ParallelComponent>
  static auto apply(db::DataBox<DbTagsList>& box,
                    const tuples::TaggedTuple<InboxTags...>& /*inboxes*/,
                    const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const ActionList /*meta*/,
                    const ParallelComponent* const /*meta*/) noexcept {
    using compute_tags = tmpl::flatten<db::AddComputeTags<
        GeneralizedHarmonic::Tags::GaugeConstraintCompute<Dim, frame>,
        // following tags added to observe constraints
        ::Tags::PointwiseL2NormCompute<
            GeneralizedHarmonic::Tags::GaugeConstraint<Dim, frame>>,
        ::Tags::PointwiseL2NormCompute<
            GeneralizedHarmonic::Tags::ThreeIndexConstraint<Dim, frame>>,
        // The 4-index constraint is only implemented in 3d
        tmpl::conditional_t<
            Dim == 3,
            tmpl::list<GeneralizedHarmonic::Tags::FourIndexConstraintCompute<
                           Dim, frame>,
                       ::Tags::PointwiseL2NormCompute<
                           GeneralizedHarmonic::Tags::FourIndexConstraint<
                               Dim, frame>>>,
            tmpl::list<>>>>;

    return std::make_tuple(
        Initialization::merge_into_databox<InitializeConstraints,
                                           db::AddSimpleTags<>, compute_tags>(
            std::move(box)));
  }
};

template <size_t Dim>
struct InitializeGhAnd3Plus1Variables {
  using frame = Frame::Inertial;

  template <typename DbTagsList, typename... InboxTags, typename Metavariables,
            typename ArrayIndex, typename ActionList,
            typename ParallelComponent>
  static auto apply(db::DataBox<DbTagsList>& box,
                    const tuples::TaggedTuple<InboxTags...>& /*inboxes*/,
                    const Parallel::ConstGlobalCache<Metavariables>& /*cache*/,
                    const ArrayIndex& /*array_index*/,
                    const ActionList /*meta*/,
                    const ParallelComponent* const /*meta*/) noexcept {
    using compute_tags = db::AddComputeTags<
        gr::Tags::SpatialMetricCompute<Dim, frame, DataVector>,
        gr::Tags::DetAndInverseSpatialMetricCompute<Dim, frame, DataVector>,
        gr::Tags::ShiftCompute<Dim, frame, DataVector>,
        gr::Tags::LapseCompute<Dim, frame, DataVector>,
        gr::Tags::SqrtDetSpatialMetricCompute<Dim, frame, DataVector>,
        gr::Tags::SpacetimeNormalOneFormCompute<Dim, frame, DataVector>,
        gr::Tags::SpacetimeNormalVectorCompute<Dim, frame, DataVector>,
        gr::Tags::InverseSpacetimeMetricCompute<Dim, frame, DataVector>,
        GeneralizedHarmonic::Tags::DerivSpatialMetricCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::DerivLapseCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::DerivShiftCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::TimeDerivSpatialMetricCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::TimeDerivLapseCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::TimeDerivShiftCompute<Dim, frame>,
        gr::Tags::DerivativesOfSpacetimeMetricCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::ThreeIndexConstraintCompute<Dim, frame>,
        gr::Tags::SpacetimeChristoffelFirstKindCompute<Dim, frame, DataVector>,
        gr::Tags::SpacetimeChristoffelSecondKindCompute<Dim, frame, DataVector>,
        gr::Tags::TraceSpacetimeChristoffelFirstKindCompute<Dim, frame,
                                                            DataVector>,
        gr::Tags::SpatialChristoffelFirstKindCompute<Dim, frame, DataVector>,
        gr::Tags::SpatialChristoffelSecondKindVarsCompute<Dim, frame,
                                                          DataVector>,
        ::Tags::DerivCompute<
            ::Tags::Variables<tmpl::list<gr::Tags::SpatialChristoffelSecondKind<
                Dim, frame, DataVector>>>,
            ::Tags::InverseJacobian<::Tags::ElementMap<Dim>,
                                    ::Tags::Coordinates<Dim, Frame::Logical>>>,
        gr::Tags::TraceSpatialChristoffelFirstKindCompute<Dim, frame,
                                                          DataVector>,
        gr::Tags::RicciTensorCompute<Dim, frame, DataVector>,
        GeneralizedHarmonic::Tags::ExtrinsicCurvatureCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::TraceExtrinsicCurvatureCompute<Dim, frame>,
        GeneralizedHarmonic::Tags::ConstraintGamma0Compute<Dim, frame>,
        GeneralizedHarmonic::Tags::ConstraintGamma1Compute<Dim, frame>,
        GeneralizedHarmonic::Tags::ConstraintGamma2Compute<Dim, frame>,
        gr::Tags::WeylElectricCompute<Dim, frame, DataVector>,
        gr::Tags::WeylElectricScalarCompute<Dim, frame, DataVector>>;
    return std::make_tuple(
        Initialization::merge_into_databox<InitializeGhAnd3Plus1Variables,
                                           db::AddSimpleTags<>, compute_tags>(
            std::move(box)));
  }
};

}  // namespace Actions
}  // namespace GeneralizedHarmonic
