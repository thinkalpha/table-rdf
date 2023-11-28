#pragma once

#include "types.h"
#include "traits.h"

#include <fmt/compile.h>
#include <boost/assert.hpp>
#include <boost/align/align_up.hpp>
#include <boost/align/is_aligned.hpp>

#include <string>
#include <vector>

using namespace rdf::types;

namespace rdf {

struct field
{
  using offset_t = uintptr_t;
  using index_t = size_t;
  using name_t = std::string;
  using description_t = std::string;

  static constexpr offset_t k_null_offset = offset_t(-1);
  static constexpr index_t  k_null_index = index_t(-1);
  static constexpr size_t   k_no_payload = 0;

  // TODO: On GCC this is 16 due to float128 being considered a primitive type.
  // We should instead compute this based on the max alignment in field::k_type_props.
  static constexpr size_t   k_record_alignment = sizeof(max_align_t);

  field(name_t name,
        description_t description,
        types::type type,
        // Records must be fixed-size, so e.g. string types define a maximum string length (payload).
        size_t payload = k_no_payload,
        // Format for printing the field for logging. Default "{>:16}" is right-aligned, padded to 16 characters.
        fmt::basic_runtime<char> fmt = fmt::runtime("{:>16}"),
        offset_t offset = k_null_offset,
        index_t index = k_null_index)
    : name_{name},
      description_{description},
      type_{type},
      payload_{payload},
      fmt_{fmt},
      offset_{offset},
      index_{index}
  {
    BOOST_ASSERT_MSG(!string_type(type_) || payload_ != k_no_payload, "string types require a maximum payload size to be specified");
  }

  template <type T> void             write(mem_t* base, value_t<T> const value) const;
  template <type T> value_t<T> const read(mem_t const* base) const;

  auto type_size() const { return types::k_type_props[type_].size_; }
  auto payload() const { return payload_; }
  auto size() const { return type_size() + payload(); }
  auto align() const { return types::k_type_props[type_].alignment_; }
  auto offset() const { return offset_; }
  auto index() const { return index_; }

private:
  template<type T> void             write_str(mem_t* base, value_t<T> value) const;
  template<type T> value_t<T> const read_str(mem_t const* base) const;

  template<class V>  V const* offset_ptr(mem_t const* base) const;
  template<class V>  V*       offset_ptr(mem_t* base) const;

  template <types::type T> void validate() const;

public:
  name_t const name_;
  description_t const description_;
  types::type const type_;
  size_t const payload_;
  fmt::basic_runtime<char> const fmt_;  // fmt library format specifier for printing (https://hackingcpp.com/cpp/libs/fmt.html).
  offset_t offset_;                     // TODO: Keep this const.
  index_t index_;
};

} // namespace rdf

#include "field.tpp"