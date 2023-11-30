#pragma once

namespace rdf
{

  descriptor::descriptor(std::string const& name,
                         std::vector<field> const& fields)
      : name_{name},
        fields_{fields},
        mem_size_{0}
  {
    // TODO: The descriptor could optimise memory packing by discovering an optimal ordering
    // of fields to minimise padding.
    for (auto const& f : fields_)
    {
      fields_by_name_.insert({f.name(), f});
    }

    auto last = fields_.back();
    mem_size_ = boost::alignment::align_up(last.offset() + last.size(), field::k_record_alignment);
  }

  descriptor::descriptor(std::string const& name,
                         fields_builder const& builder)
    : descriptor(name, builder.get())
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

  std::string descriptor::describe() const
  {
    auto const str = [this]() {
      std::stringstream ss;
      ss << "\n--- " << name() << " ---\n";
      for (auto& f : fields_) {
        auto field_fmt = fmt::format("{:25} (type: {:8} size: {:3} align: {:3}, offset: {:4}) - '{}'",
                                     f.name(), f.type_name(), f.size(), f.align(), f.offset(), f.description());
        ss << field_fmt << "\n";
      }
      ss << "--- size: " << mem_size() << ", alignment: " << field::k_record_alignment << " ---\n";
      return ss.str();
    }();
    return str;
  }

} // namespace rdf