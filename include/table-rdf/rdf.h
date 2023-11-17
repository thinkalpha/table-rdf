#pragma once

#include <functional>
#include "descriptor.h"
#include "record.h"

namespace rdf
{

  namespace views {

    inline auto records(mspan const& memory, descriptor const& d) {
      return memory | 
             std::views::stride(d.mem_size()) |
             std::views::transform(rdf::mem_to_record);
    }

    using records_view_t = decltype(std::function(records))::result_type;

  }

} // namespace rdf