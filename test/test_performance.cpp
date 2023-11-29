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
  for (int64_t i = 0; i < (int64_t)count; ++i)
  {
    d.fields(0ul) .write<Key8>      (mem, fmt::format("_B[_{}d:{}d]", i, i + 1));
    d.fields(1ul) .write<Key16>     (mem, fmt::format("AAPL_{}:*", i % 10));
    d.fields(2ul) .write<String8>   (mem, fmt::format("_B[_{}d:{}d]", i + 2, i + 3));
    d.fields(3ul) .write<String16>  (mem, fmt::format("SPY_{}:*", i % 10));
    d.fields(4ul) .write<Timestamp> (mem, util::make_time(i * 10000));
    d.fields(5ul) .write<Char>      (mem, 33 + i % (127 - 33));
    d.fields(6ul) .write<Utf_Char8> (mem, (uint64_t)i % 256);
    d.fields(7ul) .write<Utf_Char16>(mem, (uint64_t)i % 512);
    d.fields(8ul) .write<Utf_Char32>(mem, (uint64_t)i % 1024);
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
  namespace bal = boost::alignment;

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

  // Size of the test data.
  constexpr auto k_desired_file_size = 1024 * 1024; // 1 MB.

  // Set up memory mapped file for generated data.
  char const* src_file_name  = "./rw-test-source.bin"; 
  auto const record_count = k_desired_file_size / desc.mem_size();
  size_t const file_size = record_count * desc.mem_size();

  // Remove file if it exists.
  ipc::file_mapping::remove(src_file_name);

  auto create_file = [](char const* name, size_t size) {  
    std::ofstream file{ name, std::ios_base::binary };
    file.seekp(size - 1, std::ios_base::beg);
    file.put(0);  // Seek to end of file and write a null byte.
  };

  // Create a file with desired size.
  create_file(src_file_name, file_size);

  // Create a file mapping.
  ipc::file_mapping src_mapping(src_file_name, ipc::read_write);  
  ipc::mapped_region src_region(src_mapping, ipc::read_write);    // Map the whole file with read-write permissions in this process.
  void* src_file_addr = src_region.get_address();                 // Get the address of the mapped region.
  REQUIRE(src_region.get_size() == file_size);
  REQUIRE(bal::is_aligned(field::k_record_alignment, src_file_addr));
  auto src_mem = (mem_t*)src_file_addr;

  //BENCHMARK_ADVANCED("generate file")(Catch::Benchmark::Chronometer meter)
  //{
    // Generate records into filemapped memory.
    //meter.measure(
      //[=, &desc]() {
        generate_records(src_mem, desc, record_count);
      //});
//  };

  src_region.flush(); // Flush the mapped region to disk.

  // Set up memory mapped file for copied data.
  char const* dest_file_name  = "./rw-test-dest.bin"; 

  // Remove file if it exists.
  ipc::file_mapping::remove(dest_file_name);

  // Create file with desired size.
  create_file(dest_file_name, file_size);

  // Create a file mapping.
  ipc::file_mapping dest_mapping(dest_file_name, ipc::read_write);
  ipc::mapped_region dest_region(dest_mapping, ipc::read_write);
  void* dest_file_addr = dest_region.get_address();
  REQUIRE(dest_region.get_size() == file_size);
  REQUIRE(bal::is_aligned(field::k_record_alignment, dest_file_addr));
  
  auto dest_mem = (mem_t*)dest_file_addr;
  auto read_mem = src_mem;
  auto const& d = desc;

  // Read records from file and write to a second file.
  for (int64_t i = 0; i < (int64_t)record_count; ++i)
  {
    auto record = rdf::record{read_mem};

    REQUIRE(record.get<Key8>      (d.fields(0ul))  == fmt::format("_B[_{}d:{}d]", i, i + 1));
    REQUIRE(record.get<Key16>     (d.fields(1ul))  == fmt::format("AAPL_{}:*", i % 10));
    REQUIRE(record.get<String8>   (d.fields(2ul))  == fmt::format("_B[_{}d:{}d]", i + 2, i + 3));
    REQUIRE(record.get<String16>  (d.fields(3ul))  == fmt::format("SPY_{}:*", i % 10));
    REQUIRE(record.get<Timestamp> (d.fields(4ul))  == util::make_time(i * 10000));
    REQUIRE(record.get<Char>      (d.fields(5ul))  == 33 + i % (127 - 33));
    REQUIRE(record.get<Utf_Char8> (d.fields(6ul))  == (char8_t)((uint64_t)i % 256));
    REQUIRE(record.get<Utf_Char16>(d.fields(7ul))  == (char16_t)((uint64_t)i % 512));
    REQUIRE(record.get<Utf_Char32>(d.fields(8ul))  == (char32_t)((uint64_t)i % 1024));
    REQUIRE(record.get<Int8>      (d.fields(9ul))  == (int8_t)(-i << 0));
    REQUIRE(record.get<Int16>     (d.fields(10ul)) == (int16_t)(-i << 1));
    REQUIRE(record.get<Int32>     (d.fields(11ul)) == (int32_t)(-i << 2));
    REQUIRE(record.get<Int64>     (d.fields(12ul)) == (int64_t)(-i << 3));
    REQUIRE(record.get<Uint8>     (d.fields(13ul)) == (uint8_t)((uint64_t)i << 0));
    REQUIRE(record.get<Uint16>    (d.fields(14ul)) == (uint16_t)((uint64_t)i << 1));
    REQUIRE(record.get<Uint32>    (d.fields(15ul)) == (uint32_t)((uint64_t)i << 2));
    REQUIRE(record.get<Uint64>    (d.fields(16ul)) == (uint64_t)((uint64_t)i << 3));
    REQUIRE(record.get<Float16>   (d.fields(17ul)) == (std::float16_t)i * 1.f16);
    REQUIRE(record.get<Float32>   (d.fields(18ul)) == (std::float32_t)i * 10.f32);
    REQUIRE(record.get<Float64>   (d.fields(19ul)) == (std::float64_t)i * 100.f64);
    REQUIRE(record.get<Float128>  (d.fields(20ul)) == (std::float128_t)i * 1000.f128);
    REQUIRE(record.get<Bool>      (d.fields(21ul)) == i % 2);

    read_mem += d.mem_size();
  }
}

} // namespace rdf