// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Time/TimeSteppers/DormandPrince5.hpp"

#include <cmath>
#include <limits>

#include "ErrorHandling/Assert.hpp"
#include "Time/TimeStepId.hpp"
#include "Utilities/Gsl.hpp"

namespace TimeSteppers {

uint64_t DormandPrince5::number_of_substeps() const noexcept { return 6; }

size_t DormandPrince5::number_of_past_steps() const noexcept { return 0; }

// The growth function for DP5 is
//
//   g = mu^6 / 600 + \sum_{n=0}^5 mu^n / n!,
//
// where mu = lambda * dt. The equation dy/dt = -lambda * y evolves
// stably if |g| < 1. For lambda=-2, chosen so the stable_step() for
// RK1 (i.e. forward Euler) would be 1, DP5 has a stable step
// determined by inserting mu->-2 dt into the above equation. Finding the
// solutions with a numerical root find yields a stable step of about 1.653.
double DormandPrince5::stable_step() const noexcept {
  return 1.6532839463174733;
}

TimeStepId DormandPrince5::next_time_id(const TimeStepId& current_id,
                                        const TimeDelta& time_step) const
    noexcept {
  const auto& step = current_id.substep();
  const auto& t0 = current_id.step_time();
  const auto& t = current_id.substep_time();
  if (step < 6) {
    if (step == 0) {
      ASSERT(t == t0, "In DP5 substep 0, the substep time ("
                          << t << ") should equal t0 (" << t0 << ")");
    } else {
      ASSERT(t == t0 + gsl::at(_c, step - 1) * time_step,
             "In DP5 substep " << step << ", the substep time (" << t
                               << ") should equal t0+c[" << step - 1 << "]*dt ("
                               << t0 + gsl::at(_c, step - 1) * time_step
                               << ")");
    }
    if (step < 5) {
      return {current_id.time_runs_forward(), current_id.slab_number(), t0,
              step + 1, t0 + gsl::at(_c, step) * time_step};
    } else {
      return {current_id.time_runs_forward(), current_id.slab_number(),
              t0 + time_step};
    }
  } else {
    ERROR("In DP5 substep should be one of 0,1,2,3,4,5, not "
          << current_id.substep());
  }
}

constexpr double DormandPrince5::_a2;
constexpr std::array<double, 2> DormandPrince5::_a3;
constexpr std::array<double, 3> DormandPrince5::_a4;
constexpr std::array<double, 4> DormandPrince5::_a5;
constexpr std::array<double, 5> DormandPrince5::_a6;
constexpr std::array<double, 6> DormandPrince5::_b;
constexpr std::array<double, 5> DormandPrince5::_c;
constexpr std::array<double, 6> DormandPrince5::_d;
}  // namespace TimeSteppers

/// \cond
PUP::able::PUP_ID TimeSteppers::DormandPrince5::my_PUP_ID =  // NOLINT
    0;
/// \endcond
