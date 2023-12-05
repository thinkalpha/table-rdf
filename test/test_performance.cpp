#include "log.h"
#include <table-rdf/rdf.h>
#include <table-rdf/traits.h>

#include <catch2/catch.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#if !TRDF_HAS_CHRONO_PARSE
  #include <date/date.h>
#endif
#include <oneapi/tbb.h>
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
    d.fields(4ul) .write<Timestamp> (mem, util::make_timestamp(i * 10000));
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

// inline void read_write_all_fields(mem_t* write_mem, mem_t const* read_mem, descriptor const& d)
// {
#define FIELDS_READ_WRITE(wm, rm, d) \
  d.fields(0ul).write<Key8>      ( wm, d.fields(0ul).read<Key8>      (rm) ); \
  d.fields(1ul).write<Key16>     ( wm, d.fields(1ul).read<Key16>     (rm) ); \
  d.fields(2ul).write<String8>   ( wm, d.fields(2ul).read<String8>   (rm) ); \
  d.fields(3ul).write<String16>  ( wm, d.fields(3ul).read<String16>  (rm) ); \
  d.fields(4ul).write<Timestamp> ( wm, d.fields(4ul).read<Timestamp> (rm) ); \
  d.fields(5ul).write<Char>      ( wm, d.fields(5ul).read<Char>      (rm) ); \
  d.fields(6ul).write<Utf_Char8> ( wm, d.fields(6ul).read<Utf_Char8> (rm) ); \
  d.fields(7ul).write<Utf_Char16>( wm, d.fields(7ul).read<Utf_Char16>(rm) ); \
  d.fields(8ul).write<Utf_Char32>( wm, d.fields(8ul).read<Utf_Char32>(rm) ); \
  d.fields(9ul).write<Int8>      ( wm, d.fields(9ul).read<Int8>      (rm) ); \
  d.fields(10ul).write<Int16>    ( wm, d.fields(10ul).read<Int16>    (rm) ); \
  d.fields(11ul).write<Int32>    ( wm, d.fields(11ul).read<Int32>    (rm) ); \
  d.fields(12ul).write<Int64>    ( wm, d.fields(12ul).read<Int64>    (rm) ); \
  d.fields(13ul).write<Uint8>    ( wm, d.fields(13ul).read<Uint8>    (rm) ); \
  d.fields(14ul).write<Uint16>   ( wm, d.fields(14ul).read<Uint16>   (rm) ); \
  d.fields(15ul).write<Uint32>   ( wm, d.fields(15ul).read<Uint32>   (rm) ); \
  d.fields(16ul).write<Uint64>   ( wm, d.fields(16ul).read<Uint64>   (rm) ); \
  d.fields(17ul).write<Float16>  ( wm, d.fields(17ul).read<Float16>  (rm) ); \
  d.fields(18ul).write<Float32>  ( wm, d.fields(18ul).read<Float32>  (rm) ); \
  d.fields(19ul).write<Float64>  ( wm, d.fields(19ul).read<Float64>  (rm) ); \
  d.fields(20ul).write<Float128> ( wm, d.fields(20ul).read<Float128> (rm) ); \
  d.fields(21ul).write<Bool>     ( wm, d.fields(21ul).read<Bool>     (rm) );
// }

TEST_CASE( "read/write", "[!benchmark]" )
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
  constexpr auto k_desired_file_size = 1024 * 1024 * 1024; // 1 GB.

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
  REQUIRE(bal::is_aligned(desc.mem_align(), src_file_addr));
  auto const src_mem = (mem_t*)src_file_addr;

  generate_records(src_mem, desc, record_count);

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
  REQUIRE(bal::is_aligned(desc.mem_align(), dest_file_addr));

  SECTION("tbb rdf (field read / write)")
  {
    // Read records from file and write to a second file.
    BENCHMARK_ADVANCED("tbb rdf (field read / write)")(Catch::Benchmark::Chronometer meter)
    {
      meter.measure(
        [=, &desc]()
        {
          auto write_mem = (mem_t*)dest_file_addr;
          mem_t* read_mem = src_mem;
          auto const& d = desc;

          tbb::parallel_for(tbb::blocked_range<int64_t>(0, record_count),
            [&, read_mem, write_mem](const tbb::blocked_range<int64_t>& range)
            {
              for (int64_t i = range.begin(); i != range.end(); ++i)
              {
                auto const wm = write_mem + i * d.mem_size();
                auto const rm = read_mem + i * d.mem_size();
                FIELDS_READ_WRITE(wm, rm, d);
              }
          });
        });
    };

    REQUIRE(std::memcmp(src_file_addr, dest_file_addr, file_size) == 0);
  }

  SECTION("trivial")
  {
    BENCHMARK_ADVANCED("trivial copy")(Catch::Benchmark::Chronometer meter)
    {
      meter.measure(
        [=]()
        {
          std::memcpy(dest_file_addr, src_file_addr, file_size);
        });
    };

    REQUIRE(std::memcmp(src_file_addr, dest_file_addr, file_size) == 0);
  }

  SECTION("rdf (field read / write)")
  {
    // Read records from file and write to a second file.
    BENCHMARK_ADVANCED("rdf (field read / write)")(Catch::Benchmark::Chronometer meter)
    {
      meter.measure(
        [=, &desc]()
        {
          auto write_mem = (mem_t*)dest_file_addr;
          mem_t* read_mem = src_mem;
          auto const& d = desc;

          for (int64_t i = 0; i < (int64_t)record_count; ++i)
          {
            FIELDS_READ_WRITE(write_mem, read_mem, d);

            read_mem += d.mem_size();
            write_mem += d.mem_size();
          }
        });
    };

    REQUIRE(std::memcmp(src_file_addr, dest_file_addr, file_size) == 0);
  }

  SECTION("rdf (record read / field write)")
  {
    // Read records from file and write to a second file.
    BENCHMARK_ADVANCED("rdf (record read / field write)")(Catch::Benchmark::Chronometer meter)
    {
      meter.measure(
        [=, &desc]()
        {
          auto write_mem = (mem_t*)dest_file_addr;
          mem_t* read_mem = src_mem;
          auto const& d = desc;

          for (int64_t i = 0; i < (int64_t)record_count; ++i)
          {
            auto record = rdf::record{read_mem};

            d.fields(0ul).write<Key8>      ( write_mem, record.get<Key8>      (d.fields(0ul)) );
            d.fields(1ul).write<Key16>     ( write_mem, record.get<Key16>     (d.fields(1ul)) );
            d.fields(2ul).write<String8>   ( write_mem, record.get<String8>   (d.fields(2ul)) );
            d.fields(3ul).write<String16>  ( write_mem, record.get<String16>  (d.fields(3ul)) );
            d.fields(4ul).write<Timestamp> ( write_mem, record.get<Timestamp> (d.fields(4ul)) );
            d.fields(5ul).write<Char>      ( write_mem, record.get<Char>      (d.fields(5ul)) );
            d.fields(6ul).write<Utf_Char8> ( write_mem, record.get<Utf_Char8> (d.fields(6ul)) );
            d.fields(7ul).write<Utf_Char16>( write_mem, record.get<Utf_Char16>(d.fields(7ul)) );
            d.fields(8ul).write<Utf_Char32>( write_mem, record.get<Utf_Char32>(d.fields(8ul)) );
            d.fields(9ul).write<Int8>      ( write_mem, record.get<Int8>      (d.fields(9ul)) );
            d.fields(10ul).write<Int16>    ( write_mem, record.get<Int16>     (d.fields(10ul)) );
            d.fields(11ul).write<Int32>    ( write_mem, record.get<Int32>     (d.fields(11ul)) );
            d.fields(12ul).write<Int64>    ( write_mem, record.get<Int64>     (d.fields(12ul)) );
            d.fields(13ul).write<Uint8>    ( write_mem, record.get<Uint8>     (d.fields(13ul)) );
            d.fields(14ul).write<Uint16>   ( write_mem, record.get<Uint16>    (d.fields(14ul)) );
            d.fields(15ul).write<Uint32>   ( write_mem, record.get<Uint32>    (d.fields(15ul)) );
            d.fields(16ul).write<Uint64>   ( write_mem, record.get<Uint64>    (d.fields(16ul)) );
            d.fields(17ul).write<Float16>  ( write_mem, record.get<Float16>   (d.fields(17ul)) );
            d.fields(18ul).write<Float32>  ( write_mem, record.get<Float32>   (d.fields(18ul)) );
            d.fields(19ul).write<Float64>  ( write_mem, record.get<Float64>   (d.fields(19ul)) );
            d.fields(20ul).write<Float128> ( write_mem, record.get<Float128>  (d.fields(20ul)) );
            d.fields(21ul).write<Bool>     ( write_mem, record.get<Bool>      (d.fields(21ul)) );

            read_mem += d.mem_size();
            write_mem += d.mem_size();
          }
        });
    };

    REQUIRE(std::memcmp(src_file_addr, dest_file_addr, file_size) == 0);
  }
}

} // namespace rdf