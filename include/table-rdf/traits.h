#pragma once
#include "types.h"

namespace rdf {

class record;

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
static_assert(type_numof == 22, "ensure type traits are updated when new types are added");

template<> struct traits<Key8>       : traits_base<Key8>       { using type = string_t; using prefix_t = uint8_t; };
template<> struct traits<Key16>      : traits_base<Key16>      { using type = string_t; using prefix_t = uint16_t; };
template<> struct traits<String8>    : traits_base<String8>    { using type = string_t; using prefix_t = uint8_t; };
template<> struct traits<String16>   : traits_base<String16>   { using type = string_t; using prefix_t = uint16_t; };
template<> struct traits<Timestamp>  : traits_base<Timestamp>  { using type = timestamp_t; };
template<> struct traits<Char>       : traits_base<Char>       { using type = char; };
template<> struct traits<Utf_Char8>  : traits_base<Utf_Char8>  { using type = char8_t; };
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

// Alias template for the value type of a rdf::types::type.
template <type T> using value_t = traits<T>::type;

namespace concepts {

  template<class R> concept record = std::derived_from<R, rdf::record>;

  template<class V> concept string = std::is_same_v<V, string_t>;

  template<class V> concept timestamp = std::is_same_v<V, timestamp_t>;

  template<class V> concept integral = std::is_same_v<V, char> ||
                                       std::is_same_v<V, char8_t> ||
                                       std::is_same_v<V, char16_t> ||
                                       std::is_same_v<V, char32_t> ||
                                       std::is_same_v<V, int8_t> ||
                                       std::is_same_v<V, int16_t> ||
                                       std::is_same_v<V, int32_t> ||
                                       std::is_same_v<V, int64_t> ||
                                       std::is_same_v<V, uint8_t> ||
                                       std::is_same_v<V, uint16_t> ||
                                       std::is_same_v<V, uint32_t> ||
                                       std::is_same_v<V, uint64_t>;

  template<class V> concept boolean = std::is_same_v<V, bool>;

  template<class V> concept floating_point = std::is_same_v<V, std::float16_t> ||
                                             std::is_same_v<V, std::float32_t> ||
                                             std::is_same_v<V, std::float64_t> ||
                                             std::is_same_v<V, std::float128_t>;

  template<class V> concept numeric = boolean<V> || integral<V> || floating_point<V>;

  template<class V> concept field_value = numeric<V> || string<V> || timestamp<V>;
}

inline constexpr bool string8_type(type t) {
  return t == String8 || t == Key8;
}

inline constexpr bool string16_type(type t) {
  return t == String16 || t == Key16;
}

inline constexpr bool string_type(type t) {
  return string8_type(t) || string16_type(t);
}

} // namespace types
} // namespace rdf
