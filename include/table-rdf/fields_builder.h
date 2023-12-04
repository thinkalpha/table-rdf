#pragma once
#include "field.h"

namespace rdf {

// Simple helper to make it easier to build a vector of fields.
struct fields_builder
{
  fields_builder& push(field const& f)
  {
    fields_.push_back(f);
    return *this;
  }

  auto& get() const { return fields_; }

private:
  std::vector<field> fields_;
};

} // namespace rdf