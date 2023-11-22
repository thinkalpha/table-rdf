#include "log.h"
#include <table-rdf/rdf.h>

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
  rdf::fields_builder builder;
  builder.push({"formula",    "formula description", field::string_type, 31})
         .push({ "symbol",    "ticker symbol",       field::key_type, 31})
         .push({ "timestamp", "utc timestamp",       field::timestamp_type })
         .push({ "field 4",   "field 4 description", field::int32_type })
         .push({ "field 5",   "field 5 description", field::float32_type })
         .push({ "field 6",   "field 6 description", field::float64_type })
         .push({ "field 7",   "field 7 description", field::bool_type })
         .push({ "field 8",   "field 8 description", field::int32_type });

  rdf::descriptor d {"XYZ Descriptor", "%Y%m%d %T", builder};
  SPDLOG_DEBUG(d.describe());
}

TEST_CASE( "basic usage", "[core]" )
{
  using field = rdf::field;
  rdf::fields_builder builder;
  builder.push({ "Formula",          "Formula description",          field::string_type, 31})
         .push({ "Symbol",           "Ticker Symbol",                field::key_type, 31})
         .push({ "Timestamp",        "UTC Timestamp",                field::timestamp_type, field::k_no_payload, fmt::runtime("{:>40}") })
         .push({ "BarSequence",      "BarSequence description",      field::int32_type })
         .push({ "BarOpen",          "BarOpen description",          field::float32_type })
         .push({ "BarHigh",          "BarHigh description",          field::float32_type })
         .push({ "BarLow",           "BarLow description",           field::float32_type })
         .push({ "BarClose",         "BarClose description",         field::float32_type })
         .push({ "BarVolume",        "BarVolume description",        field::float32_type })
         .push({ "BarVWAPVolume",    "BarVWAPVolume description",    field::int32_type })
         .push({ "BarVWAPNotional",  "BarVWAPNotional description",  field::int64_type })
         .push({ "BarNotional",      "BarNotional description",      field::int64_type })
         .push({ "BarLastBid",       "BarLastBid description",       field::float32_type })
         .push({ "BarLastBidSize",   "BarLastBidSize description",   field::int32_type })
         .push({ "BarLastAsk",       "BarLastAsk description",       field::float32_type })
         .push({ "BarLastAskSize",   "BarLastAskSize description",   field::int32_type })
         .push({ "BarLastVolume",    "BarLastVolume description",    field::int32_type })
         .push({ "BarTotalVolume",   "BarTotalVolume description",   field::int32_type })
         .push({ "BarTradeCount",    "BarTradeCount description",    field::int32_type });

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
      rdf::raw_time_t raw_time = timestamp_t{t}.time_since_epoch().count();

      // TODO: Improve this interface to something like d.fields("Symbol").write(mem, "AAPL:*);
      field::write<string_t>  (mem, d.fields("Formula"),         fmt::format("_B[_{}d:{}d]", i, i + 1));
      field::write<key_t>     (mem, d.fields("Symbol"),          fmt::format("AAPL_{}:*", i % 10));
      field::write<raw_time_t>(mem, d.fields("Timestamp"),       raw_time);
      field::write<int32_t>   (mem, d.fields("BarSequence"),     i);
      field::write<float>     (mem, d.fields("BarOpen"),         i * 1.f);
      field::write<float>     (mem, d.fields("BarHigh"),         i * 10.f);
      field::write<float>     (mem, d.fields("BarLow"),          i * 100.f);
      field::write<float>     (mem, d.fields("BarClose"),        i * 1000.f);
      field::write<float>     (mem, d.fields("BarVolume"),       i * 10000.f);
      field::write<int32_t>   (mem, d.fields("BarVWAPVolume"),   i << 1);
      field::write<int64_t>   (mem, d.fields("BarVWAPNotional"), i << 2);
      field::write<int64_t>   (mem, d.fields("BarNotional"),     i << 3);
      field::write<float>     (mem, d.fields("BarLastBid"),      i * 100000.f);
      field::write<int32_t>   (mem, d.fields("BarLastBidSize"),  i << 4);
      field::write<float>     (mem, d.fields("BarLastAsk"),      i * 1000000.f);
      field::write<int32_t>   (mem, d.fields("BarLastAskSize"),  i << 5);
      field::write<int32_t>   (mem, d.fields("BarLastVolume"),   i << 6);
      field::write<int32_t>   (mem, d.fields("BarTotalVolume"),  i << 7);
      field::write<int32_t>   (mem, d.fields("BarTradeCount"),   i << 8);
      mem += d.mem_size();
    }

    SECTION( "record read" )
    {
      mem = mem_alloc;
      for (int i = 0; i < k_count; ++i)
      {
        auto record = rdf::record{mem};
        REQUIRE(record.get<int32_t>(fields[3].offset()) == i);

        REQUIRE(record.get<string_t>    (d.fields("Formula").offset())          == fmt::format("_B[_{}d:{}d]", i, i + 1));
        REQUIRE(record.key(d)                                                   == fmt::format("AAPL_{}:*", i % 10));
        REQUIRE(record.timestamp(d)                                             == time + timestamp_t::duration{i});
        REQUIRE(record.get<int32_t>     (d.fields("BarSequence").offset())      == i);
        REQUIRE(record.get<float>       (d.fields("BarOpen").offset())          == i * 1.f);
        REQUIRE(record.get<float>       (d.fields("BarHigh").offset())          == i * 10.f);
        REQUIRE(record.get<float>       (d.fields("BarLow").offset())           == i * 100.f);
        REQUIRE(record.get<float>       (d.fields("BarClose").offset())         == i * 1000.f);
        REQUIRE(record.get<float>       (d.fields("BarVolume").offset())        == i * 10000.f);
        REQUIRE(record.get<int32_t>     (d.fields("BarVWAPVolume").offset())    == i << 1);
        REQUIRE(record.get<int64_t>     (d.fields("BarVWAPNotional").offset())  == i << 2);
        REQUIRE(record.get<int64_t>     (d.fields("BarNotional").offset())      == i << 3);
        REQUIRE(record.get<float>       (d.fields("BarLastBid").offset())       == i * 100000.f);
        REQUIRE(record.get<int32_t>     (d.fields("BarLastBidSize").offset())   == i << 4);
        REQUIRE(record.get<float>       (d.fields("BarLastAsk").offset())       == i * 1000000.f);
        REQUIRE(record.get<int32_t>     (d.fields("BarLastAskSize").offset())   == i << 5);
        REQUIRE(record.get<int32_t>     (d.fields("BarLastVolume").offset())    == i << 6);
        REQUIRE(record.get<int32_t>     (d.fields("BarTotalVolume").offset())   == i << 7);
        REQUIRE(record.get<int32_t>     (d.fields("BarTradeCount").offset())    == i << 8);

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
    using field = rdf::field;
    rdf::fields_builder b;
    b.push({ "symbol",    "", field::key_type, 31 })
     .push({ "timestamp", "", field::timestamp_type });
    rdf::descriptor d {"Test Data Descriptor", "%Y%m%d %T",  b};
    auto now_str = d.time_to_str(now);
    auto now_parsed = d.str_to_time(now_str);
    REQUIRE(now == now_parsed);
  }
}

} // namespace rdf