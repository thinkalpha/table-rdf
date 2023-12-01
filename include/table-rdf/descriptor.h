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
  // If pack is true, field offsets are sorted by descending alignment, 
  // but the order of the fields in the descriptor is preserved.
  inline descriptor(std::string const& name,
                    std::vector<field> const& fields,
                    bool pack = true);

  inline descriptor(std::string const& name,
                    fields_builder const& builder,
                    bool pack = true);

  std::string_view          name() const { return name_; }

  std::vector<field> const& fields() const { return fields_; }
               field const& fields(field::index_t index) const { BOOST_ASSERT(index < fields().size()); return fields()[index]; }
               field const& fields(char const* name) const { return fields_by_name_.at(name); }
  
     field::offset_t        offset(char const* name) const { return fields(name).offset(); }
     field::offset_t        offset(field::index_t index) const { return fields(index).offset(); }

  size_t mem_size() const { return mem_size_; }     // The in-memory size including padding for alignment to mem_align().
  size_t mem_align() const { return mem_align_; }

  // Logging.
  inline std::string describe(bool sort_by_alignment = false) const;
  inline std::string header() const;

private:
  std::string name_;
  std::vector<field> fields_;
  std::map<field::name_t, field const&> fields_by_name_;
  size_t mem_size_;
  size_t mem_align_;
};

} // namespace rdf

#include "descriptor.tpp"