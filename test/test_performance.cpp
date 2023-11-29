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

inline void generate_records(mem_t* dest, descriptor const& d, size_t count)
{
  auto mem = dest;
  auto const time = timestamp_t::clock::now();

  for (int64_t i = 0; i < (int64_t)count; ++i)
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
}

TEST_CASE( "read/write", "[perf]" )
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


  descriptor desc {"All Fields Test Descriptor", builder};

  // Set up memory mapped file.
  char const* file_name  = "./record-rw-test.bin";
  
  constexpr auto k_desired_file_size = 1024 * 1024; // 1 GB.
  auto const record_count = k_desired_file_size / desc.mem_size();
  size_t const file_size = record_count * desc.mem_size();

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
  REQUIRE(boost::alignment::is_aligned(field::k_record_alignment, mapped_file_addr));
  
  BENCHMARK_ADVANCED("generate file")(Catch::Benchmark::Chronometer meter)
  {
    // Generate records into filemapped memory.
    meter.measure(
      [=, &desc]() {
        generate_records((mem_t*)mapped_file_addr, desc, record_count);
      });
  };
  
  /*
  // Read records from file.
  auto mem = mem_alloc;
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
  */
}

} // namespace rdf