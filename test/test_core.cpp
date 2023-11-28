#include "log.h"
#include <table-rdf/rdf.h>
#include <table-rdf/traits.h>

#include <catch2/catch.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#if !TRDF_HAS_CHRONO_PARSE
  #include <date/date.h>
#endif
#include <ranges>
#include <fstream>

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

  rdf::descriptor d {"XYZ Descriptor", builder};
  SPDLOG_DEBUG(d.describe());
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

TEST_CASE( "record <-> binary file", "[core]" )
{
  using field = rdf::field;
  using namespace types;
  namespace ipc = boost::interprocess;

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

  // Set up memory mapped file.
  char const* file_name  = "./test.bin";
  
  constexpr auto k_desired_file_size = 1024 * 1024 * 1024; // 1 GB.
  auto const record_count = k_desired_file_size / d.mem_size();
  size_t const file_size = record_count * d.mem_size();

  ipc::file_mapping::remove(file_name);
  {  
    std::ofstream file{file_name, std::ios_base::binary};
    file.seekp(file_size-1, std::ios_base::beg);         // Set the file size.
    file.put(0);
  }

  ipc::file_mapping m_file(file_name, ipc::read_write);  // Create a file mapping.
  ipc::mapped_region region(m_file, ipc::read_write);    // Map the whole file with read-write permissions in this process.
  void* mapped_file_addr = region.get_address();         // Get the address of the mapped region.
  REQUIRE(region.get_size() == file_size);

  {
    mem_t* const mem_alloc = (mem_t*)boost::alignment::align_up(mapped_file_addr, field::k_record_alignment);
    REQUIRE(boost::alignment::is_aligned(field::k_record_alignment, mem_alloc));
    auto mem = mem_alloc;

    timestamp_t const time = timestamp_t::clock::now();

    auto fields = d.fields();
    
    // Write records to file.
    for (int64_t i = 0; i < (int64_t)record_count; ++i)
    {
      auto const t = time + timestamp_t::duration{i};
      auto const ui = (uint64_t)i;

      d.fields(0ul) .write<Key8>      (mem, fmt::format("_B[_{}d:{}d]", i, i + 1));
      d.fields(1ul) .write<Key16>     (mem, fmt::format("AAPL_{}:*", i % 10));
      d.fields(2ul) .write<String8>   (mem, fmt::format("_B[_{}d:{}d]", i + 2, i + 3));
      d.fields(3ul) .write<String16>  (mem, fmt::format("SPY_{}:*", i % 10));
      d.fields(4ul) .write<Timestamp> (mem, t);
      d.fields(5ul) .write<Char>      (mem, 33 + i % (127 - 33));
      d.fields(6ul) .write<Utf_Char8> (mem, ui % 256);
      d.fields(7ul) .write<Utf_Char16>(mem, ui % 512);
      d.fields(8ul) .write<Utf_Char32>(mem, ui % 1024);
      d.fields(9ul) .write<Int8>      (mem, -i << 0);
      d.fields(10ul).write<Int16>     (mem, -i << 1);
      d.fields(11ul).write<Int32>     (mem, -i << 2);
      d.fields(12ul).write<Int64>     (mem, -i << 3);
      d.fields(13ul).write<Uint8>     (mem, i << 0);
      d.fields(14ul).write<Uint16>    (mem, i << 1);
      d.fields(15ul).write<Uint32>    (mem, i << 2);
      d.fields(16ul).write<Uint64>    (mem, i << 3);
      d.fields(17ul).write<Float16>   (mem, i * 1.f16);
      d.fields(18ul).write<Float32>   (mem, i * 10.f32);
      d.fields(19ul).write<Float64>   (mem, i * 100.f64);
      d.fields(20ul).write<Float128>  (mem, i * 1000.f128);
      d.fields(21ul).write<Bool>      (mem, i % 2);

      mem += d.mem_size();
    }

    // Read records from file.
    mem = mem_alloc;
    for (int64_t i = 0; i < (int64_t)record_count; ++i)
    {
      auto record = rdf::record{mem};

      auto const t = time + timestamp_t::duration{i};
      auto const ui = (uint64_t)i;

      REQUIRE(record.get<Key8>      (d.fields(0ul))  == fmt::format("_B[_{}d:{}d]", i, i + 1));
      REQUIRE(record.get<Key16>     (d.fields(1ul))  == fmt::format("AAPL_{}:*", i % 10));
      REQUIRE(record.get<String8>   (d.fields(2ul))  == fmt::format("_B[_{}d:{}d]", i + 2, i + 3));
      REQUIRE(record.get<String16>  (d.fields(3ul))  == fmt::format("SPY_{}:*", i % 10));
      REQUIRE(record.get<Timestamp> (d.fields(4ul))  == t);
      REQUIRE(record.get<Char>      (d.fields(5ul))  == 33 + i % (127 - 33));
      REQUIRE(record.get<Utf_Char8> (d.fields(6ul))  == (char8_t)(ui % 256));
      REQUIRE(record.get<Utf_Char16>(d.fields(7ul))  == (char16_t)(ui % 512));
      REQUIRE(record.get<Utf_Char32>(d.fields(8ul))  == (char32_t)(ui % 1024));
      REQUIRE(record.get<Int8>      (d.fields(9ul))  == (int8_t)(-i << 0));
      REQUIRE(record.get<Int16>     (d.fields(10ul)) == (int16_t)(-i << 1));
      REQUIRE(record.get<Int32>     (d.fields(11ul)) == (int32_t)(-i << 2));
      REQUIRE(record.get<Int64>     (d.fields(12ul)) == (int64_t)(-i << 3));
      REQUIRE(record.get<Uint8>     (d.fields(13ul)) == (uint8_t)(ui << 0));
      REQUIRE(record.get<Uint16>    (d.fields(14ul)) == (uint16_t)(ui << 1));
      REQUIRE(record.get<Uint32>    (d.fields(15ul)) == (uint32_t)(ui << 2));
      REQUIRE(record.get<Uint64>    (d.fields(16ul)) == (uint64_t)(ui << 3));
      REQUIRE(record.get<Float16>   (d.fields(17ul)) == (std::float16_t)i * 1.f16);
      REQUIRE(record.get<Float32>   (d.fields(18ul)) == (std::float32_t)i * 10.f32);
      REQUIRE(record.get<Float64>   (d.fields(19ul)) == (std::float64_t)i * 100.f64);
      REQUIRE(record.get<Float128>  (d.fields(20ul)) == (std::float128_t)i * 1000.f128);
      REQUIRE(record.get<Bool>      (d.fields(21ul)) == i % 2);

      mem += d.mem_size();
    }
  }
}

} // namespace rdf