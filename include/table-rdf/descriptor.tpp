#pragma once

namespace rdf
{

  descriptor::descriptor(std::string const& name,
                         std::string const& time_parse,
                         std::vector<field> const& fields)
      : name_{name},
        fields_{fields},
        key_offset_{field::k_null_offset},
        timestamp_offset_{field::k_null_offset},
        time_parse_{time_parse},
        time_format_{fmt::format("{{:{}}}", time_parse)},
        mem_size_{0}
  {
    // TODO: The descriptor could optimise memory packing by discovering an optimal ordering
    // of fields to minimise padding.
    for (auto const& f : fields_)
    {
      fields_by_name_.insert({f.name_, f});

      // Store the offsets and indices of required fields.
      if (f.type_ == types::Key8) {
        BOOST_ASSERT(key_offset_ == field::k_null_offset);
        key_offset_ = f.offset_;
        key_index_ = f.index_;
      }
      else if (f.type_ == types::Timestamp) {
        BOOST_ASSERT(timestamp_offset_ == field::k_null_offset);
        timestamp_offset_ = f.offset_;
        timestamp_index_ = f.index_;
      }
    }

    // TODO: Verify.
    auto last = fields_.back();
    mem_size_ = boost::alignment::align_up(last.offset_ + last.size(), field::k_record_alignment);

    // These must be present in the descriptor.
    BOOST_ASSERT(key_offset_ != field::k_null_offset);
    BOOST_ASSERT(timestamp_offset_ != field::k_null_offset);
    BOOST_ASSERT(key_index_ != field::k_null_index);
    BOOST_ASSERT(timestamp_index_ != field::k_null_index);
  }

  descriptor::descriptor(std::string const& name,
                         std::string const& time_parse,
                         fields_builder const& builder)
    : descriptor(name, time_parse, builder.get())
  {
  }

  timestamp_t descriptor::str_to_time(std::string_view sv) const
  {
    return util::str_to_time(sv, time_parse_);
  }

  std::string descriptor::time_to_str(timestamp_t ts) const
  {
    return util::time_to_str(ts, time_format_);
  }

  std::string descriptor::header() const
  {
    auto out = fmt::memory_buffer();
    auto out_it = std::back_inserter(out);
    fmt::format_to(out_it, "{:8}", "(row)");
    for (auto& f : fields_) {
      fmt::format_to(out_it, f.fmt_, f.name_);
    }
    return fmt::to_string(out);
  }

  std::string descriptor::describe() const
  {
    auto const str = [this]() {
      std::stringstream ss;
      ss << "\n--- " << name() << " ---\n";
      for (auto& f : fields_) {
        auto field_fmt = fmt::format("{:25} (type: {:8} size: {:3} align: {}, offset: {:4}) - '{}'", f.name_, types::enum_names_type(f.type_), f.size(), f.align(), f.offset_, f.description_);
        ss << field_fmt << "\n";
      }
      ss << "--- size: " << mem_size() << ", alignment: " << field::k_record_alignment << ", time parse string: '" << time_parse_ << "' ---\n";
      return ss.str();
    }();
    return str;
  }

} // namespace rdf