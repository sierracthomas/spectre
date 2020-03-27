# Distributed under the MIT License.
# See LICENSE.txt for details.

find_package(LAPACK REQUIRED)
message(STATUS "LAPACK libs: " ${LAPACK_LIBRARIES})

file(APPEND
  "${CMAKE_BINARY_DIR}/LibraryVersions.txt"
  "LAPACK_LIBRARIES:  ${LAPACK_LIBRARIES}\n"
  )

add_library(Lapack INTERFACE IMPORTED)
set_property(TARGET Lapack
  APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${LAPACK_LIBRARIES})
set_property(TARGET Lapack
  APPEND PROPERTY INTERFACE_LINK_OPTIONS ${LAPACK_LINKER_FLAGS})
