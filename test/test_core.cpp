#include "log.h"
#include <table-rdf/rdf.h>
#include <table-rdf/type_traits.h>

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
  using namespace types;
  rdf::fields_builder builder;
  builder.push({"formula",    "formula description", String8, 31})
         .push({ "symbol",    "ticker symbol",       Key8, 31})
         .push({ "timestamp", "utc timestamp",       Timestamp })
         .push({ "field 4",   "field 4 description", Int32 })
         .push({ "field 5",   "field 5 description", Float32 })
         .push({ "field 6",   "field 6 description", Float64 })
         .push({ "field 7",   "field 7 description", Bool })
         .push({ "field 8",   "field 8 description", Int32 });

  rdf::descriptor d {"XYZ Descriptor", "%Y%m%d %T", builder};
  SPDLOG_DEBUG(d.describe());
}

TEST_CASE( "basic usage", "[core]" )
{
  using field = rdf::field;
  using namespace types;

  rdf::fields_builder builder;
  builder.push({ "Formula",          "Formula description",          String8, 31})
         .push({ "Symbol",           "Ticker Symbol",                Key8, 31})
         .push({ "Timestamp",        "UTC Timestamp",                Timestamp, field::k_no_payload, fmt::runtime("{:>40}") })
         .push({ "BarSequence",      "BarSequence description",      Int32 })
         .push({ "BarOpen",          "BarOpen description",          Float32 })
         .push({ "BarHigh",          "BarHigh description",          Float32 })
         .push({ "BarLow",           "BarLow description",           Float32 })
         .push({ "BarClose",         "BarClose description",         Float32 })
         .push({ "BarVolume",        "BarVolume description",        Float32 })
         .push({ "BarVWAPVolume",    "BarVWAPVolume description",    Int32 })
         .push({ "BarVWAPNotional",  "BarVWAPNotional description",  Int64 })
         .push({ "BarNotional",      "BarNotional description",      Int64 })
         .push({ "BarLastBid",       "BarLastBid description",       Float32 })
         .push({ "BarLastBidSize",   "BarLastBidSize description",   Int32 })
         .push({ "BarLastAsk",       "BarLastAsk description",       Float32 })
         .push({ "BarLastAskSize",   "BarLastAskSize description",   Int32 })
         .push({ "BarLastVolume",    "BarLastVolume description",    Int32 })
         .push({ "BarTotalVolume",   "BarTotalVolume description",   Int32 })
         .push({ "BarTradeCount",    "BarTradeCount description",    Int32 });

  rdf::descriptor d {"BarRecord 1m Descriptor", "%Y%m%d %T", builder};
  // SPDLOG_DEBUG(d.describe());

  constexpr auto k_count = 100;

  SECTION( "record write")
  {
    mem_t* const mem_alloc = (mem_t*)std::aligned_alloc(field::k_record_alignment, d.mem_size() * k_count);
    REQUIRE(boost::alignment::is_aligned(field::k_record_alignment, mem_alloc));
    auto mem = mem_alloc;

    timestamp_t const time = timestamp_t::clock::now();

    auto fields = d.fields();
    
    // Write to record memory.
    for (int i = 0; i < k_count; ++i)
    {
      auto const t = time + timestamp_t::duration{i};

      d.fields("Formula")        .write<String8>  (mem, fmt::format("_B[_{}d:{}d]", i, i + 1));
      d.fields("Symbol")         .write<Key8>     (mem, fmt::format("AAPL_{}:*", i % 10));
      d.fields("Timestamp")      .write<Timestamp>(mem, t);
      d.fields("BarSequence")    .write<Int32>    (mem, i);
      d.fields("BarOpen")        .write<Float32>  (mem, i * 1.f);
      d.fields("BarHigh")        .write<Float32>  (mem, i * 10.f);
      d.fields("BarLow")         .write<Float32>  (mem, i * 100.f);
      d.fields("BarClose")       .write<Float32>  (mem, i * 1000.f);
      d.fields("BarVolume")      .write<Float32>  (mem, i * 10000.f);
      d.fields("BarVWAPVolume")  .write<Int32>    (mem, i << 1);
      d.fields("BarVWAPNotional").write<Int64>    (mem, int64_t(i << 2));
      d.fields("BarNotional")    .write<Int64>    (mem, int64_t(i << 3));
      d.fields("BarLastBid")     .write<Float32>  (mem, i * 100000.f);
      d.fields("BarLastBidSize") .write<Int32>    (mem, i << 4);
      d.fields("BarLastAsk")     .write<Float32>  (mem, i * 1000000.f);
      d.fields("BarLastAskSize") .write<Int32>    (mem, i << 5);
      d.fields("BarLastVolume")  .write<Int32>    (mem, i << 6);
      d.fields("BarTotalVolume") .write<Int32>    (mem, i << 7);
      d.fields("BarTradeCount")  .write<Int32>    (mem, i << 8);

      mem += d.mem_size();
    }

    SECTION( "record read" )
    {
      mem = mem_alloc;
      for (int i = 0; i < k_count; ++i)
      {
        auto record = rdf::record{mem};
        REQUIRE(record.get<Int32>(fields[3]) == i);

        auto const t = time + timestamp_t::duration{i};

        REQUIRE(record.get<String8>  (d.fields("Formula"))          == fmt::format("_B[_{}d:{}d]", i, i + 1));
        REQUIRE(record.get<Key8>     (d.fields("Symbol"))           == fmt::format("AAPL_{}:*", i % 10));
        REQUIRE(record.get<Timestamp>(d.fields("Timestamp"))        == t);
        REQUIRE(record.get<Int32>    (d.fields("BarSequence"))      == i);
        REQUIRE(record.get<Float32>  (d.fields("BarOpen"))          == i * 1.f);
        REQUIRE(record.get<Float32>  (d.fields("BarHigh"))          == i * 10.f);
        REQUIRE(record.get<Float32>  (d.fields("BarLow"))           == i * 100.f);
        REQUIRE(record.get<Float32>  (d.fields("BarClose"))         == i * 1000.f);
        REQUIRE(record.get<Float32>  (d.fields("BarVolume"))        == i * 10000.f);
        REQUIRE(record.get<Int32>    (d.fields("BarVWAPVolume"))    == i << 1);
        REQUIRE(record.get<Int64>    (d.fields("BarVWAPNotional"))  == i << 2);
        REQUIRE(record.get<Int64>    (d.fields("BarNotional"))      == i << 3);
        REQUIRE(record.get<Float32>  (d.fields("BarLastBid"))       == i * 100000.f);
        REQUIRE(record.get<Int32>    (d.fields("BarLastBidSize"))   == i << 4);
        REQUIRE(record.get<Float32>  (d.fields("BarLastAsk"))       == i * 1000000.f);
        REQUIRE(record.get<Int32>    (d.fields("BarLastAskSize"))   == i << 5);
        REQUIRE(record.get<Int32>    (d.fields("BarLastVolume"))    == i << 6);
        REQUIRE(record.get<Int32>    (d.fields("BarTotalVolume"))   == i << 7);
        REQUIRE(record.get<Int32>    (d.fields("BarTradeCount"))    == i << 8);

        mem += d.mem_size();
      }
    }

    SECTION( "record read via view" )
    {
      mspan mem_span{mem_alloc, d.mem_size() * k_count};
      auto records = rdf::views::records(mem_span, d);

      static_assert(std::is_same_v<rdf::views::records_view_t, decltype(records)>);
      
      for (auto r : records) {
        SPDLOG_DEBUG(r.to_string(d));
      }
      
      SPDLOG_DEBUG(d.describe());
    }

    free(mem_alloc);
  }
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
    rdf::fields_builder b;
    b.push({ "symbol",    "", types::Key8, 31 })
     .push({ "timestamp", "", types::Timestamp });
    rdf::descriptor d {"Test Data Descriptor", "%Y%m%d %T",  b};
    auto now_str = d.time_to_str(now);
    auto now_parsed = d.str_to_time(now_str);
    REQUIRE(now == now_parsed);

    // mem_t* m;
    // auto blah = field_t<types::Key8>::read(m, d.fields("asdsad"));
  }
}

} // namespace rdf