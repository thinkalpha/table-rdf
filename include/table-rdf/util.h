#pragma once

#if !TRDF_HAS_CHRONO_PARSE
  #include <date/date.h>
#endif

#include <fmt/core.h>

namespace rdf {
namespace util {

//
// Timestamp parsing and printing.
//
inline timestamp_t str_to_time(std::string_view sv, std::string_view format)
{
  timestamp_t ts;
  std::istringstream ss{std::string{sv}};
  ss >> CHRONO_PARSE_NAMESPACE::parse(format.data(), ts);
  if (ss.fail()) {
    throw std::runtime_error{fmt::format("failed to parse timestamp string '{}' with format '{}'", sv, format)};
  }
  return ts;
}

inline std::string time_to_str(timestamp_t ts, std::string_view format)
{
  return std::vformat(format.data(), std::make_format_args(ts));
}

} // namespace util
} // namespace rdf