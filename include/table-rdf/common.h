#pragma once
#include <chrono>
#include <string_view>

namespace rdf
{
  using namespace std::chrono_literals;

  using mem_t = std::byte;
  // TODO: std::span<> has some shortcomings:
  //    - lack of cbegin / cend (added back in for C++23 but not implemented in MSVC 10.0.19041.0).
  //    - lack of operator== for no good reason imo.
  // TODO: Would it be possible to use a static-extent span with dynamic descriptors.
  // TODO: std::span also cannot have a runtime stride over it's data. We need a custom type.
  using mspan = std::span<mem_t const>;

  using timestamp_t = std::chrono::sys_time<std::chrono::nanoseconds>;
  using raw_time_t = timestamp_t::duration::rep;

  // Sanity checks.
  static_assert(std::is_same_v<timestamp_t::rep, timestamp_t::duration::rep>);

  using string_t = std::string_view;
  using key_t = string_t;

  #if TRDF_HAS_CHRONO_PARSE
    #define CHRONO_PARSE_NAMESPACE std::chrono
  #else
    #define CHRONO_PARSE_NAMESPACE date
  #endif

  // TODO: Current system uses this "key plus" structure that includes both a symbol key and sec ID.
  // struct key_plus_t {
  //   string_t symbol;
  //   uint64_t security_id;
  // };

} // namespace rdf