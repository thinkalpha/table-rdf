#pragma once
#include "common.h"
#include "types.h"

namespace rdf {

namespace bal = boost::alignment;

#ifndef BOOST_ASSERT_IS_NULL
  #define DBG_VALIDATE_FIELD(EnumType, ValType) validate<EnumType,ValType>()
#else
  #define DBG_VALIDATE_FIELD(T, V) (void)0
#endif

template<typename T>
T* field::offset_ptr(mem_t* const base) const
{
  auto ptr = reinterpret_cast<T*>(base + offset());
  BOOST_ASSERT(bal::is_aligned(align(), ptr));
  BOOST_ASSERT(alignof(T) == align());
  return ptr;
}

// TODO: Separate write_str functions at the top level, with optional param to pass in the length to avoid length calculation on the fly.
template <types::type T, concepts::field_value V>
void field::write(mem_t* const base, V const value) const
{
  DBG_VALIDATE_FIELD(T, V);
  if constexpr(concepts::numeric<V>)
  {
    *offset_ptr<V>(base) = value;
  }
  else if constexpr(concepts::string<V>)
  {
    if constexpr (is_string8_type(T)) {
      write_str<uint8_t>(base, value);
    }
    else if constexpr (is_string16_type(T)) {
      write_str<uint16_t>(base, value);
    }
    static_assert(is_string_type(T));
  }
  else {
    static_assert(util::always_false_v<V>, "unsupported field value type");
  }
}

template <types::type T, concepts::field_value V>
V const field::read(mem_t* const base) const
{
  DBG_VALIDATE_FIELD(T, V);
  if constexpr (concepts::numeric<V>) {
    return *offset_ptr<V>(base);
  }
  else if constexpr (concepts::string<V>) {
    if constexpr (is_string8_type(T)) {
      return read_str<uint8_t>(base);
    }
    else if constexpr (is_string16_type(T)) {
      return read_str<uint16_t>(base);
    }
    static_assert(is_string_type(T));
  }
  else {
    static_assert(util::always_false_v<V>, "unsupported field value type");
  }
}


template<concepts::string_size_prefix P>
void field::write_str(mem_t* const base, string_t const value) const
{
  using char_type = string_t::value_type;

  // TODO: Size-prefix is number of *bytes*. But this means we need to convert to number of *chars*
  // in read_str. Should we enforce sizeof(char_type) == 1 to save the multiply and divide?
  auto const size = value.length() * sizeof(char_type);

  if (size > payload()) {
    throw std::runtime_error("string too large for payload");   // TODO: Change to assert for speed?
  }
  else if (size != (P)size) {
    throw std::runtime_error(fmt::format("strings of type {} must fit in {}-bits", enum_names_type(type_), sizeof(P) * 8));
  }

  auto length_mem = offset_ptr<mem_t>(base);
  *reinterpret_cast<P*>(length_mem) = (P)size;   // Write length prefix.
  
  auto const char_mem = offset_ptr<char_type>(base + sizeof(P));
  std::memcpy(char_mem, value.data(), size);     // Write string.
}

template<concepts::string_size_prefix P>
string_t const field::read_str(mem_t* const base) const
{
  using char_type = string_t::value_type;
  auto const length = *offset_ptr<P>(base) / sizeof(char_type);
  auto const chars = offset_ptr<string_t::value_type>(base + sizeof(P));
  return { chars, length };
}

template <types::type T, typename V>
void field::validate() const
{
  if constexpr (is_string_type(T)) {
    BOOST_ASSERT_MSG(payload() > 0, fmt::format("non-string field '{}' should not have a payload", name_).c_str());
  }
  else {
    BOOST_ASSERT_MSG(payload() == 0, fmt::format("string field '{}' should have a payload", name_).c_str());
    BOOST_ASSERT_MSG(sizeof(V) == size(), fmt::format("field '{}' size = {}B does not match value param sizeof({}) = {}B",
                                                      name_, size(), typeid(V).name(), sizeof(V)).c_str());
    BOOST_ASSERT_MSG(alignof(V) == align(), fmt::format("field '{}' alignment = {}B which does not match value parameter alignof({}) = {}B",
                                                        name_, align(), typeid(V).name(), alignof(V)).c_str());

  }

  BOOST_ASSERT_MSG(type_ == T, fmt::format("field '{}' type '{}' does not match template param <{}>(...)", 
                                            name_, enum_names_type(type_), enum_names_type(T)).c_str());
}

} // namespace rdf