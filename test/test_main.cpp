// Catch2 test executable definition.
// *** Do not put anything else in this file. ***
// See: https://github.com/catchorg/Catch2/blob/v2.x/docs/slow-compiles.md

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include "log.h"

int main( int argc, char* argv[] )
{
  tds::configure_logging();

  const auto result = Catch::Session().run( argc, argv );

  tds::finalize_logging();

  return result;
}