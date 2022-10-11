#include "Decoders.hh"

Decoded_thng decode_thng(const std::shared_ptr<const ResourceFile::Resource>& res)
{
  Decoded_thng  result = {};
  StringReader  r(res->data);
  
  result.type = r.get_u32b();
  result.sub_type = r.get_u32b();
  result.manufacturer = r.get_u32b();
  result.flags = r.get_u32b();
  
  result.code_type = r.get_u32b();
  result.code_id = r.get_s16b();
  result.name_type = r.get_u32b();
  result.name_id = r.get_s16b();
  result.info_type = r.get_u32b();
  result.info_id = r.get_s16b();
  result.icon_type = r.get_u32b();
  result.icon_id = r.get_s16b();

  if (!r.eof()) {
    result.version = r.get_u32b();
    result.registration_flags = r.get_u32b();
    result.icon_family_id = r.get_s16b();
    
    uint32_t count = r.get_u32b();
    for (uint32_t c = 0; c < count; ++c) {
      result.platform_infos.push_back({
        r.get_u32b(),
        r.get_u32b(),
        r.get_s16b(),
        r.get_s16b()
      });
    }
    
    if (!r.eof()) {
      result.res_map_type = r.get_u32b();
      result.res_map_id = r.get_s16b();
    }
  }
  return result;
}
