#pragma once
#include "field.h"

namespace rdf {

// Helper that calculates offsets as fields are added, taking the field alignment property into account.
struct fields_builder
{
  fields_builder& push(field const& f)
  {
    BOOST_ASSERT_MSG(f.offset_ == field::k_null_offset, "field has explicit offset, don't use fields_builder");
    BOOST_ASSERT_MSG(f.index_ == field::k_null_index, "field has explicit index, don't use fields_builder");
    fields_.push_back(f);

    auto& fl = fields_.back();                // Last field.
    fl.offset_ = 0;
    fl.index_ = fields_.size() - 1;
    if (fields_.size() > 1)
    {
      auto& fp = fields_[fields_.size() - 2];   // 2nd last field.

      // TODO: This assumes the base address is aligned to at least the maximum alignment of all fields.
      // Currently this is the case as memory.h aligns to max_align_t. But we should assert this in the record accessors.
      auto offset_ptr = (void*)(fp.offset_ + fp.size());
      fl.offset_ = (field::offset_t)boost::alignment::align_up(offset_ptr, fl.align());
    }
    return *this;
  }

  auto& get() const { return fields_; }

private:
  std::vector<field> fields_;
};

} // namespace rdf