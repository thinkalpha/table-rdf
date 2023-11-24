#pragma once
#include "common.h"

namespace rdf {

namespace bal = boost::alignment;

template<typename T>
T* field::offset_ptr(mem_t* const base) const
{
  auto ptr = reinterpret_cast<T*>(base + offset());
  BOOST_ASSERT(bal::is_aligned(align(), ptr));
  BOOST_ASSERT(alignof(T) == align());
  return ptr;
}

template <types::type T, concepts::numeric V>
void field::write(mem_t* const base, V const value) const
{
  // Rigorous catching of common bugs.
  BOOST_ASSERT(payload() == 0);
  BOOST_ASSERT_MSG(sizeof(V) == size(), fmt::format("field '{}' size = {}B does not match value param sizeof({}) = {}B",
                                                    name_, size(), typeid(V).name(), sizeof(V)).c_str());
  BOOST_ASSERT_MSG(alignof(V) == align(), fmt::format("field '{}' alignment = {}B which does not match value parameter alignof({}) = {}B",
                                                      name_, align(), typeid(V).name(), alignof(V)).c_str());
  BOOST_ASSERT_MSG(type_ == T, fmt::format("field '{}' type '{}' does not match write<{}>(...)", 
                   name_, enum_names_type(type_), enum_names_type(T)).c_str());

  *offset_ptr<V>(base) = value;
}

template<concepts::string_size_prefix P>
void field::write_str(mem_t* const base, string_t const value) const
{
  BOOST_ASSERT(payload() > 0);

  using char_type = string_t::value_type;
  auto const size = value.size() * sizeof(char_type);
  if (size > payload()) {
    throw std::runtime_error("string too large for payload");   // TODO: Change to assert for speed?
  }

  if (size != (P)size) {
    throw std::runtime_error("string lengths must fit in 8-bits");
  }

  auto mem = offset_ptr<char_type>(base);
  *reinterpret_cast<P*>(mem) = (P)size;       // Write length prefix.
  auto const char_mem = mem + sizeof(P);      // TODO: Unit test alignment for 8-bit and 16-bit prefixes.
  BOOST_ASSERT(bal::is_aligned(alignof(char_type), char_mem));
  std::memcpy(char_mem, value.data(), size);  // Write string.
}

template<>
void field::write<types::String8>(mem_t* const base, string_t const value) const
{
  write_str<uint8_t>(base, value);
}

template<>
void field::write<types::String16>(mem_t* const base, string_t const value) const
{
  write_str<uint16_t>(base, value);
}

template<>
void field::write<types::Key8>(mem_t* const base, string_t const value) const
{
  write_str<uint8_t>(base, value);
}

template<>
void field::write<types::Key16>(mem_t* const base, string_t const value) const
{
  write_str<uint16_t>(base, value);
}

template <types::type T, concepts::numeric V>
V field::read(mem_t* const base) const
{
  return *offset_ptr<V>(base);
}

// template<>
// template<concepts::string_size_prefix P>
// string_t const field::read<types::Key8>(mem_t* const base) const
// {
//   BOOST_ASSERT(payload() > 0);
//   auto const length = read<P>(base);
//   return std::string_view(get_ptr<char>(o + sizeof(uint8_t)), length);
// }


static constexpr bool check_types() {
  for (auto const [i, f] : std::views::enumerate(types::k_type_props)) { 
    if (f.type_ != i)
    {
      return false;
    }
  }
  return true;
}

static_assert(check_types(), "order of field::k_type_props does not match field::types::type enum");

} // namespace rdf