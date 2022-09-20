#include <stdio.h>

#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <stdexcept>
#include <string>

using namespace std;


void print_usage() {
  fprintf(stderr, "\
Usage: icon_dearchiver <input-filename> [output-dir]\n\
\n\
If output-dir is not given, the directory <input-filename>.out is created and\n\
the output is written there.\n\
\n");
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
      // anymore in the Icon Archiver application?
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
      
      // After the comments there's additional ??? data and then an array of
      // uint32_t with one element for each icon in the file. All elements
      // are zero.
      // Maybe an array of offsets to the icon data, calculated when loading
      // the archive?
      r.go(0x440 + 4 * icon_count);
    }
    else {
      // Same
      r.go(0x40 + 4 * icon_count);
    }
    
    for (std::uint32_t icon_no = 0; icon_no < icon_count; ++icon_no) {
      uint32_t compressed_icon_size = r.get_u32b();
      // ???
      r.get_u16b();
      // Seems related to compressed_icon_size, seems to be always 11 bytes
      // (version 1) / 10 bytes (version 2) less
      r.get_u16b();
      // Is the icon selected in Icon Archiver?
      r.get_u16b();
      // More compressed_icon_size relatives
      r.get_u16b();
      
      // The icon's size after decompression. Version 1 uses UnpackBits, so this
      // says when to stop. Version 2 uses ZIP, so this is the size of the
      // destination buffer
      uint32_t uncompressed_icon_size = r.get_u32b();
      
      if (version > 1) {
        // Version 2 has a bitfield of 15 bits (3 sizes, 5 color depth including mask)
        // for each icon that specifies which types of an icon family there are:
        //
        //  0 = ics#    16x16x1 with mask
        //  1 = ics4    16x16x4
        //  2 = ics8    16x16x8
        //  3 = is32?   16x16x24 without mask
        //  4 = s8mk?   16x16x8 mask
        //  5 = ICN#    32x32x1 with mask
        //  6 = icl4    32x32x4
        //  7 = icl8    32x32x8
        //  8 = il32?   32x32x24 without mask
        //  9 = l8mk?   32x32x8 mask
        // 10 = ich#?   48x48x1 with mask
        // 11 = ich4?   48x48x4
        // 12 = ich8?   48x48x8
        // 13 = ih32?   48x48x24 without mask
        // 14 = h8mk?   48x48x8 mask
        uint16_t icon_types = r.get_u16b();
        // ???
        r.get_u16b();
        string icon_name = r.readx(r.get_u8());
        // Icon name seems to be both a Pascal and a C string, skip the NUL terminator
        r.get_u8();
        
        // TODO: get zipped icon data, uncompress
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
        // always in ascending order.
        uint16_t icon_offsets[6] = {
          r.get_u16b(),
          r.get_u16b(),
          r.get_u16b(),
          r.get_u16b(),
          r.get_u16b(),
          r.get_u16b(),
        };
        string icon_name = r.readx(r.get_u8());
        // The offsets don't start at 0
        uint16_t icon_offset_base = icon_name.size() + 17;
      }
      
      // TODO: iterate over icons and write each to .icns
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
  } catch (...) {
    fprintf(stderr, "Unknown error\n");
  }
}
