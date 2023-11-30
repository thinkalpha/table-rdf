#pragma once

#if !TRDF_HAS_CHRONO_PARSE
  #include <date/date.h>
#endif

#include <fmt/core.h>

namespace rdf {
namespace util {

template<class> inline constexpr bool always_false_v = false;
template<class> inline constexpr bool always_true_v = true;

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

inline std::string time_to_str(timestamp_t ts, std::string_view format = k_time_to_str_fmt)
{
  return std::vformat(format.data(), std::make_format_args(ts));
}

// Helper class for timestamp_t <--> string conversions.
class time_fmt
{
public:
  time_fmt(std::string const& time_fmt)
    : time_fmt_{time_fmt},
      str_fmt_{fmt::format("{{:{}}}", time_fmt_)}    // std::chrono parse function uses a different syntax to the std::format function.
  {}

  timestamp_t to_time(std::string_view sv) const
  {
    return util::str_to_time(sv, time_fmt_);
  }

  std::string to_str(timestamp_t ts) const
  {
    return util::time_to_str(ts, str_fmt_);
  }

private:
  std::string const time_fmt_;
  std::string const str_fmt_;
};

inline timestamp_t make_timestamp(raw_time_t raw_time)
{
  return timestamp_t{ timestamp_t::duration{ raw_time } };
}

} // namespace util
} // namespace rdf