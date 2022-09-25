#include <stdio.h>

#include "ResourceFile.hh"
#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <stdexcept>
#include <string>
#include <zlib.h>

using namespace std;


void print_usage() {
  fprintf(stderr, "\
Usage: icon_dearchiver <input-filename> [output-dir]\n\
\n\
If output-dir is not given, the directory <input-filename>.out is created and\n\
the output is written there.\n\
\n");
}

static constexpr uint8_t ICON_TYPE_COUNT = 15;

static constexpr struct {
  uint32_t  icns_type;
  //uint8_t   width;
  //uint8_t   height;
  //uint8_t   depth;
  // Only valid for non-compressed, i.e. non-RGB types
  uint32_t  size;
  uint32_t  size_in_archive;
  // RGB type instead of indexed or b/w?
  bool      is_24_bits;
  uint8_t   type_bit;
} ICON_TYPES[] = {
  // These are in the order the icons are stored in an Icon Archiver 4 file's icon data
  { resource_type("ICN#"),  256,  256, false, 5 },
  { resource_type("icl4"),  512,  512, false, 6 },
  { resource_type("icl8"), 1024, 1024, false, 7 },
  { resource_type("il32"), 3072, 4096, true,  8 },
  { resource_type("l8mk"), 1024, 1024, false, 9 },
  
  { resource_type("ics#"),   64,   64, false, 0 },
  { resource_type("ics4"),  128,  128, false, 1 },
  { resource_type("ics8"),  256,  256, false, 2 },
  { resource_type("is32"),  768, 1024, true,  3 },
  { resource_type("s8mk"),  256,  256, false, 4 },
  
  { resource_type("ich#"),  576,  576, false, 10 },
  { resource_type("ich4"), 1152, 1152, false, 11 },
  { resource_type("ich8"), 2304, 2304, false, 12 },
  { resource_type("ih32"), 6912, 9216, true,  13 },
  { resource_type("h8mk"), 2304, 2304, false, 14 },
};
static_assert(sizeof(ICON_TYPES) / sizeof(ICON_TYPES[0]) == ICON_TYPE_COUNT);

/*static constexpr uint8_t icns_type_to_icns_idx(uint32_t icns_type) {
  for (uint8_t i = 0; i < ICON_TYPE_COUNT; ++i) {
    if (ICON_TYPES[i].icns_type == icns_type) {
      return i;
    }
  }
  throw logic_error("Unsupported icns icon type");
}*/

// Order must match the one in `ICON_TYPES` above
static constexpr uint8_t ICON_TYPE_ICNN = 0;
static constexpr uint8_t ICON_TYPE_icl4 = 1;
static constexpr uint8_t ICON_TYPE_icl8 = 2;
static constexpr uint8_t ICON_TYPE_il32 = 3;
static constexpr uint8_t ICON_TYPE_l8mk = 4;

static constexpr uint8_t ICON_TYPE_icsN = 5;
static constexpr uint8_t ICON_TYPE_ics4 = 6;
static constexpr uint8_t ICON_TYPE_ics8 = 7;
static constexpr uint8_t ICON_TYPE_is32 = 8;
static constexpr uint8_t ICON_TYPE_s8mk = 9;

static constexpr uint8_t ICON_TYPE_ichN = 10;
static constexpr uint8_t ICON_TYPE_ich4 = 11;
static constexpr uint8_t ICON_TYPE_ich8 = 12;
static constexpr uint8_t ICON_TYPE_ih32 = 13;
static constexpr uint8_t ICON_TYPE_h8mk = 14;

// .icns files must contain the icons in a specific order, namely b/w icons
// last, or they don't show up correctly in Finder
// TODO: system-made .icns don't do this?
static constexpr uint8_t ICON_ICNS_ORDER[] = {
 ICON_TYPE_ics4,
 ICON_TYPE_ics8,
 ICON_TYPE_is32,
 ICON_TYPE_s8mk,
 ICON_TYPE_icl4,
 ICON_TYPE_icl8,
 ICON_TYPE_il32,
 ICON_TYPE_l8mk,
 ICON_TYPE_ich4,
 ICON_TYPE_ich8,
 ICON_TYPE_ih32,
 ICON_TYPE_h8mk,
 ICON_TYPE_icsN,
 ICON_TYPE_ICNN,
 ICON_TYPE_ichN,
};
static_assert(sizeof(ICON_ICNS_ORDER) == ICON_TYPE_COUNT * sizeof(ICON_ICNS_ORDER[0]));



static void unpack_bits(StringReader& in, void* uncompressed_data, uint32_t uncompressed_size) {
  uint8_t*        out = static_cast<uint8_t*>(uncompressed_data);
  uint8_t* const  out_end = out + uncompressed_size;
  while (out < out_end) {
    int8_t len = in.get_s8();
    if (len < 0) {
      // -len+1 repetitions of the next byte
      uint8_t byte = in.get_u8();
      for (int i = 0; i < -len + 1; ++i) {
        *out++ = byte;
      }
    } else {
      // len + 1 raw bytes
      in.readx(out, len + 1);
      out += len + 1;
    }
  }
}

static uint32_t pack_strided_bits(StringWriter& out, const void* uncompressed_data, uint32_t uncompressed_size, uint32_t uncompressed_stride) {
  // Reverse of the following decompression pseudo-code:
  //
  //  if bit 8 of the byte is set (byte >= 128, signed_byte < 0):
  //    This is a compressed run, for some value (next byte).
  //    The length is byte - 125.
  //    Put so many copies of the byte in the current color channel.
  //  else:
  //    This is an uncompressed run, whose values follow.
  //    The length is byte +1.
  //    Read the bytes and put them in the current color channel.
  //
  // From: https://www.macdisk.com/maciconen.php#RLE
  //
  const uint8_t*  in = static_cast<const uint8_t*>(uncompressed_data);
  const uint8_t*  in_end = static_cast<const uint8_t*>(uncompressed_data) + uncompressed_size;
  uint32_t        in_stride = uncompressed_stride;
  
  uint32_t        old_out_size = out.size();
  while (in < in_end) {
    if (in + 2 * in_stride < in_end && in[0] == in[in_stride] && in[0] == in[2 * in_stride]) {
      // At least three identical bytes
      uint32_t count = 3;
      while (count < 130 && in + count * in_stride < in_end && in[count * in_stride] == in[0])
        ++count;
      
      out.put_u8(count + 128 - 3);
      out.put_u8(in[0]);
      in += count * in_stride;
    } else { 
      uint32_t count = 1;
      while (count < 128 && in + count * in_stride < in_end && in[count * in_stride] != in[(count - 1) * in_stride])
        ++count;
      
      out.put_u8(count - 1);
      for (uint32_t c = count; c > 0; --c) {
        out.put_u8(*in);
        in += in_stride;
      }
    }
  }
  return out.size() - old_out_size;
}

static bool need_bw_icon(uint8_t bw_icon_type, const int32_t (&uncompressed_offsets)[ICON_TYPE_COUNT]) {
  switch (bw_icon_type) {
    case ICON_TYPE_icsN:
      return uncompressed_offsets[ICON_TYPE_ics4] >= 0 || uncompressed_offsets[ICON_TYPE_ics8] >= 0;
    
    case ICON_TYPE_ICNN:
      return uncompressed_offsets[ICON_TYPE_icl4] >= 0 || uncompressed_offsets[ICON_TYPE_icl8] >= 0;
    
    case ICON_TYPE_ichN:
      return uncompressed_offsets[ICON_TYPE_ich4] >= 0 || uncompressed_offsets[ICON_TYPE_ich8] >= 0;
  }
  return false;
}


struct DearchiverContext {
  StringReader  in;
  string        base_name;
  string        out_dir;
};


static void write_icns(
    const DearchiverContext& context,
    uint32_t icon_number, const string& icon_name,
    const char* uncompressed_data, const int32_t (&uncompressed_offsets)[ICON_TYPE_COUNT]) {
  // TODO: custom format string
  string  filename = string_printf("%s/%s_%u", context.out_dir.c_str(), context.base_name.c_str(), icon_number);
  if (!icon_name.empty()) {
    filename += "_";
    // TODO: sanitize name
    filename += icon_name;
  }
  // TODO: write icns, icl8 etc resources into single rsrc file, use filename as rsrc name
  filename += ".icns";
  
  // Start .icns file
  StringWriter data;
  data.put_u32b(0x69636E73);
  data.put_u32b(0);

  for (unsigned int t = 0; t < ICON_TYPE_COUNT; ++t) {
    uint32_t type = ICON_ICNS_ORDER[t];
    if (uncompressed_offsets[type] >= 0) {
      data.put_u32b(ICON_TYPES[type].icns_type);
      uint32_t size_pos = data.size();
      data.put_u32b(0);
      uint32_t size;
      if (ICON_TYPES[type].is_24_bits) {
        // Icon Archiver stores 24 bit icons as ARGB. The .icns format requires them to
        // be compressed one channel after the other with a PackBits-like algorithm
        size =  pack_strided_bits(data, uncompressed_data + uncompressed_offsets[type] + 1, ICON_TYPES[type].size_in_archive, 4) +
                pack_strided_bits(data, uncompressed_data + uncompressed_offsets[type] + 2, ICON_TYPES[type].size_in_archive, 4) +
                pack_strided_bits(data, uncompressed_data + uncompressed_offsets[type] + 3, ICON_TYPES[type].size_in_archive, 4) ;
      } else {
        data.write(uncompressed_data + uncompressed_offsets[type], ICON_TYPES[type].size);
        size = ICON_TYPES[type].size;
      }
      data.pput_u32b(size_pos, 8 + size);
    } else if (need_bw_icon(type, uncompressed_offsets)) {
      // If b/w icons are missing, write a black square as icon, and all pixels set as mask:
      // color icons don't display correctly without b/w icon+mask(?)
      data.put_u32b(ICON_TYPES[type].icns_type);
      data.put_u32b(8 + ICON_TYPES[type].size);
      data.extend_by(ICON_TYPES[type].size / 2, 0x00u);
      data.extend_by(ICON_TYPES[type].size / 2, 0xFFu);
    }
  }

  // Adjust .icns size
  data.pput_u32b(4, data.size());

  save_file(filename, data.str());
  fprintf(stderr, "... %s\n", filename.c_str());
}


/*static void assign_uncompressed_offset(uint32_t offset, uint32_t icns_type, int32_t (&uncompressed_offsets)[ICON_TYPE_COUNT]) {
  for (uint32_t i = 0; i < ICON_TYPE_COUNT; ++i) {
    if (ICON_TYPES[i].icns_type == icns_type) {
      uncompressed_offsets[i] = offset;
      break;
    }
  }
}*/


static void dearchive_icon(DearchiverContext& context, uint16_t version, uint32_t icon_number) {
  StringReader& r = context.in;
  uint32_t      r_where = r.where();
  
  // This includes all the icon's data, including this very uint32_t
  uint32_t icon_size = r.get_u32b();
  
  // always 0?
  r.get_u16b();
  
  // Seems related to icon_size, seems to be always 11 bytes
  // (version 1) / 10 bytes (version 2) less
  r.get_u16b();
  
  // Is the icon selected in Icon Archiver? (doesn't seem to be actually used by
  // application)
  r.get_u16b();
  
  // More icon_size relatives
  r.get_u16b();
  
  uint16_t uncompressed_icon_size = r.get_u16b();
  
  string  icon_name;
  string  uncompressed_data(uncompressed_icon_size, '\0');
  int32_t uncompressed_offsets[ICON_TYPE_COUNT];
  std::memset(uncompressed_offsets, -1, sizeof(uncompressed_offsets));
  
  if (version > 1) {
    // Version 2 has a bitfield of 15 bits (3 sizes, 5 color depth including mask)
    // for each icon that specifies which types of an icon family there are
    uint16_t icon_types = r.get_u16b();
    uint32_t offset = 0;
    for (uint32_t type = 0; type < ICON_TYPE_COUNT; ++type) {
      if (icon_types & (1 << ICON_TYPES[type].type_bit)) {
        uncompressed_offsets[type] = offset;
        
        offset += ICON_TYPES[type].size_in_archive;
      }
      if (offset > uncompressed_icon_size) {
        fprintf(stderr, "Warning: buffer overflow while decoding icon %u: %u > %u\n", icon_number, offset, uncompressed_icon_size);
      }
    }
    if (offset == 0) {
      fprintf(stderr, "Warning: empty icon %u\n", icon_number);
      return;
    }
    
    // ???
    r.get_u16b();
    
    icon_name = r.readx(r.get_u8());
    
    // Icon name seems to be both a Pascal and a C string, skip the NUL terminator
    r.get_u8();
    
    // All icons are compressed as a single blob with zlib
    uint32_t  compressed_size_zlib = r_where + icon_size - r.where();
    uLongf    uncompressed_size_zlib = uncompressed_icon_size;
    int       zlib_result = uncompress(reinterpret_cast<Bytef*>(uncompressed_data.data()), &uncompressed_size_zlib, reinterpret_cast<const Bytef*>(r.getv(compressed_size_zlib)), compressed_size_zlib);
    if (zlib_result != 0) {
      fprintf(stderr, "Warning: zlib error decompressing icon %u: %d\n", icon_number, zlib_result);
      return;
    }
    if (uncompressed_size_zlib != uncompressed_icon_size) {
      fprintf(stderr, "Warning: decompressed icon %u is of size %lu instead of %u as expected\n", icon_number, uncompressed_size_zlib, uncompressed_icon_size);
      return;
    }
  } else {
    // Version 1 uses an array of offsets from a position before the icon's name.
    // Before System 8.5 there were only 6 icon types:
    //
    //  0 = ICN#    32x32x1 with mask
    //  1 = icl4    32x32x4
    //  2 = icl8    32x32x8
    //  3 = ics#    16x16x1 with mask
    //  4 = ics4    16x16x4
    //  5 = ics8    16x16x8
    //
    // An offset of 0 means that the icon type doesn't exist. The offsets aren't
    // always in ascending order. They are into the *uncompressed* data.
    uint16_t icon_offsets[6] = {
      r.get_u16b(),
      r.get_u16b(),
      r.get_u16b(),
      r.get_u16b(),
      r.get_u16b(),
      r.get_u16b(),
    };
    
    icon_name = r.readx(r.get_u8());
    
    // The offsets don't start at 0, i.e. they aren't relative to the beginning of
    // the compressed icon data, but relative to somewhere before the icon's name
    uint16_t offset_base = icon_name.size() + 17;
    
    // All icons are compressed as a single blob with PackBits
    unpack_bits(r, uncompressed_data.data(), uncompressed_icon_size);
    
    uncompressed_offsets[ICON_TYPE_ICNN] = icon_offsets[0] - offset_base;
    uncompressed_offsets[ICON_TYPE_icl4] = icon_offsets[1] - offset_base;
    uncompressed_offsets[ICON_TYPE_icl8] = icon_offsets[2] - offset_base;
    uncompressed_offsets[ICON_TYPE_icsN] = icon_offsets[3] - offset_base;
    uncompressed_offsets[ICON_TYPE_ics4] = icon_offsets[4] - offset_base;
    uncompressed_offsets[ICON_TYPE_ics8] = icon_offsets[5] - offset_base;
  }
  
  write_icns(context, icon_number, icon_name, uncompressed_data.data(), uncompressed_offsets);
  
  // Done: continue right after the icon, skipping any possible padding
  r.go(r_where + icon_size);
}


int main(int argc, const char** argv) {
  try {
    if (argc < 2) {
      print_usage();
      return 2;
    }
    
    DearchiverContext context;
    
    for (int x = 1; x < argc; x++) {
      if (context.base_name.empty()) {
        context.base_name = argv[x];
      } else if (context.out_dir.empty()) {
        context.out_dir = argv[x];
      } else {
        fprintf(stderr, "excess argument: %s\n", argv[x]);
        print_usage();
        return 2;
      }
    }

    if (context.base_name.empty()) {
      print_usage();
      return 2;
    }
    if (context.out_dir.empty()) {
      context.out_dir = string_printf("%s.out", context.base_name.c_str());
    }
    mkdir(context.out_dir.c_str(), 0777);
    
    string        content = load_file(context.base_name);
    StringReader& r = context.in = StringReader(content);

    // Check signature ('QBSE' 'PACK')
    if (r.get_u32b() != 0x51425345 || r.get_u32b() != 0x5041434B) {
      fprintf(stderr, "File '%s' isn't an Icon Archiver file\n", context.base_name.c_str());
      return 2;
    }
    
    // ???
    r.skip(2);
    
    // Version: 1 = Icon Archiver 2; 2 = Icon Archiver 4
    uint16_t version = r.get_u16b();
    if (version != 1 && version != 2) {
      fprintf(stderr, "File '%s' is unsupported version %hu\n", context.base_name.c_str(), version);
      return 2;
    }
    
    uint32_t icon_count = r.get_u32b();
    
    // ???
    r.skip(32);
    
    // Seems to be some kind of date
    r.get_u64b();
    
    // ???
    r.skip(8);
    
    if (version > 1) {
      // Another signature? ('IAUB')
      if (r.get_u32b() != 0x49415542) {
        fprintf(stderr, "File '%s' isn't an Icon Archiver version 2 file\n", context.base_name.c_str());
        return 2;
      }
      
      // ???
      r.skip(57);
      
      // Are the copyright and comment string locked, i.e. can't be changed
      // anymore in the Icon Archiver application
      r.get_u8();
      
      // ???
      r.skip(2);
      
      // Copyright and comment strings are Pascal strings padded to a fixed length
      r.get_u8();
      string copyright = r.readx(63);
      
      r.get_u8();
      string comment = r.readx(255);
      // Comment ends at 0x1C0
      
      if (!copyright.empty() || !comment.empty()) {
        // TODO: output archive comments?
      }
      
      // After the comments there's additional ??? data and then an array of
      // uint32_t with one element for each icon in the file. All elements
      // are zero. Might be an array of offsets to the icon data, initialized
      // when loading the archive
      r.go(0x440 + 4 * icon_count);
    }
    else {
      // Same, but for Icon Archiver 2
      r.go(0x40 + 4 * icon_count);
    }
    
    for (std::uint32_t icon_no = 0; icon_no < icon_count; ++icon_no) {
      dearchive_icon(context, version, icon_no);
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }
}
