#pragma once

enum class types::type
{
  key_type,
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

namespace rdf {

template <class T> struct type_trait
{
  using concrete_type = T;
  static types::type const type;
	static char const* name;
};

template <> char const* type_trait<double>::name = "double";
template <> types::type const type_trait<double>::type = types::type::float64_type;

static_assert(std::is_same_v<type_trait<double>::concrete_type, double>);

struct type_handler {
    template <class Type>
    static type_handler create() {
        type_handler ci;
        return ci;
    }
};

typedef std::map<char const*, type_handler> type_handler_map;

template <typename... Args>
struct types_builder;

template <class Type, typename... Args>
struct types_builder<Type, Args...>
{
	static void build_map(type_handler_map& map) {
		map[type_trait<Type>::name] = type_handler::create<Type>();
		types_builder<Args...>::build_map(map);
	}

	static type_handler_map get_type_handler_map() {
		type_handler_map map;
		build_map(map);
		return map;
	}
};

template <>
struct types_builder<> {
	static void build_map(type_handler_map& /*info*/) {}		// For terminal case of zero template arguments.
};

typedef types_builder<double //,
                      // string_t
                      > all_types;

static type_handler_map k_type_handlers = all_types::get_type_handler_map();



// template<> char const* type_traits<asdsadas>::name = "char";

// template <field::type T>
// struct field_t {};

// template <field::type T>
// struct mutable_field {
//   static auto read(mem_t* const dest, field const& field);
//   static void write(mem_t* const dest, field const& field, auto value);
// };

// template <field::type T>
// inline
// auto mutable_field<T>::read(mem_t* const source, field const& field)
// {
//   return 0;
// }

// template <field::type T>
// inline
// void mutable_field<T>::write(mem_t* const dest, field const& field, auto value)
// {
//   return;
// }

// template <field::type T>
// struct mutable_string_field : public mutable_field<T> {
//   static std::string_view read(mem_t* const dest, field const& field);
//   static void write(mem_t* const dest, field const& field, std::string_view value);
// };

// template <>
// inline
// std::string_view mutable_string_field<String8>::read(mem_t* const source, field const& field)
// {
//   return std::string_view{"hello world"};
// }

// template <>
// inline
// void mutable_string_field<String8>::write(mem_t* const dest, field const& field, std::string_view value)
// {
//   return;
// }

// template<> struct field_t<Key8> : public mutable_string_field<Key8> {};

// struct reader_writer {
//     template <field::type T>
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

// template <field::type T, typename... Args>
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

// template<field::type T> struct type {
//   static constexpr field::type_props const desc = field::k_type_props[T];
// };

// template <field::type T, typename SizePrefix> struct string_traits : type<T>
// {
//   using size_prefix_t = SizePrefix;
// };

// template<> char const* string_traits<field::key_type, >::name = "char";

// traits<field::key_type>::write()

} // namespace rdf