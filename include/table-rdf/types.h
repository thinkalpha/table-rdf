#pragma once
#include <cstddef>
#include <cstdint>
#include <stdfloat>
#include <array>

namespace rdf {
namespace concepts {
  template <typename T> concept string_prefix = std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>;
  
  // TODO: Elaborate numeric so that it is precisely the primitive types that we map to in types::k_type_props.
  template <typename T> concept numeric = std::is_floating_point_v<T> || std::is_integral_v<T>;
  template <typename T> concept string = std::is_convertible_v<T, string_t>;
  template <typename T> concept field_value = numeric<T> || string<T>;
}
namespace types {

// TODO: Consider using type traits.
enum type
{
  Key8 = 0,   // Key string with 8-bit size prefix.
  Key16,      // Key string with 16-bit size prefix.
  String8,    // String with 8-bit size prefix.
  String16,   // String with 16-bit size prefix.
  Timestamp,
  Char,
  Utf_Char16,
  Utf_Char32,
  Int8,
  Int16,
  Int32,
  Int64,
  Uint8,
  Uint16,
  Uint32,
  Uint64,
  Float16,
  Float32,
  Float64,
  Float128,
  Bool,
  type_numof
};

struct type_props {
  type const type_;
  char const* const name_;
  size_t const size_;
  size_t const alignment_;
};

#if (__STDCPP_FLOAT32_T__ != 1) || (__STDCPP_FLOAT64_T__ != 1) || (__STDCPP_FLOAT128_T__ != 1)
  #error "<stdfloat> types not available"
#endif

static constexpr std::array<type_props, type_numof> k_type_props {
  type_props{ Key8,       "*key8*",  sizeof(uint8_t),              alignof(uint8_t) },
  type_props{ Key16,      "*key16*", sizeof(uint16_t),             alignof(uint16_t) },
  type_props{ String8,    "str8",    sizeof(uint8_t),              alignof(uint8_t) },
  type_props{ String16,   "str16",   sizeof(uint16_t),             alignof(uint16_t) },
  type_props{ Timestamp,  "tstamp",  sizeof(raw_time_t),           alignof(raw_time_t) },
  type_props{ Char,       "char",    sizeof(char),                 alignof(char) },
  type_props{ Utf_Char16, "utf-c16", sizeof(char16_t),             alignof(char16_t) },
  type_props{ Utf_Char32, "utf-c32", sizeof(char32_t),             alignof(char32_t) },
  type_props{ Int8,       "i8",      sizeof(int8_t),               alignof(int8_t) },
  type_props{ Int16,      "i16",     sizeof(int16_t),              alignof(int16_t) },
  type_props{ Int32,      "i32",     sizeof(int32_t),              alignof(int32_t) },
  type_props{ Int64,      "i64",     sizeof(int64_t),              alignof(int64_t) },
  type_props{ Uint8,      "u8",      sizeof(uint8_t),              alignof(uint8_t) },
  type_props{ Uint16,     "u16",     sizeof(uint16_t),             alignof(uint16_t) },
  type_props{ Uint32,     "u32",     sizeof(uint32_t),             alignof(uint32_t) },
  type_props{ Uint64,     "u64",     sizeof(uint64_t),             alignof(uint64_t) },
  type_props{ Float16,    "f16",     sizeof(std::float16_t),       alignof(std::float16_t) },
  type_props{ Float32,    "f32",     sizeof(std::float32_t),       alignof(std::float32_t) },
  type_props{ Float64,    "f64",     sizeof(std::float64_t),       alignof(std::float64_t) },
  type_props{ Float128,   "f128",    sizeof(std::float128_t),      alignof(std::float128_t) },
  type_props{ Bool,       "bool",    sizeof(bool),                 alignof(bool) }
};

static inline char const* enum_names_type(type t) {
  return k_type_props[t].name_;
}

inline constexpr bool is_string8_type(type t) {
  return t == String8 || t == Key8;
}

inline constexpr bool is_string16_type(type t) {
  return t == String16 || t == Key16;
}

inline constexpr bool is_string_type(type t) {
  return is_string8_type(t) || is_string16_type(t);
}

static inline constexpr bool check_types() {
  for (size_t i = 0; i < k_type_props.size(); ++i) { 
    if (k_type_props[i].type_ != i)
    {
      return false;
    }
  }
  return true;
}

static_assert(check_types(), "order of field::k_type_props does not match field::types::type enum");

} // namespace types
} // namespace rdf