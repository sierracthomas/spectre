// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines Time and TimeDelta

#pragma once

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <limits>

#include "Time/Slab.hpp"
#include "Utilities/Rational.hpp"

/// \cond
namespace PUP {
class er;
}  // namespace PUP
class TimeDelta;
/// \endcond

/// \ingroup TimeGroup
///
/// The time in a simulation.  Times can be safely compared for exact
/// equality as long as they do not belong to overlapping unequal
/// slabs.
class Time {
 public:
  using rational_t = Rational;

  /// Default constructor gives an invalid Time.
  Time() noexcept = default;

  /// A time a given fraction of the way through the given slab.
  Time(Slab slab, rational_t fraction) noexcept;

  /// Move the time to a different slab.  The time must be at an end
  /// of the current slab and the new slab must share that endpoint.
  Time with_slab(const Slab& new_slab) const noexcept;

  /// Approximate numerical value of the Time.
  double value() const noexcept { return value_; }
  const Slab& slab() const noexcept { return slab_; }
  const rational_t& fraction() const noexcept { return fraction_; }

  Time& operator+=(const TimeDelta& delta) noexcept;
  Time& operator-=(const TimeDelta& delta) noexcept;

  bool is_at_slab_start() const noexcept;
  bool is_at_slab_end() const noexcept;
  bool is_at_slab_boundary() const noexcept;

  // clang-tidy: google-runtime-references
  void pup(PUP::er& p) noexcept;  // NOLINT

  /// A comparison operator that compares Times structurally, i.e.,
  /// just looking at the class members.  This is only intended for
  /// use as the comparator in a map.  The returned ordering does not
  /// match the time ordering and opposite sides of slab boundaries do
  /// not compare equal.  It is, however, much faster to compute than
  /// the temporal ordering, so it is useful when an ordering is
  /// required, but the ordering does not have to be physically
  /// meaningful.
  struct StructuralCompare {
    bool operator()(const Time& a, const Time& b) const;
  };

 private:
  Slab slab_;
  rational_t fraction_;
  double value_ = std::numeric_limits<double>::signaling_NaN();

  // The value is precomputed so that we can avoid doing the rational
  // math repeatedly.  The value of a Time should almost always be
  // needed at some point.
  void compute_value() noexcept;

  void range_check() const noexcept;

  friend class TimeDelta;
};

/// \ingroup TimeGroup
///
/// Represents an interval of time within a single slab.
class TimeDelta {
 public:
  using rational_t = Time::rational_t;

  /// Default constructor gives an invalid TimeDelta.
  TimeDelta() noexcept = default;

  /// An interval covering a given fraction of the slab.
  TimeDelta(Slab slab, rational_t fraction) noexcept;

  /// Move the interval to a different slab.  The resulting interval
  /// will in general not be the same length, but will take up the
  /// same fraction of its slab.
  TimeDelta with_slab(const Slab& new_slab) const noexcept;

  Slab slab() const noexcept { return slab_; }
  rational_t fraction() const noexcept { return fraction_; }

  /// Approximate numerical length of the interval.
  double value() const noexcept;

  /// Test if the interval is oriented towards larger time.
  bool is_positive() const noexcept;

  TimeDelta& operator+=(const TimeDelta& other) noexcept;
  TimeDelta& operator-=(const TimeDelta& other) noexcept;
  TimeDelta operator+() const noexcept;
  TimeDelta operator-() const noexcept;
  TimeDelta& operator*=(const rational_t& mult) noexcept;
  TimeDelta& operator/=(const rational_t& div) noexcept;

  // clang-tidy: google-runtime-references
  void pup(PUP::er& p) noexcept;  // NOLINT

 private:
  Slab slab_;
  rational_t fraction_;

  friend class Time;
};

// Time <cmp> Time
// clang-tidy: clang-tidy wants this removed in favor of friend
// declaration in different header.
bool operator==(const Time& a, const Time& b) noexcept;  // NOLINT
bool operator!=(const Time& a, const Time& b) noexcept;
bool operator<(const Time& a, const Time& b) noexcept;
bool operator>(const Time& a, const Time& b) noexcept;
bool operator<=(const Time& a, const Time& b) noexcept;
bool operator>=(const Time& a, const Time& b) noexcept;

// TimeDelta <cmp> TimeDelta
bool operator==(const TimeDelta& a, const TimeDelta& b) noexcept;
bool operator!=(const TimeDelta& a, const TimeDelta& b) noexcept;
bool operator<(const TimeDelta& a, const TimeDelta& b) noexcept;
bool operator>(const TimeDelta& a, const TimeDelta& b) noexcept;
bool operator<=(const TimeDelta& a, const TimeDelta& b) noexcept;
bool operator>=(const TimeDelta& a, const TimeDelta& b) noexcept;

// Time <op> Time
TimeDelta operator-(const Time& a, const Time& b) noexcept;

// Time <op> TimeDelta, TimeDelta <op> Time
Time operator+(Time a, const TimeDelta& b) noexcept;
Time operator+(const TimeDelta& a, Time b) noexcept;
Time operator-(Time a, const TimeDelta& b) noexcept;

// TimeDelta <op> TimeDelta
TimeDelta operator+(TimeDelta a, const TimeDelta& b) noexcept;
TimeDelta operator-(TimeDelta a, const TimeDelta& b) noexcept;

// This returns a double rather than a rational so we can compare dt
// in different slabs.
double operator/(const TimeDelta& a, const TimeDelta& b) noexcept;

// rational <op> TimeDelta, TimeDelta <op> rational
TimeDelta operator*(TimeDelta a, const TimeDelta::rational_t& b) noexcept;
TimeDelta operator*(const TimeDelta::rational_t& a, TimeDelta b) noexcept;
TimeDelta operator/(TimeDelta a, const TimeDelta::rational_t& b) noexcept;

// Miscellaneous other functions for Time

std::ostream& operator<<(std::ostream& os, const Time& t) noexcept;

size_t hash_value(const Time& t) noexcept;

namespace std {
template <>
struct hash<Time> {
  size_t operator()(const Time& t) const noexcept;
};
}  // namespace std

// Miscellaneous other functions for TimeDelta

TimeDelta abs(TimeDelta t) noexcept;

std::ostream& operator<<(std::ostream& os, const TimeDelta& dt) noexcept;
