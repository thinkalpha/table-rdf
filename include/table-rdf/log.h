#pragma once

#if TRDF_TEST
  #ifdef NDEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
  #else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
  #endif
#else
  #ifdef NDEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
  #else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
  #endif
#endif  

#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>

#if TRDF_TEST
  #include <catch2/catch.hpp>
#endif

#define SPDLOG_TS_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_TS(...) SPDLOG_TRACE(__VA_ARGS__) // For logging data from the table server.
#define SPDLOG_LVL(level) SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_##level

namespace tds {

inline void configure_logging()
{
  try
  {
    auto level = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

    // Console logging sink.
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);

  #if TRDF_FILE_LOGGING
    // Daily logging sink. Written to e.g. "px-daily_2021-10-06.log", for last 10 days.
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("px-daily.log", 23, 59, false, 10);
    file_sink->set_level(level);
  #endif

    // Set up the default logger with these sinks.
  #if TRDF_ASYNC_LOGGING
    spdlog::init_thread_pool(8192, 1);
    std::vector<spdlog::sink_ptr> sinks = 
    { 
    #if TRDF_FILE_LOGGING
      file_sink, 
    #endif
      console_sink 
    };
    auto logger = std::make_shared<spdlog::async_logger>("tds", sinks.begin(), sinks.end(),
                                                         spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
  #else
    spdlog::sinks_init_list sink_list =
    { 
    #if TRDF_FILE_LOGGING
      file_sink, 
    #endif
      console_sink 
    };
    auto logger = std::make_shared<spdlog::logger>("tds", sink_list);
  #endif

    spdlog::set_default_logger(logger);
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    
  #ifndef NDEBUG
    spdlog::flush_on(spdlog::level::trace);
  #else
    spdlog::flush_on(spdlog::level::err);
  #endif

    logger->set_pattern("[%T.%e] [%^%l%$] [%s:%#] %v");
  }
  catch (const spdlog::spdlog_ex& ex)
  {
      std::cerr << "Log initialization failed: " << ex.what() << std::endl;
  }
}

inline void finalize_logging()
{
  spdlog::shutdown();
}

//
// Debug helpers.
//
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
  #define DBG_PRINT_BUF(prefix, bufs, count) debug_print_buffer(prefix, bufs, count)

  inline void
  debug_print_buffer(char const* prefix, std::string const b, std::size_t count)
  {
    SPDLOG_DEBUG("--- {} ---", prefix);
    SPDLOG_DEBUG("{}", (b.length() == count ? b : b.substr(0, count)));
    SPDLOG_DEBUG("--- {} ---", count);
  }
#else
  #define DBG_PRINT_BUF(prefix, bufs, count) ((void)0)
#endif

} // namespace tds

namespace boost
{
    inline void assertion_failed(char const * expr, char const * function, char const * file, long line)
    {
      SPDLOG_ERROR("boost assert failed: '{}' in function {} @ {}:{}", expr, function, file, line);
      #if TRDF_TEST
        assert(false);
        FAIL();
      #else
        assert(false);
      #endif
    }

    inline void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line)
    {
      SPDLOG_ERROR("boost assert msg failed: {}\n\t'{}' in function {} @ {}:{}", msg, expr, function, file, line);
      #if TRDF_TEST
        assert(false);
        FAIL();
      #else
        assert(false);
      #endif
    }
} // namespace boost