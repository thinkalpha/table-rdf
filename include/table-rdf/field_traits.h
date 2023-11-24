#pragma once
#include "field.h"

namespace rdf {

template<typename T> struct type_traits {
  static char const* name;
  static size_t const size;
  static size_t const align;
};

template<> char const* type_traits<asdsadas>::name = "char";

asdasd



// template <field::types::type T>
// struct field_t {};

// template <field::types::type T>
// struct mutable_field {
//   static auto read(mem_t* const dest, field const& field);
//   static void write(mem_t* const dest, field const& field, auto value);
// };

// template <field::types::type T>
// inline
// auto mutable_field<T>::read(mem_t* const source, field const& field)
// {
//   return 0;
// }

// template <field::types::type T>
// inline
// void mutable_field<T>::write(mem_t* const dest, field const& field, auto value)
// {
//   return;
// }

// template <field::types::type T>
// struct mutable_string_field : public mutable_field<T> {
//   static std::string_view read(mem_t* const dest, field const& field);
//   static void write(mem_t* const dest, field const& field, std::string_view value);
// };

// template <>
// inline
// std::string_view mutable_string_field<types::String8>::read(mem_t* const source, field const& field)
// {
//   return std::string_view{"hello world"};
// }

// template <>
// inline
// void mutable_string_field<types::String8>::write(mem_t* const dest, field const& field, std::string_view value)
// {
//   return;
// }

// template<> struct field_t<types::Key8> : public mutable_string_field<types::Key8> {};

// struct reader_writer {
//     template <field::types::type T>
//     static reader_writer create() {
//         reader_writer rw;
//         ci.toAmino        = &setGraphInput<Type>;
//         // ci.toBifCmd       = &getGraphOutput<Type>;
//         return rw;
//     }

//     using read_func = decltype(&mutable_field<T>::read);
//     using ToBifCmdFn = 

//     ToAminoFn     toAmino;
//     ToBifCmdFn    toBifCmd;
// };

// using converters = std::array<reader_writer, field::type_numof>;

// template <typename... Args>
// struct ConverterBuilder;

// template <field::types::type T, typename... Args>
// struct ConverterBuilder<T, Args...> 
// {
// 	static void buildMap(type_traits& array) {
// 		array[T] = field_t<T>{};
// 		ConverterBuilder<Args...>::buildMap(map);
// 	}

// 	// static ConverterMap getConverterMap() {
// 	// 	ConverterMap map;
// 	// 	buildMap(map);
// 	// 	return map;
// 	// }
// };

// template <>
// struct ConverterBuilder<> {
// 	static void buildMap(ConverterMap& /*info*/) {}		// For terminal case of zero template arguments.
// };

// typedef ConverterBuilder<bool,
// 					 	float,
// 					//    double,
// 					//    int,
// 					//    unsigned int,
// 					//    long long int,
// 					//    long long unsigned int,
// 					//    Math::float2,
// 					//    Math::float3,
// 					//    Math::float4,
// 					//    Math::double2,
// 					//    Math::double3,
// 					//    Math::double4,
// 					   Amino::Object,
// 					   Amino::String
// 					   > AllConverters;

// ConverterMap gConverters = AllConverters::getConverterMap();

// template<field::types::type T> struct type {
//   static constexpr field::type_props const desc = field::k_type_props[T];
// };

// template <field::types::type T, typename SizePrefix> struct string_traits : type<T>
// {
//   using size_prefix_t = SizePrefix;
// };

// template<> char const* string_traits<field::key_type, >::name = "char";

// traits<field::key_type>::write()

} // namespace rdf