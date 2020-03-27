// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "ErrorHandling/Error.hpp"
#include "Framework/TestHelpers.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/SimpleMortarData.hpp"
#include "Utilities/Literals.hpp"  // IWYU pragma: keep

// IWYU pragma: no_include <type_traits>  // for __decay_and_strip<>::__type

SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData", "[Unit][NumericalAlgorithms]") {
  dg::SimpleMortarData<size_t, std::string, double> data;
  data = serialize_and_deserialize(data);
  data.local_insert(0, "string 1");
  data = serialize_and_deserialize(data);
  data.remote_insert(0, 1.234);
  CHECK(data.extract() == std::make_pair("string 1"s, 1.234));
  data = serialize_and_deserialize(data);
  data.remote_insert(1, 2.345);
  data = serialize_and_deserialize(data);
  data.local_insert(1, "string 2");
  CHECK(data.extract() == std::make_pair("string 2"s, 2.345));
}

// [[OutputRegex, Local data not available.]]
[[noreturn]] SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData.no_local_data",
                               "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.remote_insert(0, 1.234);
  data.local_data(0);
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Only have local data at temporal_id]]
[[noreturn]] SPECTRE_TEST_CASE(
    "Unit.DG.SimpleMortarData.no_local_data_at_time",
    "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.local_insert(1, "");
  data.local_data(0);
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Received local data at 1, but already have remote
// data at 0]]
[[noreturn]] SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData.wrong_time.local",
                               "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.remote_insert(0, 0.);
  data.local_insert(1, "");
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Received remote data at 0, but already have local
// data at 1]]
[[noreturn]] SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData.wrong_time.remote",
                                "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.local_insert(1, "");
  data.remote_insert(0, 0.);
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Already received local data]]
[[noreturn]] SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData.double_insert.local",
                               "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.local_insert(1, "");
  data.local_insert(1, "");
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Already received remote data]]
[[noreturn]] SPECTRE_TEST_CASE(
    "Unit.DG.SimpleMortarData.double_insert.remote",
    "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.remote_insert(0, 0.);
  data.remote_insert(0, 0.);
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Tried to extract boundary data, but do not have any data]]
[[noreturn]] SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData.bad_extract.none",
                               "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.extract();
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Tried to extract boundary data, but do not have remote data]]
[[noreturn]] SPECTRE_TEST_CASE(
    "Unit.DG.SimpleMortarData.bad_extract.no_remote",
    "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.local_insert(1, "");
  data.extract();
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}

// [[OutputRegex, Tried to extract boundary data, but do not have local data]]
[[noreturn]] SPECTRE_TEST_CASE("Unit.DG.SimpleMortarData.bad_extract.no_local",
                               "[Unit][NumericalAlgorithms]") {
  ASSERTION_TEST();
#ifdef SPECTRE_DEBUG
  dg::SimpleMortarData<size_t, std::string, double> data;
  data.remote_insert(0, 0.);
  data.extract();
  ERROR("Failed to trigger ASSERT in an assertion test");
#endif
}
