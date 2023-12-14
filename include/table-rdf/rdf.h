#pragma once

#include <functional>
#include "descriptor.h"
#include "record.h"

namespace rdf
{

  namespace views {

    template<concepts::record R>
    auto records(mspan const& memory, descriptor const& d) {
      return memory | 
             std::views::stride(d.mem_size()) |
             std::views::transform(mem_to_record<R>);
    }
    template<concepts::record R>
    using records_view_t = decltype(std::function(records<R>))::result_type;

  }

  static_assert(std::ranges::random_access_range<rdf::views::records_view_t<rdf::record>>);

} // namespace rdf