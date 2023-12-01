#pragma once
#include "common.h"
#include "types.h"

namespace rdf {

namespace bal = boost::alignment;

#ifndef BOOST_ASSERT_IS_NULL
  #define DBG_VALIDATE_FIELD(EnumType) validate<EnumType>()
#else
  #define DBG_VALIDATE_FIELD(EnumType) (void)0
#endif

// TODO: string_t is currently std::string_view which is std::basic_string_view<char>.
// Ensure this can be generalised to any char type.



template<class V>
V const* field::offset_ptr(mem_t const* base) const
{
  auto ptr = reinterpret_cast<V const*>(base + offset());
  BOOST_ASSERT(bal::is_aligned(ptr, align()));
  BOOST_ASSERT(bal::is_aligned(ptr, alignof(V)));
  return ptr;
}

template<class V>
V* field::offset_ptr(mem_t* base) const
{
  auto ptr = reinterpret_cast<V*>(base + offset());
  BOOST_ASSERT(bal::is_aligned(ptr, align()));
  BOOST_ASSERT(bal::is_aligned(ptr, alignof(V)));
  return ptr;
}

// TODO: Separate write_str functions at the top level, with optional param to pass in the length to avoid length calculation on the fly.
template <type T>
void field::write(mem_t* base, value_t<T> const value) const
{
  DBG_VALIDATE_FIELD(T);
  using V = value_t<T>;

  if constexpr(concepts::string<V>) {
    write_str<T>(base, value);
  }
  else if constexpr(concepts::numeric<V>) {
    *offset_ptr<V>(base) = value;
  }
  else if constexpr(concepts::timestamp<V>) {
    *offset_ptr<raw_time_t>(base) = value.time_since_epoch().count();
  }
  else {
    static_assert(util::always_false_v<V>, "unsupported field value type");
  }
}

template <type T>
value_t<T> const field::read(mem_t const* base) const
{
  DBG_VALIDATE_FIELD(T);
  using V = value_t<T>;

  if constexpr (concepts::string<V>) {
    return read_str<T>(base);
  }
  else if constexpr (concepts::numeric<V>) {
    return *offset_ptr<V>(base);
  }
  else if constexpr(concepts::timestamp<V>) {
    return timestamp_t{ timestamp_t::duration{ *offset_ptr<raw_time_t const>(base) } };
  }
  else {
    static_assert(util::always_false_v<V>, "unsupported field value type");
  }
}


template<type T>
void field::write_str(mem_t* base, value_t<T> value) const
{
  using char_type = value_t<T>::value_type;
  using prefix_t = traits<T>::prefix_t;

  // See comment in types.h.
  static_assert(alignof(prefix_t) >= alignof(char_type));
  constexpr auto k_payload_offset = std::max(sizeof(prefix_t), sizeof(char_type));

  auto const length = value.length() * sizeof(char_type);

  if (length > payload()) {
    throw std::runtime_error("string too large for payload");   // TODO: Change to assert for speed?
  }
  else if (length != (prefix_t)length) {
    throw std::runtime_error(fmt::format("string of type '{}' has length {} that does not fit in {}-bits", enum_names_type(type_), length, sizeof(prefix_t) * 8));
  }

  auto length_mem = offset_ptr<prefix_t>(base);
  *reinterpret_cast<prefix_t*>(length_mem) = (prefix_t)length;   // Write length prefix.
  
  auto const char_mem = offset_ptr<char_type>(base + k_payload_offset);
  std::memcpy(char_mem, value.data(), length);                   // Write string.
}

template<type T>
value_t<T> const field::read_str(mem_t const* base) const
{
  using char_type = value_t<T>::value_type;
  using prefix_t = traits<T>::prefix_t;

  // See comment in types.h.
  static_assert(alignof(prefix_t) >= alignof(char_type));
  constexpr auto k_payload_offset = std::max(sizeof(prefix_t), sizeof(char_type));

  auto const length = *offset_ptr<prefix_t const>(base) / sizeof(char_type);
  auto const chars = offset_ptr<char_type const>(base + k_payload_offset);
  return { chars, length };
}

template <types::type T>
void field::validate() const
{
  using V = value_t<T>;

  BOOST_ASSERT_MSG(type_ == T, fmt::format("field '{}' type '{}' does not match template param '{}'", 
                                            name_, enum_names_type(type_), enum_names_type(T)).c_str());

  if constexpr (string_type(T)) {
    BOOST_ASSERT_MSG(payload() > 0, fmt::format("non-string field '{}' should not have a payload", name_).c_str());
  }
  else {
    BOOST_ASSERT_MSG(payload() == 0, fmt::format("string field '{}' should have a payload", name_).c_str());
    BOOST_ASSERT_MSG(sizeof(V) == size(), fmt::format("field '{}' size = {}B does not match value param sizeof({}) = {}B",
                                                      name_, size(), typeid(V).name(), sizeof(V)).c_str());
    BOOST_ASSERT_MSG(alignof(V) == align(), fmt::format("field '{}' alignment = {}B which does not match value parameter alignof({}) = {}B",
                                                        name_, align(), typeid(V).name(), alignof(V)).c_str());

  }
}

std::string field::describe() const
{
  return fmt::format("{:25} (type: {:8} size: {:3} align: {:3}, offset: {:4}) - '{}'",
                     name(), type_name(), size(), align(), offset(), description());
}

} // namespace rdf