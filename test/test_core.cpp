#include "log.h"
#include "memory.h"
#include "window.h"
#include "rdf/descriptor.h"
#include "csv/csv_stream.h"

#include <catch2/catch.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_map.hpp>
#if !TRDF_HAS_CHRONO_PARSE
  #include <date/date.h>
#endif

#include <ranges>

namespace tds {

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

TEST_CASE( "memory", "[core]" )
{
  SECTION ( "basic" )
  {
    using tds::record_pool;
    constexpr auto k_record_sz = 64, k_pool_sz = 2 * 64;
    record_pool pool{k_pool_sz, k_record_sz};
    auto b = std::static_pointer_cast<tds::pooled_block>(pool.create_block());

    REQUIRE(b->used() == 0);
    REQUIRE(b->available() == pool.block_size());

    auto m1 = b->alloc();

    REQUIRE(b->used() == k_record_sz);
    REQUIRE(b->available() == k_record_sz);

    auto m2 = b->alloc();

    REQUIRE(b->used() == pool.block_size());
    REQUIRE(b->available() == 0);

    REQUIRE_THROWS_AS(b->alloc(), std::bad_alloc);   // Block is full.
    REQUIRE_THROWS_AS(b->alloc(), std::bad_alloc);

    REQUIRE(b->used() == pool.block_size());
    REQUIRE(b->available() == 0);

    REQUIRE_THROWS_WITH(b->free(m1), Catch::Matchers::Contains("pooled_block memory must be freed in reverse order"));
    REQUIRE_THROWS_WITH(b->free(m1 - 1000), Catch::Matchers::Contains("not from this block"));
    REQUIRE_THROWS_WITH(b->free(m2 + 1000), Catch::Matchers::Contains("not from this block"));

    b->free(m2);

    REQUIRE(b->used() == pool.record_size());
    REQUIRE(b->available() == pool.record_size());

    auto m3 = b->alloc();
    REQUIRE(m3 == m2);
    REQUIRE(b->used() == pool.block_size());
    REQUIRE(b->available() == 0);

    b->free(m3);
    b->free(m1);

    REQUIRE(b->used() == 0);
    REQUIRE(b->available() == pool.block_size());
  }

#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
  {
    constexpr auto k_count = 1000;
    record_pool pool{128 * k_count, 128};
    std::vector<rdf::mem_t*> allocs;
    allocs.reserve(k_count);
    BENCHMARK("pooled_block allocation usage") {
      allocs.clear();
      auto b = std::static_pointer_cast<tds::pooled_block>(pool.create_block());
      for (auto i : std::views::iota(0, k_count)) {
        (void)i;
        auto* p = b->alloc();
        new (p) char[128];
        allocs.push_back(p);
      }
      for (auto p : allocs | std::ranges::views::reverse) {
        b->free(p);
      }
    };
    {
      auto b = std::static_pointer_cast<tds::pooled_block>(pool.create_block());
      BENCHMARK("pooled_block record alloc / free") {
        auto* p = b->alloc();
        b->free(p);
      };
    }
  }

  // TODO: Figure out how the default allocators are faster than the pooled allocators.
  BENCHMARK("shared_ptr default allocator") {
    auto p = std::make_shared<char>();
  };

  using char_allocator = boost::pool_allocator<char>;
  char_allocator alloc;
  BENCHMARK("shared_ptr pooled allocator") {
    auto p = std::allocate_shared<char, char_allocator>(alloc);
  };
  

  BENCHMARK("default allocator") {
    for (int i = 0; i < 1000; ++i) {
      auto p = new char(0);
      delete p;
    }
  };

  char_allocator alloc2;
  BENCHMARK("pooled allocator") {
    for (int i = 0; i < 1000; ++i) {
      auto p = char_allocator::allocate(1);
      char_allocator::deallocate(p, 1);
    }
  };
#endif  
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

  SECTION( "record write / read")
  {
    constexpr auto k_count = 10;
    record_pool pool{d.mem_size() * k_count, d.mem_size()};
    pool.begin_allocs();
      auto fields = d.fields();
      // Write to record memory.
      for (int i = 0; i < k_count; ++i) {
        rdf::raw_time_t raw_time = time.time_since_epoch().count();
        auto mem = pool.alloc();
        field::write<rdf::string_t>(mem, fields[0], "_B[_1d:3d]");
        field::write<rdf::key_t>(mem, fields[1], "AAPL:*");
        field::write<rdf::raw_time_t>(mem, fields[2], raw_time);
        field::write<int32_t>(mem, fields[3], i);
        time += 1s; 
      }
      timespan_t ts{time0, time};
    auto alloc_info = pool.end_allocs();

    auto blocks = alloc_info.blocks_;
    REQUIRE(blocks.size() == 1);
    REQUIRE(blocks.back()->used() == d.mem_size() * k_count);
    REQUIRE(blocks.back()->stride() == d.mem_size());

    SECTION( "record_span" )
    {
      // Make record_spans.
      auto span_ptrs = record_span::make_spans(alloc_info, ts, d, 1ns);


      
      auto r = span_ptrs.front()->front();
      REQUIRE(r.key(d) == "AAPL:*");
      REQUIRE(r.timestamp(d) == time0);

      SECTION( "window" )
      {
        // Windows hold record_spans (not record_span::ptrs).
        // TODO: record_spans are lightweight and we only use record_span::ptr currently to 
        // make it easier to use boost::icl::interval_map. Consider removing record_span::ptr.

        std::vector<record_span> spans;
        for (auto s : span_ptrs) {
          spans.emplace_back(*s);
        }

        SECTION( "window over entire timespan of record_spans" )
        {
          auto w = window::create(std::move(spans), d,
                                  {"AAPL:*", 0, timestamp_t{0s}, timestamp_t::max()});

          auto c = 0;
          for (auto rec : *w) {
            SPDLOG_DEBUG("record 1 [{}]: {}, {:%T} UTC", c, rec.key(d), rec.timestamp(d).time_since_epoch());
            REQUIRE(rec.key(d) == "AAPL:*");
            ++c;
          }
          REQUIRE(c == k_count);
        }

        SECTION( "window over partial timespan" )
        {
          auto w = window::create(std::move(spans), d,
                                  {"AAPL:*", 0, time0 + 1s, time - 5s});

          auto c = 0;
          for (auto rec : *w) {
            SPDLOG_DEBUG("record 1 [{}]: {}, {:%T} UTC", c, rec.key(d), rec.timestamp(d).time_since_epoch());
            REQUIRE(rec.key(d) == "AAPL:*");
            ++c;
          }
          //REQUIRE(c == k_count);
        }

      }
    }
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
    b.push({ "symbol",    "", field::key_type })
     .push({ "timestamp", "", field::timestamp_type });
    rdf::descriptor d {"CSV Test Data Descriptor", "%Y%m%d %T",  b};
    auto now_str = d.time_to_str(now);
    auto now_parsed = d.str_to_time(now_str);
    REQUIRE(now == now_parsed);
  }
}

TEST_CASE( "csv_stream", "[core]" )
{
  SECTION( "test_data_simple_aapl.csv" )
  {
    using field = rdf::field;
    rdf::fields_builder b;
    b.push({"formula",    "formula description", field::string_type})
    .push({ "symbol",    "ticker symbol",       field::key_type})
    .push({ "timestamp", "utc timestamp",       field::timestamp_type, fmt::runtime("{:>40}") })
    .push({ "field 4",   "field 4 description", field::int32_type })
    .push({ "field 5",   "field 5 description", field::float32_type })
    .push({ "field 6",   "field 6 description", field::float64_type })
    .push({ "field 7",   "field 7 description", field::bool_type, fmt::runtime("{:>12}") })
    .push({ "field 8",   "field 8 description", field::int32_type });

    rdf::descriptor d {"CSV Test Data Descriptor", "%Y%m%d %T", b};
    SPDLOG_DEBUG(d.describe());  
    csv_stream csv{d, "./captures/test_data_simple_aapl.csv", 1ns};

    SPDLOG_WARN("iota: {}", csv.iota());

    auto window = csv.request(window::params{"AAPL:*", 0, timestamp_t{0s}, timestamp_t::max()});
    REQUIRE(window);
    window->log();
    // csv.request(window::params{"AAPL:*", 0, t + 1h, t + 2h});
  }

  SECTION( "BarRecord_1m-20231027.psv" )
  {
    // Corresponding xtl compile-time rdf::descriptor for BarRecord.
    /*
      template <> struct ContentBuilder<dictionary::BarRecord>
      {
          using SymKey_t = Common::Symbol;
          using Ordinal_tl = xtl::type_pack<
              //  dictionary::RecordStatus,
                dictionary::Time
              , dictionary::BarSequence
              , dictionary::BarOpen
              , dictionary::BarHigh
              , dictionary::BarLow
              , dictionary::BarClose
              , dictionary::BarVolume
              , dictionary::BarVWAPVolume
              , dictionary::BarVWAPNotional
              , dictionary::BarNotional
              , dictionary::BarLastBid
              , dictionary::BarLastBidSize
              , dictionary::BarLastAsk
              , dictionary::BarLastAskSize
              , dictionary::BarLastVolume
              , dictionary::BarTotalVolume
              , dictionary::BarTradeCount
          >;
      };
    */
    using field = rdf::field;
    rdf::fields_builder b;
    b.push({"Formula",          "Formula description",          field::string_type})
    .push({ "Symbol",           "Ticker Symbol",                field::key_type})
    .push({ "Timestamp",        "UTC Timestamp",                field::timestamp_type, fmt::runtime("{:>40}") })
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

    rdf::descriptor d {"BarRecord_1m-20231027.psv Descriptor", "%Y%m%d %T", b};
    // SPDLOG_DEBUG(d.describe());
    csv_stream csv_1m_bars{d, "./captures/BarRecord_1m-20231027.psv", 1min, '|'};

    SECTION("request all 'AAPL:*', fetch all") 
    {
      auto w1 = csv_1m_bars.request(window::params{"AAPL:*", 0, timestamp_t{0s}, timestamp_t::max()});
      REQUIRE(std::ranges::distance(*w1) == 960);

      SECTION("request time range 'AAPL:*', no fetch needed") 
      {
        auto const p = window::params{"AAPL:*", 0, "20231027 19:57:00", "20231027 19:59:00.000000001", d};
        REQUIRE(csv_1m_bars.is_cached(p));    // Previous request should have fetched and indexed the data for this.
        auto w2 = csv_1m_bars.request(p);
        REQUIRE(std::ranges::distance(*w2) == 3);   // Because the interval just goes into the 3rd minute, we expect 3 records.
        w2->log();
      }

      SECTION("request time range 'SPY:*', fetch range") 
      {
        auto const p = window::params{"SPY:*", 0, "20231027 12:00:00", "20231027 12:10:00", d};
        REQUIRE(!csv_1m_bars.is_cached(p));
        auto w3 = csv_1m_bars.request(p);
        REQUIRE(std::ranges::distance(*w3) == 10);    // We should have 10 records over a 10-minute right-open interval.
        w3->log();

        SECTION("request time range that overlaps previous fetch") 
        {
          csv_1m_bars.request({"SPY:*", 0, "20231027 12:00:00", "20231027 12:20:00", d})->log();

          SECTION("request time range within previous fetch, cache hit") 
          {
            REQUIRE(csv_1m_bars.is_cached({"SPY:*", 0, "20231027 12:05:00", "20231027 12:07:00", d}));
            csv_1m_bars.request({"SPY:*", 0, "20231027 12:05:00", "20231027 12:07:00", d})->log();
          }
        }
      }
    }
  }

}

} // namespace tds