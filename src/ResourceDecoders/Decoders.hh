#pragma once

#include "../ResourceFile.hh"

#include <vector>


// See Components.r
struct Decoded_thng {
  struct PlatformInfo {
    uint32_t  flags;
    uint32_t  code_type;
    int16_t   code_id;
    // platform68k = 1
    // platformPowerPC = 2
    // platformInterpreted = 3
    // platformWin32 = 4
    // platformPowerPCNativeEntryPoint = 5
    int16_t   platform_type;
  };
  
  uint32_t  type;
  uint32_t  sub_type;
  uint32_t  manufacturer;
  uint32_t  flags;
  uint32_t  code_type;
  int16_t   code_id;
  uint32_t  name_type;
  int16_t   name_id;
  uint32_t  info_type;
  int16_t   info_id;
  uint32_t  icon_type;
  int16_t   icon_id;
  
  // Optional
  uint32_t  version;
  uint32_t  registration_flags;
  int16_t   icon_family_id;
  
  std::vector<PlatformInfo> platform_infos;
  
  // Even more optional
  uint32_t  res_map_type;
  int16_t   res_map_id;
};

Decoded_thng decode_thng(const std::shared_ptr<const ResourceFile::Resource>& res);