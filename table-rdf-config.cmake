# CMake config file to facilitate find_package(table-rdf CONFIG REQURIED) etc.

# Include the auto-generated targets .cmake file.
include("${CMAKE_CURRENT_LIST_DIR}/table-rdf-targets.cmake")

# Find dependencies.
find_package(spdlog CONFIG REQUIRED)
find_package(Boost 1.80.0 REQUIRED)
find_package(date CONFIG REQUIRED)
