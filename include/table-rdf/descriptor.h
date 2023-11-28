#pragma once

#include "common.h"
#include "util.h"
#include "field.h"
#include "fields_builder.h"

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
                    std::vector<field> const& fields);

  inline descriptor(std::string const& name,
                    fields_builder const& builder);

  std::string_view          name() const { return name_; }

  std::vector<field> const& fields() const { return fields_; }
               field const& fields(field::index_t index) const { BOOST_ASSERT(index < fields().size()); return fields()[index]; }
               field const& fields(char const* name) const { return fields_by_name_.at(name); }
  
     field::offset_t        offset(char const* name) const { return fields(name).offset_; }
     field::offset_t        offset(field::index_t index) const { return fields(index).offset_; }

  // The in-memory size including padding for alignment to k_record_alignment.
  size_t mem_size() const { return mem_size_; }

  // Logging.
  inline std::string describe() const;
  inline std::string header() const;

private:
  std::string const name_;
  std::vector<field> const fields_;
  std::map<field::name_t, field const&> fields_by_name_;
  size_t mem_size_;
};

} // namespace rdf

#include "descriptor.tpp"