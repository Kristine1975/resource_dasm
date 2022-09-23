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

static constexpr uint32_t ICON_TYPES[] = {
  resource_type("ics#"), //  0 = 16x16x1 with mask
  resource_type("ics4"), //  1 = 16x16x4
  resource_type("ics8"), //  2 = 16x16x8
  resource_type("is32"), //  3 = 16x16x24 without mask
  resource_type("s8mk"), //  4 = 16x16x8 mask
  resource_type("ICN#"), //  5 = 32x32x1 with mask
  resource_type("icl4"), //  6 = 32x32x4
  resource_type("icl8"), //  7 = 32x32x8
  resource_type("il32"), //  8 = 32x32x24 without mask
  resource_type("l8mk"), //  9 = 32x32x8 mask
  resource_type("ich#"), // 10 = 48x48x1 with mask
  resource_type("ich4"), // 11 = 48x48x4
  resource_type("ich8"), // 12 = 48x48x8
  resource_type("ih32"), // 13 = 48x48x24 without mask
  resource_type("h8mk"), // 14 = 48x48x8 mask
};
static_assert(sizeof(ICON_TYPES) == ICON_TYPE_COUNT * sizeof(ICON_TYPES[0]));

static constexpr uint32_t ICON_SIZES[] = {
    64, //  0 = ics#    16x16x1 with mask
   128, //  1 = ics4    16x16x4
   256, //  2 = ics8    16x16x8
   768, //  3 = is32?   16x16x24 without mask
   256, //  4 = s8mk?   16x16x8 mask
   256, //  5 = ICN#    32x32x1 with mask
   512, //  6 = icl4    32x32x4
  1024, //  7 = icl8    32x32x8
  3072, //  8 = il32?   32x32x24 without mask
  1024, //  9 = l8mk?   32x32x8 mask
   576, // 10 = ich#?   48x48x1 with mask
  1152, // 11 = ich4?   48x48x4
  2304, // 12 = ich8?   48x48x8
  6912, // 13 = ih32?   48x48x24 without mask
  2304, // 14 = h8mk?   48x48x8 mask
};
static_assert(sizeof(ICON_SIZES) == ICON_TYPE_COUNT * sizeof(ICON_SIZES[0]));

// .icns files must contain the icons in a specific order, namely b/w icons
// last, or they don't show up correctly in Finder
static constexpr uint8_t ICON_ICNS_ORDER[] = {
  1, // ics4    16x16x4
  2, // ics8    16x16x8
  3, // is32?   16x16x24 without mask
  4, // s8mk?   16x16x8 mask
  6, // icl4    32x32x4
  7, // icl8    32x32x8
  8, // il32?   32x32x24 without mask
  9, // l8mk?   32x32x8 mask
 11, // ich4?   48x48x4
 12, // ich8?   48x48x8
 13, // ih32?   48x48x24 without mask
 14, // h8mk?   48x48x8 mask  
  0, // ics#    16x16x1 with mask
  5, // ICN#    32x32x1 with mask
 10, // ich#?   48x48x1 with mask
};
static_assert(sizeof(ICON_ICNS_ORDER) == ICON_TYPE_COUNT * sizeof(ICON_ICNS_ORDER[0]));



static void unpack_bits(StringReader& r, uint8_t* uncompressed_data, uint32_t uncompressed_size) {
  uint8_t*  uncompressed_end = uncompressed_data + uncompressed_size;
  while (uncompressed_data < uncompressed_end) {
    int8_t len = r.get_s8();
    if (len < 0) {
      // -len+1 repetitions of the next byte
      uint8_t byte = r.get_u8();
      for (uint32_t i = 0; i < uint32_t(-len + 1); ++i) {
        *uncompressed_data++ = byte;
      }
    } else {
      // len + 1 raw bytes
      r.readx(uncompressed_data, len + 1);
      uncompressed_data += len + 1;
    }
  }
}


static void write_icns(uint32_t icon_number, const string& icon_name, const char* uncompressed_data, const int32_t (&uncompressed_offsets)[ICON_TYPE_COUNT], const string& out_dir) {
  // TODO: custom format string
  string  filename = string_printf("%s/%u", out_dir.c_str(), icon_number);
  if (!icon_name.empty()) {
    filename += "_";
    filename += icon_name;
  }
  filename += ".icns";
  
  // Start .icns file
  StringWriter data;
  data.put_u32b(0x69636E73);
  data.put_u32b(0);

  for (uint32_t t = 0; t < ICON_TYPE_COUNT; ++t) {
    uint32_t type = ICON_ICNS_ORDER[t];
    if (uncompressed_offsets[type] >= 0) {
      data.put_u32b(ICON_TYPES[type]);
      data.put_u32b(8 + ICON_SIZES[type]);
      data.write(uncompressed_data + uncompressed_offsets[type], ICON_SIZES[type]);
    }
    // TODO: maybe check for missing b/w icons and write dummies?
  }

  // Adjust .icns size
  data.pput_u32b(4, data.size());

  save_file(filename, data.str());
  fprintf(stderr, "... %s\n", filename.c_str());
}


static void unarchive_icon(StringReader& r, uint16_t version, uint32_t icon_number, const string& out_dir) {
  uint32_t r_where = r.where();
  uint32_t compressed_icon_size = r.get_u32b();
  
  // ???
  r.get_u16b();
  
  // Seems related to compressed_icon_size, seems to be always 11 bytes
  // (version 1) / 10 bytes (version 2) less
  r.get_u16b();
  
  // Is the icon selected in Icon Archiver? (doesn't seem to be actually used by
  // application)
  r.get_u16b();
  
  // More compressed_icon_size relatives
  r.get_u16b();
  
  // The icon's size after decompression. Version 1 uses UnpackBits, so this
  // says when to stop. Version 2 uses ZIP, so this is the size of the
  // destination buffer
  uint32_t uncompressed_icon_size = r.get_u32b();
  
  string  icon_name;
  string  uncompressed_data(uncompressed_icon_size, '\0');
  int32_t uncompressed_offsets[] = {
    -1, //  0 = ics#    16x16x1 with mask
    -1, //  1 = ics4    16x16x4
    -1, //  2 = ics8    16x16x8
    -1, //  3 = is32?   16x16x24 without mask
    -1, //  4 = s8mk?   16x16x8 mask
    -1, //  5 = ICN#    32x32x1 with mask
    -1, //  6 = icl4    32x32x4
    -1, //  7 = icl8    32x32x8
    -1, //  8 = il32?   32x32x24 without mask
    -1, //  9 = l8mk?   32x32x8 mask
    -1, // 10 = ich#?   48x48x1 with mask
    -1, // 11 = ich4?   48x48x4
    -1, // 12 = ich8?   48x48x8
    -1, // 13 = ih32?   48x48x24 without mask
    -1, // 14 = h8mk?   48x48x8 mask
  };
  static_assert(sizeof(uncompressed_offsets) == sizeof(ICON_TYPES));
  
  if (version > 1) {
    // Version 2 has a bitfield of 15 bits (3 sizes, 5 color depth including mask)
    // for each icon that specifies which types of an icon family there are (see
    // offset array above)
    uint16_t icon_types = r.get_u16b();
    
    // ???
    r.get_u16b();
    
    icon_name = r.readx(r.get_u8());
    
    // Icon name seems to be both a Pascal and a C string, skip the NUL terminator
    r.get_u8();
    
    // All icons are compressed as a single blob with zlib
    uLongf  uncompressed_size_zlib = 0;
    uncompress(reinterpret_cast<Bytef*>(uncompressed_data.data()), &uncompressed_size_zlib, reinterpret_cast<const Bytef*>(r.peek(compressed_icon_size)), compressed_icon_size);
    if (uncompressed_size_zlib != uncompressed_icon_size) {
      // TODO: size mismatch, error, possibly corrupted archive data
    }
    
    // Fill offsets
    uint32_t offset = 0;
    for (uint32_t type = 0; type < ICON_TYPE_COUNT; ++type) {
      if (icon_types & (1 << type)) {
        uncompressed_offsets[type] = offset;
        
        offset += ICON_SIZES[type];
      }
    }
  } else {
    // Version 1 uses an array of offsets from a position before the icon's name.
    // Before System 8.5 there were only 6 icon types:
    //
    //  0 = ics#    16x16x1 with mask
    //  1 = ics4    16x16x4
    //  2 = ics8    16x16x8
    //  3 = ICN#    32x32x1 with mask
    //  4 = icl4    32x32x4
    //  5 = icl8    32x32x8
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
    unpack_bits(r, reinterpret_cast<uint8_t*>(uncompressed_data.data()), uncompressed_icon_size);
    
    uncompressed_offsets[0] = icon_offsets[0] - offset_base;
    uncompressed_offsets[1] = icon_offsets[1] - offset_base;
    uncompressed_offsets[2] = icon_offsets[2] - offset_base;
    uncompressed_offsets[5] = icon_offsets[3] - offset_base;
    uncompressed_offsets[6] = icon_offsets[4] - offset_base;
    uncompressed_offsets[7] = icon_offsets[5] - offset_base;
  }
  
  write_icns(icon_number, icon_name, uncompressed_data.data(), uncompressed_offsets, out_dir);
  
  // Done: continue right after the icon, skipping any possible padding
  r.go(r_where);
}


int main(int argc, const char** argv) {
  try {
    if (argc <= 2) {
      print_usage();
      return 1;
    }
    string      filename;
    string      out_dir;
    
    for (int x = 1; x < argc; x++) {
      if (filename.empty()) {
        filename = argv[x];
      } else if (out_dir.empty()) {
        out_dir = argv[x];
      } else {
        fprintf(stderr, "excess argument: %s\n", argv[x]);
        print_usage();
        return 2;
      }
    }

    if (filename.empty()) {
      print_usage();
      return 2;
    }
    if (out_dir.empty()) {
      out_dir = string_printf("%s.out", filename.c_str());
    }
    mkdir(out_dir.c_str(), 0777);
    
    string data = load_file(filename);
    StringReader r(data.data(), data.size());

    // Check signature ('QBSE' 'PACK')
    if (r.get_u32b() != 0x51425345 || r.get_u32b() != 0x5041434B) {
      fprintf(stderr, "File '%s' isn't an Icon Archiver file\n", filename.c_str());
      return 2;
    }
    
    // ???
    r.skip(2);
    
    // Version: 1 = Icon Archiver 2; 2 = Icon Archiver 4
    uint16_t version = r.get_u16b();
    if (version != 1 && version != 2) {
      fprintf(stderr, "File '%s' is unsupported version %hu\n", filename.c_str(), version);
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
        fprintf(stderr, "File '%s' isn't an Icon Archiver version 2 file\n", filename.c_str());
        return 2;
      }
      
      // ???
      r.skip(57);
      
      // Are the copyright and comment string locked, i.e. can't be changed
      // anymore in the Icon Archiver application
      r.get_u8();
      
      // ???
      r.skip(2);
      
      // Length of copyright string
      r.get_u8();
      string copyright = r.readx(63);
      // Length of comment string
      r.get_u8();
      string comment = r.readx(255);
      // Comment ends at 0x1C0
      
      if (!copyright.empty() || !comment.empty()) {
        // TODO: output archive comments
      }
      
      // After the comments there's additional ??? data and then an array of
      // uint32_t with one element for each icon in the file. All elements
      // are zero. Might be an array of offsets to the icon data, initialized
      // when loading the archive
      r.go(0x440 + 4 * icon_count);
    }
    else {
      // Same, but for Icon Archiver 2.x
      r.go(0x40 + 4 * icon_count);
    }
    
    for (std::uint32_t icon_no = 0; icon_no < icon_count; ++icon_no) {
      unarchive_icon(r, version, icon_no, out_dir);
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
  } catch (...) {
    fprintf(stderr, "Unknown error\n");
  }
}
