#include <table-rdf/log.h>
#include <table-rdf/descriptor.h>

#include <catch2/catch.hpp>

#if !TRDF_HAS_CHRONO_PARSE
  #include <date/date.h>
#endif

#include <ranges>

namespace rdf {

TEST_CASE( "basics", "[core]" )
{
  #ifndef TRDF_TEST
    FAIL();
  #endif

  SECTION( "logging" )
  {
    SPDLOG_TRACE("trace");
    SPDLOG_DEBUG("debug");
    SPDLOG_INFO("info");
    SPDLOG_WARN("warn");
    SPDLOG_ERROR("error");
    SPDLOG_CRITICAL("critical");
  }

  SECTION ( "exceptions" )
  {
    using namespace Catch::Matchers;
    const auto thrower = [](char const* msg) {
      throw std::runtime_error(msg);
    };
    REQUIRE_THROWS_WITH(thrower("hello Catch2 exception testing"), Contains("hello"));
  }
}

TEST_CASE( "fields_builder", "[core]" )
{
  using field = rdf::field;
  rdf::fields_builder b;
  b.push({"formula",    "formula description", field::string_type})
   .push({ "symbol",    "ticker symbol",       field::key_type})
   .push({ "timestamp", "utc timestamp",       field::timestamp_type })
   .push({ "field 4",   "field 4 description", field::int32_type })
   .push({ "field 5",   "field 5 description", field::float32_type })
   .push({ "field 6",   "field 6 description", field::float64_type })
   .push({ "field 7",   "field 7 description", field::bool_type })
   .push({ "field 8",   "field 8 description", field::int32_type });

  rdf::descriptor d {"XYZ rdf::descriptor", "%Y%m%d %T", b};
  SPDLOG_DEBUG(d.describe());

  timestamp_t time0 = timestamp_t::clock::now() - 1000s; 
  timestamp_t time = time0;

}

TEST_CASE( "timestamp", "[core]" )
{
  constexpr auto parse_format = "%Y%m%d %T";
  constexpr auto str_format = "{:%Y%m%d %T}";
  
  SECTION( "raw symetry" ) 
  {
    std::string input = "20170915 13:11:34.356648000";
    std::istringstream in{input};
    timestamp_t tp;
    in >> CHRONO_PARSE_NAMESPACE::parse(parse_format, tp);
    REQUIRE(!in.fail());
    auto output = std::format(str_format, tp);
    REQUIRE(input == output);
  }

  timestamp_t now = timestamp_t::clock::now();

  SECTION( "util symetry" )
  {
    auto now_str = rdf::util::time_to_str(now, str_format);
    auto now_parsed = rdf::util::str_to_time(now_str, parse_format);
    REQUIRE(now == now_parsed);
  }

  SECTION( "rdf::descriptor symetry" )
  {
    using field = rdf::field;
    rdf::fields_builder b;
    b.push({ "symbol",    "", field::key_type })
     .push({ "timestamp", "", field::timestamp_type });
    rdf::descriptor d {"CSV Test Data Descriptor", "%Y%m%d %T",  b};
    auto now_str = d.time_to_str(now);
    auto now_parsed = d.str_to_time(now_str);
    REQUIRE(now == now_parsed);
  }
}

} // namespace rdf