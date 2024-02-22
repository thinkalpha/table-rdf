cmake_minimum_required(VERSION 3.22.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vcpkg cmake integration via cmake toolchain file.
if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
else()
  message(FATAL_ERROR "VCPKG_ROOT environment variable is not defined.")
endif()

set(VCPKG_FEATURE_FLAGS versions)

project(table-rdf VERSION 0.1.0 LANGUAGES CXX)

find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(Boost 1.80.0 REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)


# In particular -Wreorder should be on to protect our use of std::move() in constructor initialiser lists.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(TRDF_COMPILE_OPTIONS /W4          # TODO: Switch to /Wall.
                           /w15038       # Make class member initialisation order a level 1 warning.
                           /wd4505       # Unreferenced local function has been removed.
                           #[[/wd5045]]) # TODO: Decide on Spectre mitigation policy.
  set(TRDF_HAS_CHRONO_PARSE 1)
else()
  # Assume gcc-based compiler / frontend.
  set(TRDF_COMPILE_OPTIONS -Wall)
  set(TRDF_HAS_CHRONO_PARSE 0)          # TODO: std::chrono::parse will be available in gcc 14, remove this if we upgrade.
endif()

set(TRDF_COMPILE_DEFS TRDF_ASYNC_LOGGING=$<CONFIG:Release>
                      TRDF_HAS_CHRONO_PARSE=${TRDF_HAS_CHRONO_PARSE}
                      # To aid in debugging timestamp log output. This will add different ANSI-colors each timestamp 
                      # string based on the string hash. Note: This causes some timestamp parsing tests to fail.
                      TRDF_TIMESTAMP_COLORING=0
                      BOOST_ENABLE_ASSERT_DEBUG_HANDLER)

set(TRDF_LINK_LIBS spdlog::spdlog
                   # Use Howard Hinnant's date library for libcpp implementations that do not implement std::chrono parsing.
                   $<$<NOT:$<BOOL:${TRDF_HAS_CHRONO_PARSE}>>:date::date>)

set(TRDF_INCLUDE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

#
# [ Global ]
#
option(TRDF_ASAN "Build this target with AddressSanitizer" OFF)
if (TRDF_ASAN)
  if (MSVC)
    add_compile_options(/fsanitize=address)
  else()
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
endif()

#
# [ Table Record Data Format (RDF) ]
# Currently configured as a header-only target.
#
add_library               (table-rdf INTERFACE)
target_include_directories(table-rdf INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                                               $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>
                                               ${TRDF_INCLUDE_DIRS})

target_link_libraries     (table-rdf PUBLIC INTERFACE ${TRDF_LINK_LIBS})

target_compile_definitions(table-rdf INTERFACE ${TRDF_COMPILE_DEFS})
target_compile_options    (table-rdf INTERFACE ${TRDF_COMPILE_OPTIONS})

set_property              (TARGET table-rdf PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#
# [ Table Record Data Format (RDF) Tests ]
# This target can be run directly or invoked via Ctest.
#
set(TRDF_CATCH_TESTS test/test_main.cpp
                     test/test_core.cpp
                     test/test_performance.cpp)

add_executable            (table-rdf-tests ${TRDF_CATCH_TESTS})

set_property              (TARGET table-rdf-tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options    (table-rdf-tests PRIVATE ${TRDF_COMPILE_OPTIONS}
                                                   -ltbb)
target_compile_definitions(table-rdf-tests PRIVATE ${TRDF_COMPILE_DEFS}
                                                   TRDF_TEST=1
                                                   TRDF_FILE_LOGGING=0
                                                   # TODO: Update to Catch2 v3.0.1 to get support for runtime --skip-benchmarks.
                                                   # https://github.com/catchorg/Catch2/blob/devel/docs/command-line.md#skip-all-benchmarks 
                                                   CATCH_CONFIG_ENABLE_BENCHMARKING)

target_link_libraries     (table-rdf-tests PRIVATE table-rdf
                                                   Catch2::Catch2
                                                   TBB::tbb)

# Catch2 setup and Ctest integration.
include(CTest)
include(Catch)
catch_discover_tests(table-rdf-tests #[[REPORTER junit]] # TODO: Why does this cause error: '"junit" is not a valid reporter!' on Mozart?
                                     OUTPUT_DIR test_output
                                     OUTPUT_PREFIX "trdf_"
                                     OUTPUT_SUFFIX ".xml")

# Install.
include(GNUInstallDirs)

install(TARGETS table-rdf EXPORT table-rdf-targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
# Generate table-rdf-targets.cmake and install in conventional location.
install(EXPORT table-rdf-targets FILE table-rdf-targets.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/table-rdf)
# Install table-rdf-config.cmake in conventional location.
install(FILES table-rdf-config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/table-rdf)
# Install the headers. Note the destination creates a /table-rdf subdirectory
# in accordance with the installation target_include_directories() above.
install(DIRECTORY include/table-rdf/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/table-rdf FILES_MATCHING PATTERN "*")

export(EXPORT table-rdf-targets FILE table-rdf-config.cmake)
