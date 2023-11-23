#pragma once

#include "common.h"
#include "util.h"
#include "field.h"

#include <map>
#include <unordered_map>
#include <fmt/core.h>
#include <array>

namespace rdf
{
class descriptor final
{
public:
  inline descriptor(std::string const& name,
             std::string const& time_parse,
             std::vector<field> const& fields);

  inline descriptor(std::string const& name,
             std::string const& time_parse,
             fields_builder const& builder);

  std::string_view          name() const { return name_; }

  std::vector<field> const& fields() const { return fields_; }
               field const& fields(field::index_t field_index) const { BOOST_ASSERT(field_index < fields().size()); return fields()[field_index]; }
               field const& fields(char const* field_name) const { return fields_by_name_.at(field_name); }
  
     field::offset_t        offset(char const* field_name) const { return fields(field_name).offset_; }

     field::offset_t        key_o() const { return key_offset_; }
     field::offset_t        timestamp_o() const { return timestamp_offset_; }

      field::index_t        key_i() const { return key_index_; }
      field::index_t        timestamp_i() const { return timestamp_index_; }

  inline timestamp_t        str_to_time(std::string_view sv) const;
  inline std::string        time_to_str(timestamp_t ts) const;

         std::string        time_parse() const { return time_parse_; }
         std::string        time_format() const { return time_format_; }

  // The in-memory size including padding for alignment to k_record_alignment.
  size_t mem_size() const { return mem_size_; }

  // Debugging.
  inline std::string describe() const;
  inline std::string header() const;

private:
  std::string const name_;
  std::vector<field> const fields_;
  std::map<field::name_t, field const&> fields_by_name_;

  field::offset_t key_offset_;
  field::offset_t timestamp_offset_;

  field::index_t key_index_;
  field::index_t timestamp_index_;

  std::string const time_parse_;    // Because the std::chrono parse function uses a slightly different syntax to the std::format function.
  std::string const time_format_;

  size_t mem_size_;
};

} // namespace rdf

#include "descriptor.tpp"