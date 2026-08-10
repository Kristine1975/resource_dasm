#pragma once

#include <stdint.h>

namespace ResourceDASM {
  // A pair of (resource type, resource ID), because std::pair isn't hashable
  struct ResourceTypeID {
    uint32_t  res_type;
    int16_t   res_id;
    
    bool operator==(const ResourceTypeID& rhs) const = default;
  };
}


template <>
struct std::hash<ResourceDASM::ResourceTypeID> {
  std::size_t operator()(const ResourceDASM::ResourceTypeID& r) const noexcept {
    auto t = std::hash<uint32_t>()(r.res_type);
    auto i = std::hash<int16_t>()(r.res_id);
    
    // From cppreference's example
    return t ^ (i << 1);
  }
};
