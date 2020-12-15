#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <phosg/Filesystem.hh>
#include <phosg/Image.hh>

#include <vector>

#include "mc68k.hh"
#include "quickdraw_formats.hh"
#include "pict.hh"



#define RESOURCE_TYPE_ADBS  0x41444253
#define RESOURCE_TYPE_CDEF  0x43444546
#define RESOURCE_TYPE_cicn  0x6369636E
#define RESOURCE_TYPE_cfrg  0x63667267
#define RESOURCE_TYPE_clok  0x636C6F6B
#define RESOURCE_TYPE_clut  0x636C7574
#define RESOURCE_TYPE_cmid  0x636D6964
#define RESOURCE_TYPE_CODE  0x434F4445
#define RESOURCE_TYPE_crsr  0x63727372
#define RESOURCE_TYPE_csnd  0x63736E64
#define RESOURCE_TYPE_CURS  0x43555253
#define RESOURCE_TYPE_dcmp  0x64636D70
#define RESOURCE_TYPE_ecmi  0x65636D69
#define RESOURCE_TYPE_emid  0x656D6964
#define RESOURCE_TYPE_ESnd  0x45536E64
#define RESOURCE_TYPE_esnd  0x65736E64
#define RESOURCE_TYPE_icl4  0x69636C34
#define RESOURCE_TYPE_icl8  0x69636C38
#define RESOURCE_TYPE_icm4  0x69636D34
#define RESOURCE_TYPE_icm8  0x69636D38
#define RESOURCE_TYPE_icmN  0x69636D23
#define RESOURCE_TYPE_ICNN  0x49434E23
#define RESOURCE_TYPE_icns  0x69636E73
#define RESOURCE_TYPE_ICON  0x49434F4E
#define RESOURCE_TYPE_ics4  0x69637334
#define RESOURCE_TYPE_ics8  0x69637338
#define RESOURCE_TYPE_icsN  0x69637323
#define RESOURCE_TYPE_INIT  0x494E4954
#define RESOURCE_TYPE_INST  0x494E5354
#define RESOURCE_TYPE_kcs4  0x6B637334
#define RESOURCE_TYPE_kcs8  0x6B637338
#define RESOURCE_TYPE_kcsN  0x6B637323
#define RESOURCE_TYPE_LDEF  0x4C444546
#define RESOURCE_TYPE_MADH  0x4D414448
#define RESOURCE_TYPE_MDBF  0x4D444246
#define RESOURCE_TYPE_MDEF  0x4D444546
#define RESOURCE_TYPE_MIDI  0x4D494449
#define RESOURCE_TYPE_Midi  0x4D696469
#define RESOURCE_TYPE_midi  0x6D696469
#define RESOURCE_TYPE_MOOV  0x4D4F4F56
#define RESOURCE_TYPE_MooV  0x4D6F6F56
#define RESOURCE_TYPE_moov  0x6D6F6F76
#define RESOURCE_TYPE_PACK  0x5041434B
#define RESOURCE_TYPE_PAT   0x50415420
#define RESOURCE_TYPE_PATN  0x50415423
#define RESOURCE_TYPE_PICT  0x50494354
#define RESOURCE_TYPE_pltt  0x706C7474
#define RESOURCE_TYPE_ppat  0x70706174
#define RESOURCE_TYPE_pptN  0x70707423
#define RESOURCE_TYPE_proc  0x70726F63
#define RESOURCE_TYPE_PTCH  0x50544348
#define RESOURCE_TYPE_ptch  0x70746368
#define RESOURCE_TYPE_ROvr  0x524F7672
#define RESOURCE_TYPE_SERD  0x53455244
#define RESOURCE_TYPE_SICN  0x5349434E
#define RESOURCE_TYPE_SMOD  0x534D4F44
#define RESOURCE_TYPE_SMSD  0x534D5344
#define RESOURCE_TYPE_snd   0x736E6420
#define RESOURCE_TYPE_snth  0x736E7468
#define RESOURCE_TYPE_SONG  0x534F4E47
#define RESOURCE_TYPE_STR   0x53545220
#define RESOURCE_TYPE_STRN  0x53545223
#define RESOURCE_TYPE_styl  0x7374796C
#define RESOURCE_TYPE_TEXT  0x54455854
#define RESOURCE_TYPE_Tune  0x54756E65
#define RESOURCE_TYPE_WDEF  0x57444546

std::string string_for_resource_type(uint32_t type);


struct resource_fork_header {
  uint32_t resource_data_offset;
  uint32_t resource_map_offset;
  uint32_t resource_data_size;
  uint32_t resource_map_size;

  void read(int fd, size_t offset);
};

struct resource_map_header {
  uint8_t reserved[16];
  uint32_t reserved_handle;
  uint16_t reserved_file_ref_num;
  uint16_t attributes;
  uint16_t resource_type_list_offset; // relative to start of this struct
  uint16_t resource_name_list_offset; // relative to start of this struct

  void read(int fd, size_t offset);
};

struct resource_type_list_entry {
  uint32_t resource_type;
  uint16_t num_items; // actually num_items - 1
  uint16_t reference_list_offset; // relative to start of type list

  void read(int fd, size_t offset);
};

struct resource_type_list {
  uint16_t num_types; // actually num_types - 1
  std::vector<resource_type_list_entry> entries;

  void read(int fd, size_t offset);
};

struct resource_reference_list_entry {
  int16_t resource_id;
  uint16_t name_offset;
  uint32_t attributes_and_offset; // attr = high 8 bits; offset relative to resource data segment start
  uint32_t reserved;

  void read(int fd, size_t offset);
};



class ResourceFile {
public:
  ResourceFile(const std::string& filename);
  ResourceFile(const char* filename);
  virtual ~ResourceFile() = default;

  virtual bool resource_exists(uint32_t type, int16_t id);
  virtual std::string get_resource_data(uint32_t type, int16_t id,
      bool decompress = true,
      DebuggingMode decompress_debug = DebuggingMode::Disabled);
  virtual bool resource_is_compressed(uint32_t type, int16_t id);
  virtual std::vector<int16_t> all_resources_of_type(uint32_t type);
  virtual std::vector<std::pair<uint32_t, int16_t>> all_resources();

  uint32_t find_resource_by_id(int16_t id, const std::vector<uint32_t>& types);

  struct decoded_cicn {
    Image image;
    Image bitmap;

    decoded_cicn(Image&& image, Image&& bitmap);
  };

  struct decoded_CURS {
    Image bitmap;
    uint16_t hotspot_x;
    uint16_t hotspot_y;

    decoded_CURS(Image&& bitmap, uint16_t x, uint16_t y);
  };

  struct decoded_crsr {
    Image image;
    Image bitmap;
    uint16_t hotspot_x;
    uint16_t hotspot_y;

    decoded_crsr(Image&& image, Image&& bitmap, uint16_t x,
        uint16_t y);
  };

  struct decoded_INST {
    struct key_region {
      uint8_t key_low;
      uint8_t key_high;
      uint8_t base_note;
      int16_t snd_id;
      uint32_t snd_type; // can be RESOURCE_TYPE_snd or RESOURCE_TYPE_csnd

      key_region(uint8_t key_low, uint8_t key_high, uint8_t base_note,
          int16_t snd_id, uint32_t snd_type);
    };

    std::vector<key_region> key_regions;
    uint8_t base_note;
    bool use_sample_rate;
    bool constant_pitch;
  };

  struct decoded_SONG {
    int16_t midi_id;
    uint16_t tempo_bias;
    int8_t semitone_shift;
    uint8_t percussion_instrument;
    bool allow_program_change;
    std::unordered_map<uint16_t, uint16_t> instrument_overrides;
  };

  decoded_cicn decode_cicn(int16_t id, uint32_t type = RESOURCE_TYPE_cicn);
  decoded_CURS decode_CURS(int16_t id, uint32_t type = RESOURCE_TYPE_CURS);
  decoded_crsr decode_crsr(int16_t id, uint32_t type = RESOURCE_TYPE_crsr);
  std::pair<Image, Image> decode_ppat(int16_t id, uint32_t type = RESOURCE_TYPE_ppat);
  std::vector<std::pair<Image, Image>> decode_pptN(int16_t id, uint32_t type = RESOURCE_TYPE_pptN);
  Image decode_PAT(int16_t id, uint32_t type = RESOURCE_TYPE_PAT);
  std::vector<Image> decode_PATN(int16_t id, uint32_t type = RESOURCE_TYPE_PATN);
  std::vector<Image> decode_SICN(int16_t id, uint32_t type = RESOURCE_TYPE_SICN);
  Image decode_icl8(int16_t id, uint32_t type = RESOURCE_TYPE_icl8);
  Image decode_icm8(int16_t id, uint32_t type = RESOURCE_TYPE_icm8);
  Image decode_ics8(int16_t id, uint32_t type = RESOURCE_TYPE_ics8);
  Image decode_kcs8(int16_t id, uint32_t type = RESOURCE_TYPE_kcs8);
  Image decode_icl4(int16_t id, uint32_t type = RESOURCE_TYPE_icl4);
  Image decode_icm4(int16_t id, uint32_t type = RESOURCE_TYPE_icm4);
  Image decode_ics4(int16_t id, uint32_t type = RESOURCE_TYPE_ics4);
  Image decode_kcs4(int16_t id, uint32_t type = RESOURCE_TYPE_kcs4);
  Image decode_ICON(int16_t id, uint32_t type = RESOURCE_TYPE_ICON);
  Image decode_ICNN(int16_t id, uint32_t type = RESOURCE_TYPE_ICNN);
  Image decode_icmN(int16_t id, uint32_t type = RESOURCE_TYPE_icmN);
  Image decode_icsN(int16_t id, uint32_t type = RESOURCE_TYPE_icsN);
  Image decode_kcsN(int16_t id, uint32_t type = RESOURCE_TYPE_kcsN);
  decoded_INST decode_INST(int16_t id, uint32_t type = RESOURCE_TYPE_INST);
  pict_render_result decode_PICT(int16_t id, uint32_t type = RESOURCE_TYPE_PICT);
  std::vector<color> decode_pltt(int16_t id, uint32_t type = RESOURCE_TYPE_pltt);
  std::vector<color> decode_clut(int16_t id, uint32_t type = RESOURCE_TYPE_clut);
  std::string decode_snd(int16_t id, uint32_t type = RESOURCE_TYPE_snd);
  std::string decode_csnd(int16_t id, uint32_t type = RESOURCE_TYPE_csnd);
  std::string decode_esnd(int16_t id, uint32_t type = RESOURCE_TYPE_esnd);
  std::string decode_ESnd(int16_t id, uint32_t type = RESOURCE_TYPE_ESnd);
  std::string decode_cmid(int16_t id, uint32_t type = RESOURCE_TYPE_cmid);
  std::string decode_emid(int16_t id, uint32_t type = RESOURCE_TYPE_emid);
  std::string decode_ecmi(int16_t id, uint32_t type = RESOURCE_TYPE_ecmi);
  std::string decode_SMSD(int16_t id, uint32_t type = RESOURCE_TYPE_SMSD);
  decoded_SONG decode_SONG(int16_t id, uint32_t type = RESOURCE_TYPE_SONG);
  std::string decode_Tune(int16_t id, uint32_t type = RESOURCE_TYPE_Tune);
  std::pair<std::string, std::string> decode_STR(int16_t id, uint32_t type = RESOURCE_TYPE_STR);
  std::pair<std::vector<std::string>, std::string> decode_STRN(int16_t id, uint32_t type = RESOURCE_TYPE_STRN);
  std::string decode_TEXT(int16_t id, uint32_t type = RESOURCE_TYPE_TEXT);
  std::string decode_styl(int16_t id, uint32_t type = RESOURCE_TYPE_styl);
  std::string decode_CODE(int16_t id, uint32_t type = RESOURCE_TYPE_CODE);
  std::string decode_dcmp(int16_t id, uint32_t type = RESOURCE_TYPE_dcmp);
  std::string decode_CDEF(int16_t id, uint32_t type = RESOURCE_TYPE_CDEF);
  std::string decode_INIT(int16_t id, uint32_t type = RESOURCE_TYPE_INIT);
  std::string decode_LDEF(int16_t id, uint32_t type = RESOURCE_TYPE_LDEF);
  std::string decode_MDBF(int16_t id, uint32_t type = RESOURCE_TYPE_MDBF);
  std::string decode_MDEF(int16_t id, uint32_t type = RESOURCE_TYPE_MDEF);
  std::string decode_PACK(int16_t id, uint32_t type = RESOURCE_TYPE_PACK);
  std::string decode_PTCH(int16_t id, uint32_t type = RESOURCE_TYPE_PTCH);
  std::string decode_WDEF(int16_t id, uint32_t type = RESOURCE_TYPE_WDEF);
  std::string decode_ADBS(int16_t id, uint32_t type = RESOURCE_TYPE_ADBS);
  std::string decode_clok(int16_t id, uint32_t type = RESOURCE_TYPE_clok);
  std::string decode_proc(int16_t id, uint32_t type = RESOURCE_TYPE_proc);
  std::string decode_ptch(int16_t id, uint32_t type = RESOURCE_TYPE_ptch);
  std::string decode_ROvr(int16_t id, uint32_t type = RESOURCE_TYPE_ROvr);
  std::string decode_SERD(int16_t id, uint32_t type = RESOURCE_TYPE_SERD);
  std::string decode_snth(int16_t id, uint32_t type = RESOURCE_TYPE_snth);
  std::string decode_SMOD(int16_t id, uint32_t type = RESOURCE_TYPE_SMOD);

private:
  scoped_fd fd;

  bool empty;
  resource_fork_header header;
  resource_map_header map_header;
  resource_type_list map_type_list;
  std::unordered_map<uint32_t, std::vector<resource_reference_list_entry>> reference_list_cache;

  std::unordered_map<uint64_t, std::string> resource_data_cache;

  std::vector<resource_reference_list_entry>* get_reference_list(uint32_t type);
  std::string decompress_resource(const std::string& data,
      DebuggingMode debug = DebuggingMode::Disabled);
  static const std::string& get_system_decompressor(int16_t resource_id);
};


class SingleResourceFile : public ResourceFile {
public:
  SingleResourceFile(uint32_t type, int16_t id, const void* data, size_t size);
  SingleResourceFile(uint32_t type, int16_t id, const std::string& data);
  virtual ~SingleResourceFile() = default;

  virtual bool resource_exists(uint32_t type, int16_t id);
  virtual std::string get_resource_data(uint32_t type, int16_t id,
      bool decompress = true,
      DebuggingMode decompress_debug = DebuggingMode::Disabled);
  virtual bool resource_is_compressed(uint32_t type, int16_t id);
  virtual std::vector<int16_t> all_resources_of_type(uint32_t type);
  virtual std::vector<std::pair<uint32_t, int16_t>> all_resources();

private:
  uint32_t type;
  int16_t id;
  const std::string data;
};



std::string decode_mac_roman(const char* data, size_t size);
std::string decode_mac_roman(const std::string& data);
