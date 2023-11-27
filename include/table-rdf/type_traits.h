#pragma once
#include "field.h"

namespace rdf {
namespace types {

template<type T> struct traits_base
{
  static constexpr types::type const etype = T;
  static constexpr size_t const      size  = k_type_props[T].size_;
  static constexpr size_t const      align = k_type_props[T].alignment_;
  static constexpr char const*       name  = k_type_props[T].name_;
};

template <type T> struct traits {};

// Reminder.
static_assert(type_numof == 21, "ensure type traits are updated when new types are added");

template<> struct traits<Key8>       : traits_base<Key8>       { using type = string_t; using prefix_t = uint8_t; };
template<> struct traits<Key16>      : traits_base<Key16>      { using type = string_t; using prefix_t = uint16_t; };
template<> struct traits<String8>    : traits_base<String8>    { using type = string_t; using prefix_t = uint8_t; };
template<> struct traits<String16>   : traits_base<String16>   { using type = string_t; using prefix_t = uint16_t; };
template<> struct traits<Timestamp>  : traits_base<Timestamp>  { using type = timestamp_t; };
template<> struct traits<Char>       : traits_base<Char>       { using type = char; };
template<> struct traits<Utf_Char16> : traits_base<Utf_Char16> { using type = char16_t; };
template<> struct traits<Utf_Char32> : traits_base<Utf_Char32> { using type = char32_t; };
template<> struct traits<Int8>       : traits_base<Int8>       { using type = int8_t; };
template<> struct traits<Int16>      : traits_base<Int16>      { using type = int16_t; };
template<> struct traits<Int32>      : traits_base<Int32>      { using type = int32_t; };
template<> struct traits<Int64>      : traits_base<Int64>      { using type = int64_t; };
template<> struct traits<Uint8>      : traits_base<Uint8>      { using type = uint8_t; };
template<> struct traits<Uint16>     : traits_base<Uint16>     { using type = uint16_t; };
template<> struct traits<Uint32>     : traits_base<Uint32>     { using type = uint32_t; };
template<> struct traits<Uint64>     : traits_base<Uint64>     { using type = uint64_t; };
template<> struct traits<Float16>    : traits_base<Float16>    { using type = std::float16_t; };
template<> struct traits<Float32>    : traits_base<Float32>    { using type = std::float32_t; };
template<> struct traits<Float64>    : traits_base<Float64>    { using type = std::float64_t; };
template<> struct traits<Float128>   : traits_base<Float128>   { using type = std::float128_t; };
template<> struct traits<Bool>       : traits_base<Bool>       { using type = bool; };

} // namespace types
} // namespace rdf
