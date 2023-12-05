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

template<class T> struct type_trait
{
  using concrete_type = T;
  static types::type const type;
	static char const* name;
};

template<> char const* type_trait<double>::name = "double";
template<> types::type const type_trait<double>::type = types::type::float64_type;

static_assert(std::is_same_v<type_trait<double>::concrete_type, double>);

struct type_handler {
    template<class Type>
    static type_handler create() {
        type_handler ci;
        return ci;
    }
};

typedef std::map<char const*, type_handler> type_handler_map;

template <class... Args>
struct types_builder;

template <class Type, class... Args>
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


} // namespace rdf