#pragma once

#include <fmt/compile.h>
#include <boost/assert.hpp>
#include <boost/align/align_up.hpp>
#include <boost/align/is_aligned.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace rdf {

struct field
{
  using offset_t = uintptr_t;
  using index_t = size_t;
  using name_t = std::string;
  using description_t = std::string;

  static constexpr offset_t k_null_offset = offset_t(-1);
  static constexpr index_t  k_null_index = index_t(-1);
  static constexpr size_t   k_max_key_payload = 8;
  static constexpr size_t   k_max_string_payload = 64;

  // TODO: On GCC this is 16 due to float128 being considered a primitive type.
  // We should instead compute this based on the max alignment in field::k_types.
  static constexpr size_t   k_record_alignment = sizeof(max_align_t);

  enum d_type
  {
    key_type = 0,
    timestamp_type,
    char_type,
    int32_type,
    float32_type,
    int64_type,
    float64_type,
    string_type,
    bool_type,
    type_numof
  };

  // TODO: Use static inheritance heirarchy.
  struct desc_type {
    d_type const type_;
    char const* const name_;
    size_t const type_sz_;
    size_t const payload_sz_;
    size_t const alignment_;
  };

  // TODO: Use new <stdfloat> header with std::float32_t etc. if available?
  static constexpr std::array<desc_type, type_numof> k_types {
    desc_type{ key_type,       "*key*",  sizeof(key_size_prefix_t),    k_max_key_payload,    alignof(key_size_prefix_t) },
    desc_type{ timestamp_type, "tstamp", sizeof(raw_time_t),           0,                    alignof(raw_time_t) },
    desc_type{ char_type,      "char",   sizeof(char),                 0,                    alignof(char) },
    desc_type{ int32_type,     "i32",    sizeof(int32_t),              0,                    alignof(int32_t) },
    desc_type{ float32_type,   "f32",    sizeof(float),                0,                    alignof(float) },
    desc_type{ int64_type,     "i64",    sizeof(int64_t),              0,                    alignof(int64_t) },
    desc_type{ float64_type,   "f64",    sizeof(double),               0,                    alignof(double) },
    desc_type{ string_type,    "str",    sizeof(string_size_prefix_t), k_max_string_payload, alignof(string_size_prefix_t) },
    desc_type{ bool_type,      "bool",   sizeof(bool),                 0,                    alignof(bool) }
  };

  static inline char const* enum_names_type(d_type const t) {
    return k_types[t].name_;
  }

  field(name_t name,
        description_t description,
        d_type type,
        fmt::basic_runtime<char> fmt = fmt::runtime("{:>16}"),
        offset_t offset = k_null_offset,
        index_t index = k_null_index)
    : name_{name},
      description_{description},
      type_{type},
      fmt_{fmt},
      offset_{offset},
      index_{index}
  {
  
  }

  // TODO: No point to these being constexpr (unless object declaration is constexpr)?
  constexpr auto type_size() const { return k_types[type_].type_sz_; }
  constexpr auto payload() const { return k_types[type_].payload_sz_; }
  constexpr auto size() const { return type_size() + payload(); }
  constexpr auto align() const { return k_types[type_].alignment_; }
  auto offset() const { return offset_; }
  auto index() const { return index_; }

  template <std::semiregular T, typename... Args> static
  void write(mem_t* const record_base, field const& field, Args... args);

  name_t const name_;
  description_t const description_;
  d_type const type_;
  // fmt library format specifier for printing (https://hackingcpp.com/cpp/libs/fmt.html).
  fmt::basic_runtime<char> const fmt_;
  offset_t offset_;     // TODO: Keep this const.
  index_t index_;
};

template <std::semiregular T, typename... Args> inline
void field::write(mem_t* const record_base, field const& field, Args... args)
{
    mem_t* const mem = record_base + field.offset();
    BOOST_ASSERT(boost::alignment::is_aligned(field.align(), mem));

    // For string_view types, this *copies* the source string data into 
    // the record_block memory.
    //
    // Memory layout: |size prefix (u8, u16)|char data for string_view...|
    //                |<--------------- field.size() ------------------->|
    // TODO: Uses 1-byte for strings < 256 bytes, otherwise 2-bytes.
    if constexpr (std::is_same_v<T, std::string_view>)
    {
      static_assert(std::is_same_v<string_t, std::string_view>);
      auto const payload = field.payload(); 
      BOOST_ASSERT(payload > 0);

      using char_type = std::string_view::value_type;
      std::string_view const tmp{args...};

      auto const size = tmp.size() * sizeof(char_type);
      if (size > payload) {
        throw std::runtime_error("string too large for payload");
      }

      // This could be constexpr when we add type traits and also adapt to uint16_t prefix.
      if (size != (uint8_t)size) {
        throw std::runtime_error("string lengths must fit in 8-bits");
      }
      auto const char_mem = mem + sizeof(uint8_t);
      *reinterpret_cast<uint8_t*>(mem) = (uint8_t)size;

      BOOST_ASSERT(boost::alignment::is_aligned(alignof(char_type), char_mem));
      std::memcpy(char_mem, tmp.data(), size);
    }
    // Non-string types.
    else {
      BOOST_ASSERT(field.payload() == 0);
      BOOST_ASSERT_MSG(sizeof(T) == field.size(), fmt::format("field '{}' = {}B which does not match sizeof({}) = {}B",
                                                              field.name_, field.size(), typeid(T).name(), sizeof(T)).c_str());
      new (mem) T{args...};
    }
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

} // namespace rdf