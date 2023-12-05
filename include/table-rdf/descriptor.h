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
class descriptor
{
public:
  // Construct a descriptor from a vector of fields.
  // The memory layout is computed to mimic C++ struct layout rules:
  //   - mem_align() is the size and alignment of the largest field.
  //   - Each offset will respect the corresponding field's alignment.
  //   - mem_size() includes padding for alignment to mem_align().
  // If pack is true, field offsets are sorted by descending alignment.
  // The index order of the fields argument is always preserved in the descriptor, i.e. desc.fields(i) == fields[i].
  // Use describe() to print the descriptor layout.
  inline descriptor(std::string const& name,
                    std::vector<field> const& fields,
                    bool pack = true);

  inline descriptor(std::string const& name,
                    fields_builder const& builder,
                    bool pack = true);

  virtual ~descriptor() = default;

  std::string_view          name() const { return name_; }

  std::vector<field> const& fields() const { return fields_; }
               field const& fields(field::index_t index) const { BOOST_ASSERT(index < fields().size()); return fields()[index]; }
               field const& fields(char const* name) const { return fields_by_name_.at(name); }
  
  size_t mem_size() const { return mem_size_; }     // The in-memory size including padding for alignment to mem_align().
  size_t mem_align() const { return mem_align_; }

  inline field const& find(type t) const;

  // Logging.
  inline std::string describe(bool sort_by_offset = false) const;
  inline std::string header() const;

private:
  std::string name_;
  std::vector<field> fields_;
  std::map<std::string, field const&> fields_by_name_;
  size_t mem_size_;
  size_t mem_align_;
};

} // namespace rdf

#include "descriptor.tpp"