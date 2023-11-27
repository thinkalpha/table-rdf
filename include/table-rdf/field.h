#pragma once

#include "types.h"

#include <fmt/compile.h>
#include <boost/assert.hpp>
#include <boost/align/align_up.hpp>
#include <boost/align/is_aligned.hpp>

#include <string>
#include <vector>

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

  auto type_size() const { return types::k_type_props[type_].size_; }
  auto payload() const { return payload_; }
  auto size() const { return type_size() + payload(); }
  auto align() const { return types::k_type_props[type_].alignment_; }
  auto offset() const { return offset_; }
  auto index() const { return index_; }

  template <types::type T, concepts::field_value V>
            void write(mem_t* base, V const value) const;

  template <types::type T, concepts::field_value V>
         V const read(mem_t const* base) const;

private:
  template<concepts::string_prefix P> void           write_str(mem_t* base, string_t const value) const;
  template<concepts::string_prefix P> string_t const read_str(mem_t const* base) const;

  template<typename T>  T const* offset_ptr(mem_t const* base) const;
  template<typename T>  T*       offset_ptr(mem_t* base) const;

  template <types::type T, typename V> void validate() const;

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

} // namespace rdf

#include "field.tpp"