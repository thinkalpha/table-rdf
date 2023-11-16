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

find_package(Threads REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(Boost 1.80.0 REQUIRED)

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
                      TRDF_HAS_CHRONO_PARSE=${TRDF_HAS_CHRONO_PARSE})

set(TRDF_LINK_LIBS spdlog::spdlog
                   Threads::Threads)           # Posix Threads.

set(TRDF_INCLUDE_DIRS # Include Howard Hinnant's date library for libcpp implementations that do not implement std::chrono parsing.
                      $<$<NOT:$<BOOL:${TRDF_HAS_CHRONO_PARSE}>>:${CMAKE_CURRENT_SOURCE_DIR}/third-party/date-3.0.1/include>)

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
                                               ${TRDF_INCLUDE_DIRS})
target_include_directories(table-rdf SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_link_libraries     (table-rdf INTERFACE ${TRDF_LINK_LIBS})

target_compile_definitions(table-rdf INTERFACE ${TRDF_COMPILE_DEFS})
target_compile_options    (table-rdf INTERFACE ${TRDF_COMPILE_OPTIONS})

set_property              (TARGET table-rdf PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#
# [ Table Record Data Format (RDF) Tests ]
# This target can be run directly or invoked via Ctest.
#
set(TRDF_CATCH_TESTS test/test_main.cpp
                     test/test_core.cpp)

add_executable            (table-rdf-tests ${TRDF_CATCH_TESTS})

set_property              (TARGET table-rdf-tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options    (table-rdf-tests PRIVATE ${TRDF_COMPILE_OPTIONS})
target_compile_definitions(table-rdf-tests PRIVATE ${TRDF_COMPILE_DEFS}
                                                   TRDF_TEST=1
                                                   TRDF_FILE_LOGGING=0
                                                   BOOST_ENABLE_ASSERT_HANDLER
                                                   # TODO: Update to Catch2 v3.0.1 to get support for runtime --skip-benchmarks.
                                                   # https://github.com/catchorg/Catch2/blob/devel/docs/command-line.md#skip-all-benchmarks 
                                                   #[[CATCH_CONFIG_ENABLE_BENCHMARKING]])

target_link_libraries     (table-rdf-tests PRIVATE table-rdf
                                                   Catch2::Catch2)

# Catch2 setup and Ctest integration.
include(CTest)
include(Catch)
catch_discover_tests(table-rdf-tests #[[REPORTER junit]] # TODO: Why does this cause error: '"junit" is not a valid reporter!' on Mozart?
                                     OUTPUT_DIR test_output
                                     OUTPUT_PREFIX "trdf_"
                                     OUTPUT_SUFFIX ".xml")
