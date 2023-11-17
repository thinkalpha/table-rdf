#pragma once

#include "descriptor.h"
#include "record.h"

namespace rdf
{

  using transform_t = std::remove_reference<std::remove_reference<rdf::record (*&)(const std::byte &)>::type>::type; // TODO: See if function decltype trick can make this more readable.
  using records_view_t = std::ranges::transform_view<std::ranges::stride_view<rdf::mspan>, transform_t>;

  inline auto records_view(mspan const& memory, size_t record_size) {
    return memory | 
           std::views::stride(record_size) |
           std::views::transform(rdf::mem_to_record);
  }

} // namespace rdf