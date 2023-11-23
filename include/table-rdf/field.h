#pragma once

#include "types.h"

#include <fmt/compile.h>
#include <boost/assert.hpp>
#include <boost/align/align_up.hpp>
#include <boost/align/is_aligned.hpp>

#include <string>
#include <vector>
#include <ranges>

namespace rdf {

namespace concepts {
  template <typename T> concept string_size_prefix = std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>;
  template <typename T> concept numeric = std::is_floating_point_v<T> || std::is_integral_v<T>;
}

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
        size_t payload = 0,
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
    BOOST_ASSERT_MSG(!is_string_type(type_) || payload_ != k_no_payload, "string types require a maximum payload size to be specified");
  }

  // TODO: No point to these being constexpr (unless object declaration is constexpr)?
  constexpr auto type_size() const { return types::k_type_props[type_].size_; }
  constexpr auto payload() const { return payload_; }
  constexpr auto size() const { return type_size() + payload(); }
  constexpr auto align() const { return types::k_type_props[type_].alignment_; }
  auto offset() const { return offset_; }
  auto index() const { return index_; }

  template <types::type T, concepts::numeric V>
  inline void write(mem_t* const record_start, V const value) const;

  template <types::type T>
  inline void write(mem_t* const record_start, string_t const value) const;

private:
  template<concepts::string_size_prefix T>
  inline void write_string(mem_t* const record_start, string_t const value) const;

public:
  name_t const name_;
  description_t const description_;
  types::type const type_;
  size_t const payload_;
  // fmt library format specifier for printing (https://hackingcpp.com/cpp/libs/fmt.html).
  fmt::basic_runtime<char> const fmt_;
  offset_t offset_;     // TODO: Keep this const.
  index_t index_;
};

template <types::type T, concepts::numeric V>
void field::write(mem_t* const record_start, V const value) const
{
  BOOST_ASSERT(payload() == 0);
  BOOST_ASSERT_MSG(sizeof(V) == size(), fmt::format("field '{}' = {}B which does not match sizeof({}) = {}B",
                                                           name_, size(), typeid(V).name(), sizeof(V)).c_str());

  mem_t* const mem = record_start + offset();
  BOOST_ASSERT(boost::alignment::is_aligned(align(), mem));

  // new (mem) V{value};
  *reinterpret_cast<V*>(mem) = value;
}

template<concepts::string_size_prefix T> 
void field::write_string(mem_t* const record_start, string_t const value) const
{
  BOOST_ASSERT(payload() > 0);

  using char_type = string_t::value_type;
  auto const size = value.size() * sizeof(char_type);
  if (size > payload()) {
    throw std::runtime_error("string too large for payload");
  }

  if (size != (T)size) {
    throw std::runtime_error("string lengths must fit in 8-bits");
  }

  mem_t* const mem = record_start + offset();
  BOOST_ASSERT(boost::alignment::is_aligned(align(), mem));
  auto const char_mem = mem + sizeof(T);
  *reinterpret_cast<uint8_t*>(mem) = (T)size;

  BOOST_ASSERT(boost::alignment::is_aligned(alignof(char_type), char_mem));
  std::memcpy(char_mem, value.data(), size);
}

template<>
void field::write<types::String8>(mem_t* const record_start, string_t const value) const
{
  write_string<uint8_t>(record_start, value);
}

template<>
void field::write<types::String16>(mem_t* const record_start, string_t const value) const
{
  write_string<uint16_t>(record_start, value);
}

template<>
void field::write<types::Key8>(mem_t* const record_start, string_t const value) const
{
  write_string<uint8_t>(record_start, value);
}

template<>
void field::write<types::Key16>(mem_t* const record_start, string_t const value) const
{
  write_string<uint16_t>(record_start, value);
}

// Helper that calculates offsets as fields are added, taking the field alignment property into account.
struct fields_builder
{
  fields_builder& push(field const& f)
  {
    BOOST_ASSERT_MSG(f.offset_ == field::k_null_offset, "field has explicit offset, don't use fields_builder");
    BOOST_ASSERT_MSG(f.index_ == field::k_null_index, "field has explicit index, don't use fields_builder");
    fields_.push_back(f);

    auto& fl = fields_.back();                // Last field.
    fl.offset_ = 0;
    fl.index_ = fields_.size() - 1;
    if (fields_.size() > 1)
    {
      auto& fp = fields_[fields_.size() - 2];   // 2nd last field.

      // TODO: This assumes the base address is aligned to at least the maximum alignment of all fields.
      // Currently this is the case as memory.h aligns to max_align_t. But we should assert this in the record accessors.
      auto offset_ptr = (void*)(fp.offset_ + fp.size());
      fl.offset_ = (field::offset_t)boost::alignment::align_up(offset_ptr, fl.align());
    }
    return *this;
  }

  auto& get() const { return fields_; }

private:
  std::vector<field> fields_;
};

static constexpr bool check_types() {
  for (auto const [i, f] : std::views::enumerate(types::k_type_props)) { 
    if (f.type_ != i) return false;
  }
  return true;
}

static_assert(check_types(), "order of field::k_type_props does not match field::types::type enum");

} // namespace rdf