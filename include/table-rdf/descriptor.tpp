#pragma once
#include <ranges>
#include <numeric>

namespace rdf
{

  descriptor::descriptor(std::string const& name,
                         std::vector<field> const& fields,
                         bool pack)
      : name_{name},
        fields_{fields},
        mem_size_{0},
        mem_align_{0}
  {
    namespace bal = boost::alignment;

    if (fields_.empty()) {
      throw std::runtime_error("descriptor must have at least one field");
    }

    // Create a vector of field indices [0, 1, 2, ...].
    std::vector<field::index_t> indices(fields_.size());
    std::iota(std::begin(indices), std::end(indices), 0);

    if (pack)
    {
      // Sort indices by descending alignment.
      std::ranges::sort(indices, [&](auto a, auto b) {
        return fields_[a].align() > fields_[b].align();
      });
    }

    // Compute offsets so that the in-memory layout is in order of indices vector.
    // The index order of the fields argument is always preserved in the descriptor, i.e. desc.fields(i) == fields[i].
    auto const i0 = indices[0];
    fields_[i0].offset_ = 0;
    fields_[i0].index_ = i0;
    auto max_align = fields_[i0].align();
    for (auto [a, b] : indices | std::views::adjacent<2>)
    {
      auto offset_ptr = (void*)(fields_[a].offset() + fields_[a].size());
      fields_[b].offset_ = (field::offset_t)bal::align_up(offset_ptr, fields_[b].align());
      fields_[b].index_ = b;
      max_align = std::max(max_align, fields_[b].align());
    }

    // Set the alignment of the descriptor to the maximum alignment of its fields.
    mem_align_ = max_align;

    // Compute total size including alignment padding.
    auto last = fields_[indices.back()];
    mem_size_ = bal::align_up(last.offset() + last.size(), mem_align_);
    BOOST_ASSERT(mem_size_ % mem_align_ == 0);

    // Build a map of field names to fields.
    for (auto const& f : fields_) {
      BOOST_VERIFY_MSG(fields_by_name_.insert({f.name(), f}).second, fmt::format("duplicate field name '{}'", f.name()).c_str());
    }
  }

  descriptor::descriptor(std::string const& name,
                         fields_builder const& builder,
                         bool pack)
    : descriptor(name, builder.get(), pack)
  {
  }

  std::string descriptor::header() const
  {
    auto out = fmt::memory_buffer();
    auto out_it = std::back_inserter(out);
    fmt::format_to(out_it, "{:8}", "(row)");
    for (auto& f : fields_) {
      fmt::format_to(out_it, f.fmt(), f.name());
    }
    return fmt::to_string(out);
  }

  std::string descriptor::describe(bool sort_by_offset) const
  {
    std::vector<field::index_t> indices(fields_.size());
    std::iota(std::begin(indices), std::end(indices), 0);
    if (sort_by_offset) {
      std::ranges::sort(indices, [&](auto a, auto b) {
        return fields_[a].offset() < fields_[b].offset();
      });
    }

    auto const str = [&indices, this]() {
      std::stringstream ss;
      ss << "\n--- " << name() << " ---\n";
      for (auto i : indices) {
        ss << fields(i).describe(i) << "\n";
      }
      ss << "--- size: " << mem_size() << ", alignment: " << mem_align() << " ---\n";
      return ss.str();
    }();
    return str;
  }
} // namespace rdf