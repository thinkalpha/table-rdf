#
# [ table-rdf ]
#
# CMakeLists for creating and installing an interface (header only) library for the Record Data Format (RDF) library.
# This is used by the table-rdf vcpkg portfile and must be compatible with it.
#

cmake_minimum_required(VERSION 3.23.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(table-rdf INTERFACE)

# Location of headers in build tree.
target_include_directories(table-rdf INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/table-rdf/>)

# Location of headers in installation.
# We set this to the root include directory (e.g. out\build\win-debug-x64\vcpkg_installed\x64-windows-static\include)
# so that usage is '#include <table-rdf/header.h>'.
target_include_directories(table-rdf SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

include(GNUInstallDirs)

install(TARGETS table-rdf EXPORT table-rdf-targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
# Generate table-rdf-targets.cmake and install in conventional location.
install(EXPORT table-rdf-targets FILE table-rdf-targets.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/table-rdf)
# Install table-rdf-config.cmake in conventional location.
install(FILES table-rdf-config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/table-rdf)
# Install the headers. Note the destination creates a /table-rdf subdirectory
# in accordance with the installation target_include_directories() above.
install(DIRECTORY table-rdf/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/table-rdf FILES_MATCHING PATTERN "*")

export(EXPORT table-rdf-targets FILE table-rdf-config.cmake)

