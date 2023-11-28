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

  template <types::type T>
  inline value_t<T> const get(const field& f) const
  {
    return f.read<T>(memory());
  }

  mem_t const* memory() const;
  std::string  to_string(descriptor const& desc) const;

private:
  mem_t const* mem_;
};

inline auto mem_to_record(mem_t const& mem) {
  return record{&mem};
}

mem_t const* record::memory() const
{ 
  return mem_; 
};

inline std::string record::to_string(descriptor const& desc) const
{
  auto out = fmt::memory_buffer();
  auto out_it = std::back_inserter(out);

  #define FMT_FIELD(F) \
    case F: \
      out_it = fmt::format_to(out_it, f.fmt_, get<F>(f)); \
      break;

  for (auto f : desc.fields()) {
    switch(f.type_)
    {
      FMT_FIELD(Key8);
      FMT_FIELD(Key16);
      FMT_FIELD(String8);
      FMT_FIELD(String16);
      case Timestamp: {
        out_it = fmt::format_to(out_it, f.fmt_, util::time_to_str(get<Timestamp>(f)));
        break;
      }
      FMT_FIELD(Char);
      case Utf_Char8: {
        out_it = fmt::format_to(out_it, f.fmt_, (uint64_t)get<Utf_Char8>(f));  // TODO: Fix this temporary hack.
        break;
      }
      case Utf_Char16: {
        out_it = fmt::format_to(out_it, f.fmt_, (uint64_t)get<Utf_Char16>(f));  // TODO: Fix this temporary hack.
        break;
      }
      case Utf_Char32: {
        out_it = fmt::format_to(out_it, f.fmt_, (uint64_t)get<Utf_Char32>(f));  // TODO: Fix this temporary hack.
        break;
      }
      FMT_FIELD(Int8);
      FMT_FIELD(Int16);
      FMT_FIELD(Int32);
      FMT_FIELD(Int64);
      FMT_FIELD(Uint8);
      FMT_FIELD(Uint16);
      FMT_FIELD(Uint32);
      FMT_FIELD(Uint64);
      case Float16: {
        out_it = fmt::format_to(out_it, f.fmt_, (float)get<Float16>(f));        // TODO: Fix this temporary hack.
        break;
      }
      FMT_FIELD(Float32);
      FMT_FIELD(Float64);
      case Float128: {
        out_it = fmt::format_to(out_it, f.fmt_, (double)get<Float128>(f));      // TODO: Fix this temporary hack.
        break;
      }
      FMT_FIELD(Bool);
      case type_numof:
        BOOST_ASSERT_MSG(false, "invalid field type");
        break;
    }
  }

  return fmt::to_string(out);
}

}  // namespace rdf



