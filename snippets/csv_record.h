#pragma once
#include <variant>
#include <vector>
#include <ranges>
#include "record.h"

namespace rdf
{

class csv_record : public record {

public:
  using cell_t = std::variant<tds::timestamp_t,
                                  std::string,
                                  rdf::key_t,
                                  int32_t,
                                  int64_t,
                                  float,
                                  double,
                                  bool>;
  using values = std::vector<cell_t>;

  csv_record(values const& values)
    : record({}),
      values_{values}
  {

  }

  rdf::key_t key(rdf::descriptor const& d) const override { return std::get<rdf::key_t>(values_[d.key_o()]); }
  timestamp_t timestamp(rdf::descriptor const& d) const override { return std::get<timestamp_t>(values_[d.timestamp_o()]); }

  std::string to_string(cell_t const& v) const
  {
      std::stringstream ss;
      // See: https://en.cppreference.com/w/cpp/utility/variant/visit
      std::visit([&ss](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int32_t> || 
                      std::is_same_v<T, int64_t> ||
                      std::is_same_v<T, float> ||
                      std::is_same_v<T, double>) 
          ss << arg;
        else if constexpr (std::is_same_v<T, std::string>)
          ss << std::quoted(arg);
        else if constexpr (std::is_same_v<T, rdf::key_t>)
          ss << std::quoted(arg);
        else if constexpr (std::is_same_v<T, bool>)
          ss << arg ? "true" : "false";
        else if constexpr (std::is_same_v<T, tds::timestamp_t>) {
          ss << std::format("{:%Y-%m-%d %H:%M}", arg);
        }
        else 
            static_assert(always_false_v<T>, "visitor does not handle all variant types");
      }
      , v);
      return ss.str();
  }

  std::string to_string(rdf::descriptor const& desc) const override
  {
    // See: https://en.cppreference.com/w/cpp/utility/variant/visit
    std::stringstream ss;
    // values_ | std::ranges::views::transform()
    assert(desc.fields().size() == values_.size());
    for (size_t i = 0; i < values_.size(); ++i)
    {
      ss << fmt::format(desc.fields().at(i).fmt_, to_string(values_.at(i)));
    }
    return ss.str();
  }

private:
  values const& values_;
};

} // namespace rdf