#include "log.h"
#include <table-rdf/rdf.h>
#include <table-rdf/traits.h>

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

TEST_CASE( "field packing", "[core]" )
{
  using namespace types;

  const auto payload_size = 186;

  rdf::fields_builder builder;
  builder.push({ "1",  "", String8, payload_size})
         .push({ "2",  "", Int8 })
         .push({ "3",  "", Key8, payload_size})
         .push({ "4",  "", Uint8 })
         .push({ "5",  "", Timestamp })
         .push({ "6",  "", Bool })
         .push({ "7",  "", Int32 })
         .push({ "8",  "", Int16 })
         .push({ "9",  "", Float32 })
         .push({ "10", "", Bool })
         .push({ "11", "", Float64 })
         .push({ "12", "", Bool })
         .push({ "13", "", Float128 })
         .push({ "14", "", Bool })
         .push({ "15", "", Int32 });

  rdf::descriptor d {"Descriptor", builder, false};
  SPDLOG_DEBUG(d.describe());

  rdf::descriptor d_packed {"Packed Descriptor", builder, true};
  SPDLOG_DEBUG(d_packed.describe(true));

  REQUIRE(d.mem_size() >= d_packed.mem_size());

  struct str8 {
     uint8_t prefix;
     std::string_view::value_type payload[payload_size];
  };
  using key8 = str8;

  // Check that our alignment calculations match the compiler's.
  struct check_align {
    str8 f1;
    int8_t f2;
    key8 f3;
    uint8_t f4;
    timestamp_t f5;
    bool f6;
    int32_t f7;
    int16_t f8;
    std::float32_t f9;
    bool f10;
    std::float64_t f11;
    bool f12;
    std::float128_t f13;
    bool f14;
    int32_t f15;
  };

  struct check_align_packed {
    std::float128_t f13;
    timestamp_t f5;
    std::float64_t f11;
    int32_t f7;
    std::float32_t f9;
    int32_t f15;
    int16_t f8;
    str8 f1;
    int8_t f2;
    key8 f3;
    uint8_t f4;
    bool f6;
    bool f10;
    bool f12;
    bool f14;
  };

  SPDLOG_DEBUG("sizeof(check_align) = {}", sizeof(check_align));
  SPDLOG_DEBUG("alignof(check_align) = {}", alignof(check_align));
  SPDLOG_DEBUG("sizeof(check_align_packed) = {}", sizeof(check_align_packed));
  SPDLOG_DEBUG("alignof(check_align_packed) = {}", alignof(check_align_packed));

  REQUIRE(sizeof(check_align) == d.mem_size());
  REQUIRE(alignof(check_align) == d.mem_align());
  REQUIRE(sizeof(check_align_packed) == d_packed.mem_size());
  REQUIRE(alignof(check_align_packed) == d_packed.mem_align());
}

TEST_CASE( "basic usage", "[core]" )
{
  using field = rdf::field;
  using namespace types;

  // Test a descriptor with all supported types.
  rdf::fields_builder builder;
  builder.push({ "Key8 Field",       "Key8 field description",       Key8, 31})
         .push({ "Key16 Field",      "Key16 field description",      Key16, 254 })
         .push({ "String8 Field",    "String8 field description",    String8, 31 })
         .push({ "String16 Field",   "String16 field description",   String16, 254 })
         .push({ "Timestamp Field",  "Timestamp field description",  Timestamp, field::k_no_payload, fmt::runtime("{:>40}") })
         .push({ "Char Field",       "Char field description",       Char })
         .push({ "Utf_Char8 Field",  "Utf_Char8 field description",  Utf_Char8 })
         .push({ "Utf_Char16 Field", "Utf_Char16 field description", Utf_Char16 })
         .push({ "Utf_Char32 Field", "Utf_Char32 field description", Utf_Char32 })
         .push({ "Int8 Field",       "Int8 field description",       Int8 })
         .push({ "Int16 Field",      "Int16 field description",      Int16 })
         .push({ "Int32 Field",      "Int32 field description",      Int32 })
         .push({ "Int64 Field",      "Int64 field description",      Int64 })
         .push({ "Uint8 Field",      "Uint8 field description",      Uint8 })
         .push({ "Uint16 Field",     "Uint16 field description",     Uint16 })
         .push({ "Uint32 Field",     "Uint32 field description",     Uint32 })
         .push({ "Uint64 Field",     "Uint64 field description",     Uint64 })
         .push({ "Float16 Field",    "Float16 field description",    Float16 })
         .push({ "Float32 Field",    "Float32 field description",    Float32 })
         .push({ "Float64 Field",    "Float64 field description",    Float64 })
         .push({ "Float128 Field",   "Float128 field description",   Float128 })
         .push({ "Bool Field",       "Bool field description",       Bool });


  descriptor d {"All Fields Test Descriptor", builder};

  constexpr auto k_count = 100;

  SECTION("record write")
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
      auto const ui = (uint64_t)i;

      d.fields("Key8 Field")      .write<Key8>      (mem, fmt::format("_B[_{}d:{}d]", i, i + 1));
      d.fields("Key16 Field")     .write<Key16>     (mem, fmt::format("AAPL_{}:*", i % 10));
      d.fields("String8 Field")   .write<String8>   (mem, fmt::format("_B[_{}d:{}d]", i + 2, i + 3));
      d.fields("String16 Field")  .write<String16>  (mem, fmt::format("SPY_{}:*", i % 10));
      d.fields("Timestamp Field") .write<Timestamp> (mem, t);
      d.fields("Char Field")      .write<Char>      (mem, 33 + i % (127 - 33));
      d.fields("Utf_Char8 Field") .write<Utf_Char8> (mem, ui % 256);
      d.fields("Utf_Char16 Field").write<Utf_Char16>(mem, ui % 512);
      d.fields("Utf_Char32 Field").write<Utf_Char32>(mem, ui % 1024);
      d.fields("Int8 Field")      .write<Int8>      (mem, -i << 0);
      d.fields("Int16 Field")     .write<Int16>     (mem, -i << 1);
      d.fields("Int32 Field")     .write<Int32>     (mem, -i << 2);
      d.fields("Int64 Field")     .write<Int64>     (mem, -i << 3);
      d.fields("Uint8 Field")     .write<Uint8>     (mem, i << 0);
      d.fields("Uint16 Field")    .write<Uint16>    (mem, i << 1);
      d.fields("Uint32 Field")    .write<Uint32>    (mem, i << 2);
      d.fields("Uint64 Field")    .write<Uint64>    (mem, i << 3);
      d.fields("Float16 Field")   .write<Float16>   (mem, i * 1.f16);
      d.fields("Float32 Field")   .write<Float32>   (mem, i * 10.f32);
      d.fields("Float64 Field")   .write<Float64>   (mem, i * 100.f64);
      d.fields("Float128 Field")  .write<Float128>  (mem, i * 1000.f128);
      d.fields("Bool Field")      .write<Bool>      (mem, i % 2);

      mem += d.mem_size();
    }

    SECTION( "record read" )
    {
      mem = mem_alloc;
      for (int i = 0; i < k_count; ++i)
      {
        auto record = rdf::record{mem};

        auto const t = time + timestamp_t::duration{i};
        auto const ui = (uint64_t)i;

        REQUIRE(record.get<Key8>      (d.fields("Key8 Field"))       == fmt::format("_B[_{}d:{}d]", i, i + 1));
        REQUIRE(record.get<Key16>     (d.fields("Key16 Field"))      == fmt::format("AAPL_{}:*", i % 10));
        REQUIRE(record.get<String8>   (d.fields("String8 Field"))    == fmt::format("_B[_{}d:{}d]", i + 2, i + 3));
        REQUIRE(record.get<String16>  (d.fields("String16 Field"))   == fmt::format("SPY_{}:*", i % 10));
        REQUIRE(record.get<Timestamp> (d.fields("Timestamp Field"))  == t);
        REQUIRE(record.get<Char>      (d.fields("Char Field"))       == 33 + i % (127 - 33));
        REQUIRE(record.get<Utf_Char8> (d.fields("Utf_Char8 Field"))  == ui % 256);
        REQUIRE(record.get<Utf_Char16>(d.fields("Utf_Char16 Field")) == ui % 512);
        REQUIRE(record.get<Utf_Char32>(d.fields("Utf_Char32 Field")) == ui % 1024);
        REQUIRE(record.get<Int8>      (d.fields("Int8 Field"))       == -i << 0);
        REQUIRE(record.get<Int16>     (d.fields("Int16 Field"))      == -i << 1);
        REQUIRE(record.get<Int32>     (d.fields("Int32 Field"))      == -i << 2);
        REQUIRE(record.get<Int64>     (d.fields("Int64 Field"))      == -i << 3);
        REQUIRE(record.get<Uint8>     (d.fields("Uint8 Field"))      == ui << 0);
        REQUIRE(record.get<Uint16>    (d.fields("Uint16 Field"))     == ui << 1);
        REQUIRE(record.get<Uint32>    (d.fields("Uint32 Field"))     == ui << 2);
        REQUIRE(record.get<Uint64>    (d.fields("Uint64 Field"))     == ui << 3);
        REQUIRE(record.get<Float16>   (d.fields("Float16 Field"))    == i * 1.f16);
        REQUIRE(record.get<Float32>   (d.fields("Float32 Field"))    == i * 10.f32);
        REQUIRE(record.get<Float64>   (d.fields("Float64 Field"))    == i * 100.f64);
        REQUIRE(record.get<Float128>  (d.fields("Float128 Field"))   == i * 1000.f128);
        REQUIRE(record.get<Bool>      (d.fields("Bool Field"))       == i % 2);

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
  SECTION( "raw symetry" ) 
  {
    std::string input = "20170915 13:11:34.356648000";
    std::istringstream in{input};
    timestamp_t tp;
    in >> CHRONO_PARSE_NAMESPACE::parse("%Y%m%d %T", tp);
    REQUIRE(!in.fail());
    auto output = std::format("{:%Y%m%d %T}", tp);
    REQUIRE(input == output);
  }

  timestamp_t now = timestamp_t::clock::now();

  SECTION( "util symetry" )
  {
    auto now_str = rdf::util::time_to_str(now, "{:%Y%m%d %T}");
    auto now_parsed = rdf::util::str_to_time(now_str, "%Y%m%d %T");
    REQUIRE(now == now_parsed);
  }

  SECTION( "helper symetry" )
  {
    util::time_fmt tf{"%Y%m%d %T"};    
    auto now_str = tf.to_str(now);
    auto now_parsed = tf.to_time(now_str);
    REQUIRE(now == now_parsed);
  }

}

} // namespace rdf