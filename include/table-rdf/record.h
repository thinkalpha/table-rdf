#pragma once
#include "descriptor.h"

#include <fmt/core.h>

#include <span>
#include <ranges>

namespace rdf
{
// Base record type.
class record final
{
public:
  record(mem_t const* mem)
    : mem_{mem}
  {
  };

  ~record() = default;

  template<typename T> T const* get_ptr(field::offset_t o) const {
    T const* value = reinterpret_cast<T const*>(mem_ + o);
    return value;
  }

  template<typename T> T get(field::offset_t o) const {
    T const value = *get_ptr<T>(o);
    return value;
  }

  // TODO: Vlad says records don't contain a key. Probably since this is redundant data that can be expressed via collections of records by key.  
  key_t       key(descriptor const& d) const;
  timestamp_t timestamp(descriptor const& d) const;
  std::string timestamp_str(descriptor const& d) const;

  mem_t const* raw() const { return mem_; };

  // Debugging.
  std::string to_string(descriptor const& desc) const;

private:
  mem_t const* mem_;
};

inline auto mem_to_record(mem_t const& mem) {
  return record{&mem};
}

template<> inline std::string_view record::get(field::offset_t o) const {
  // TODO: Make more robust with type traits.
  auto const length = get<uint8_t>(o);
  return std::string_view(get_ptr<char>(o + sizeof(uint8_t)), length);
}

inline key_t record::key(descriptor const& d) const
{
  return get<key_t>(d.key_o());
}

inline timestamp_t record::timestamp(descriptor const& d) const
{
  auto const raw = get<raw_time_t>(d.timestamp_o());
  return timestamp_t{ rdf::timestamp_t::duration{ raw } };
}

inline std::string record::timestamp_str(descriptor const& d) const
{
  return d.time_to_str(timestamp(d));
}

inline std::string record::to_string(descriptor const& desc) const
{
  auto out = fmt::memory_buffer();
  auto out_it = std::back_inserter(out);
  for (auto f : desc.fields()) {
    switch(f.type_) {
      case types::Key8:
        fmt::format_to(out_it, f.fmt_, key(desc));
        break;
      case types::Timestamp:
        fmt::format_to(out_it, f.fmt_, timestamp_str(desc));
        break;
      case types::Char:
        fmt::format_to(out_it, f.fmt_, get<char>(f.offset_));
        break;
      case types::Int32:
        fmt::format_to(out_it, f.fmt_, get<int32_t>(f.offset_));
        break;
      case types::Float32:
        fmt::format_to(out_it, f.fmt_, get<float>(f.offset_));
        break;
      case types::Int64:
        fmt::format_to(out_it, f.fmt_, get<int64_t>(f.offset_));
        break;
      case types::Float64:
        fmt::format_to(out_it, f.fmt_, get<double>(f.offset_));
        break;
      case types::String8:
        fmt::format_to(out_it, f.fmt_, get<std::string_view>(f.offset_));
        break;
      case types::Bool:
        fmt::format_to(out_it, f.fmt_, get<bool>(f.offset_));
        break;
      default:
        BOOST_ASSERT(false);
        break;
    }
  }
  return fmt::to_string(out);
}

}  // namespace rdf



