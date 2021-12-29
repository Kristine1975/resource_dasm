#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <unordered_map>
#include <vector>

#include "RealmzLib.hh"

using namespace std;



static string first_file_that_exists(const vector<string>& names) {
  for (const auto& it : names) {
    struct stat st;
    if (stat(it.c_str(), &st) == 0) {
      return it;
    }
  }
  return "";
}



static unordered_map<string, TileSetDefinition> load_default_tilesets(
    const string& data_dir) {
  static const unordered_map<string, vector<string>> land_type_to_filenames({
    {"indoor",  {"data_castle_bd", "Data Castle BD", "DATA CASTLE BD"}},
    {"desert",  {"data_desert_bd", "Data Desert BD", "DATA DESERT BD"}},
    {"outdoor", {"data_p_bd", "Data P BD", "DATA P BD"}},
    {"snow",    {"data_snow_bd", "Data Snow BD", "DATA SNOW BD"}},
    {"cave",    {"data_sub_bd", "Data SUB BD", "DATA SUB BD"}},
    {"abyss",   {"data_swamp_bd", "Data Swamp BD", "DATA SWAMP BD"}},
  });
  unordered_map<string, TileSetDefinition> tilesets;
  for (const auto& it : land_type_to_filenames) {
    vector<string> filenames;
    for (const auto& filename : it.second)
      filenames.emplace_back(string_printf("%s/%s", data_dir.c_str(),
          filename.c_str()));

    string filename = first_file_that_exists(filenames);
    if (!filename.empty()) {
      printf("loading tileset %s definition\n", it.first.c_str());
      tilesets.emplace(it.first, load_tileset_definition(filename));
      populate_custom_tileset_configuration(it.first, tilesets[it.first]);
    } else {
      printf("warning: tileset definition for %s is missing\n",
          it.first.c_str());
    }
  }

  return tilesets;
}

int disassemble_scenario(const string& data_dir, const string& scenario_dir,
    const string& out_dir) {

  string scenario_name;
  {
    size_t where = scenario_dir.rfind('/');
    if (where == string::npos) {
      scenario_name = scenario_dir;
    } else {
      scenario_name = scenario_dir.substr(where + 1);
    }
  }

  printf("scenario directory: %s\n", scenario_dir.c_str());
  printf("disassembly directory: %s\n", out_dir.c_str());

  // Find all the files
  string scenario_metadata_name = scenario_dir + "/" + scenario_name;
  string global_metadata_name = first_file_that_exists({
      (scenario_dir + "/global"),
      (scenario_dir + "/Global")});
  string dungeon_map_index_name = first_file_that_exists({
      (scenario_dir + "/data_dl"),
      (scenario_dir + "/Data DL"),
      (scenario_dir + "/DATA DL")});
  string land_map_index_name = first_file_that_exists({
      (scenario_dir + "/data_ld"),
      (scenario_dir + "/Data LD"),
      (scenario_dir + "/DATA LD")});
  string string_index_name = first_file_that_exists({
      (scenario_dir + "/data_sd2"),
      (scenario_dir + "/Data SD2"),
      (scenario_dir + "/DATA SD2")});
  string ecodes_index_name = first_file_that_exists({
      (scenario_dir + "/data_edcd"),
      (scenario_dir + "/Data EDCD"),
      (scenario_dir + "/DATA EDCD")});
  string land_ap_index_name = first_file_that_exists({
      (scenario_dir + "/data_dd"),
      (scenario_dir + "/Data DD"),
      (scenario_dir + "/DATA DD")});
  string dungeon_ap_index_name = first_file_that_exists({
      (scenario_dir + "/data_ddd"),
      (scenario_dir + "/Data DDD"),
      (scenario_dir + "/DATA DDD")});
  string extra_ap_index_name = first_file_that_exists({
      (scenario_dir + "/data_ed3"),
      (scenario_dir + "/Data ED3"),
      (scenario_dir + "/DATA ED3")});
  string land_metadata_index_name = first_file_that_exists({
      (scenario_dir + "/data_rd"),
      (scenario_dir + "/Data RD"),
      (scenario_dir + "/DATA RD")});
  string dungeon_metadata_index_name = first_file_that_exists({
      (scenario_dir + "/data_rdd"),
      (scenario_dir + "/Data RDD"),
      (scenario_dir + "/DATA RDD")});
  string simple_encounter_index_name = first_file_that_exists({
      (scenario_dir + "/data_ed"),
      (scenario_dir + "/Data ED"),
      (scenario_dir + "/DATA ED")});
  string complex_encounter_index_name = first_file_that_exists({
      (scenario_dir + "/data_ed2"),
      (scenario_dir + "/Data ED2"),
      (scenario_dir + "/DATA ED2")});
  string party_map_index_name = first_file_that_exists({
      (scenario_dir + "/data_md2"),
      (scenario_dir + "/Data MD2"),
      (scenario_dir + "/DATA MD2")});
  string treasure_index_name = first_file_that_exists({
      (scenario_dir + "/data_td"),
      (scenario_dir + "/Data TD"),
      (scenario_dir + "/DATA TD")});
  string rogue_encounter_index_name = first_file_that_exists({
      (scenario_dir + "/data_td2"),
      (scenario_dir + "/Data TD2"),
      (scenario_dir + "/DATA TD2")});
  string time_encounter_index_name = first_file_that_exists({
      (scenario_dir + "/data_td3"),
      (scenario_dir + "/Data TD3"),
      (scenario_dir + "/DATA TD3")});
  string scenario_resources_name = first_file_that_exists({
      (scenario_dir + "/scenario.rsf"),
      (scenario_dir + "/Scenario.rsf"),
      (scenario_dir + "/SCENARIO.RSF"),
      (scenario_dir + "/scenario/rsrc"),
      (scenario_dir + "/Scenario/rsrc"),
      (scenario_dir + "/SCENARIO/rsrc"),
      (scenario_dir + "/scenario/..namedfork/rsrc"),
      (scenario_dir + "/Scenario/..namedfork/rsrc"),
      (scenario_dir + "/SCENARIO/..namedfork/rsrc")});
  string the_family_jewels_name = first_file_that_exists({
      (data_dir + "/the_family_jewels.rsf"),
      (data_dir + "/The Family Jewels.rsf"),
      (data_dir + "/THE FAMILY JEWELS.RSF"),
      (data_dir + "/the_family_jewels/rsrc"),
      (data_dir + "/The Family Jewels/rsrc"),
      (data_dir + "/THE FAMILY JEWELS/rsrc"),
      (data_dir + "/the_family_jewels/..namedfork/rsrc"),
      (data_dir + "/The Family Jewels/..namedfork/rsrc"),
      (data_dir + "/THE FAMILY JEWELS/..namedfork/rsrc")});

  // Load images
  populate_image_caches(the_family_jewels_name);

  // Load everything else
  printf("loading dungeon map index\n");
  auto dungeon_maps = load_dungeon_map_index(dungeon_map_index_name);
  printf("loading land map index\n");
  auto land_maps = load_land_map_index(land_map_index_name);
  printf("loading string index\n");
  auto strings = load_string_index(string_index_name);
  printf("loading ecodes index\n");
  auto ecodes = load_ecodes_index(ecodes_index_name);
  printf("loading dungeon action point index\n");
  auto dungeon_aps = load_ap_index(dungeon_ap_index_name);
  printf("loading land action point index\n");
  auto land_aps = load_ap_index(land_ap_index_name);
  printf("loading extra action point index\n");
  auto xaps = load_xap_index(extra_ap_index_name);
  printf("loading dungeon map metadata index\n");
  auto dungeon_metadata = load_map_metadata_index(dungeon_metadata_index_name);
  printf("loading land map metadata index\n");
  auto land_metadata = load_map_metadata_index(land_metadata_index_name);
  printf("loading simple encounter index\n");
  auto simple_encs = load_simple_encounter_index(simple_encounter_index_name);
  printf("loading complex encounter index\n");
  auto complex_encs = load_complex_encounter_index(complex_encounter_index_name);
  printf("loading party map index\n");
  auto party_maps = load_party_map_index(party_map_index_name);
  printf("loading treasure index\n");
  auto treasures = load_treasure_index(treasure_index_name);
  printf("loading rogue encounter index\n");
  auto rogue_encs = load_rogue_encounter_index(rogue_encounter_index_name);
  printf("loading time encounter index\n");
  auto time_encs = load_time_encounter_index(time_encounter_index_name);
  printf("loading global metadata\n");
  GlobalMetadata global;
  try {
    global = load_global_metadata(global_metadata_name);
  } catch (const exception& e) {
    printf("warning: global metadata appears to be missing\n");
  }
  printf("loading scenario metadata\n");
  auto scen_metadata = load_scenario_metadata(scenario_metadata_name);
  printf("loading picture resources\n");
  unordered_map<int16_t, Image> picts = get_picts(scenario_resources_name);
  printf("loading icon resources\n");
  unordered_map<int16_t, ResourceFile::DecodedColorIconResource> cicns = get_cicns(scenario_resources_name);
  printf("loading sound resources\n");
  unordered_map<int16_t, string> snds = get_snds(scenario_resources_name);
  printf("loading text resources\n");
  unordered_map<int16_t, pair<string, bool>> texts = get_texts(scenario_resources_name);

  // Load layout separately because it doesn't have to exist
  LandLayout layout;
  {
    string fname = first_file_that_exists({
        (scenario_dir + "/layout"),
        (scenario_dir + "/Layout")});
    if (!fname.empty()) {
      layout = load_land_layout(fname);
    } else {
      printf("note: this scenario has no land layout information\n");
    }
  }

  // Load default tilesets
  unordered_map<string, TileSetDefinition> tilesets = load_default_tilesets(
      data_dir);

  // If custom tilesets exist for this scenario, load them
  unordered_map<int, TileSetDefinition> custom_tilesets;
  for (int x = 1; x < 4; x++) {
    string fname = first_file_that_exists({
        string_printf("%s/data_custom_%d_bd", scenario_dir.c_str(), x),
        string_printf("%s/Data Custom %d BD", scenario_dir.c_str(), x),
        string_printf("%s/DATA CUSTOM %d BD", scenario_dir.c_str(), x)});
    if (!fname.empty()) {
      printf("loading custom tileset %d definition\n", x);
      custom_tilesets.emplace(x, load_tileset_definition(fname));
      populate_custom_tileset_configuration(string_printf("custom_%d", x),
          custom_tilesets[x]);
    }
  }

  // Make necessary directories for output
  {
    mkdir(out_dir.c_str(), 0755);
    string filename = string_printf("%s/media", out_dir.c_str());
    mkdir(filename.c_str(), 0755);
  }

  // Disassemble scenario text
  {
    string filename = string_printf("%s/script.txt", out_dir.c_str());
    auto f = fopen_unique(filename.c_str(), "wt");

    // global metadata
    printf("... %s (global metadata)\n", filename.c_str());
    fwritex(f.get(), disassemble_globals(global));
 
    // treasures
    printf("... %s (treasures)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_treasures(treasures));

    // party maps
    printf("... %s (party_maps)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_party_maps(party_maps));

    // simple encounters
    printf("... %s (simple encounters)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_simple_encounters(simple_encs, ecodes, strings));

    // complex encounters
    printf("... %s (complex encounters)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_complex_encounters(complex_encs, ecodes, strings));

    // rogue encounters
    printf("... %s (rogue encounters)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_rogue_encounters(rogue_encs, strings));

    // time encounters
    printf("... %s (time encounters)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_time_encounters(time_encs));

    // dungeon APs
    printf("... %s (dungeon APs)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_aps(dungeon_aps, ecodes, strings, 1));

    // land APs
    printf("... %s (land APs)\n", filename.c_str());
    fwritex(f.get(), disassemble_all_aps(land_aps, ecodes, strings, 0));

    // extra APs
    printf("... %s (extra APs)\n", filename.c_str());
    fwritex(f.get(), disassemble_xaps(xaps, ecodes, strings, land_metadata, dungeon_metadata));
  }

  // Save media
  for (const auto& it : picts) {
    string filename = string_printf("%s/media/picture_%d.bmp", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    it.second.save(filename.c_str(), Image::WindowsBitmap);
  }
  for (const auto& it : cicns) {
    string filename = string_printf("%s/media/icon_%d.bmp", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    it.second.image.save(filename.c_str(), Image::WindowsBitmap);
  }
  for (const auto& it : snds) {
    string filename = string_printf("%s/media/snd_%d.wav", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    save_file(filename.c_str(), it.second);
  }
  for (const auto& it : texts) {
    string filename = string_printf("%s/media/text_%d.%s", out_dir.c_str(),
        it.first, it.second.second ? "rtf" : "txt");
    printf("... %s\n", filename.c_str());
    save_file(filename, it.second.first);
  }

  // Generate custom tileset legends
  for (auto it : custom_tilesets) {
    try {
      string filename = string_printf("%s/tileset_custom_%d_legend.bmp",
          out_dir.c_str(), it.first);
      printf("... %s\n", filename.c_str());
      Image legend = generate_tileset_definition_legend(it.second,
          string_printf("custom_%d", it.first), scenario_resources_name);
      legend.save(filename.c_str(), Image::WindowsBitmap);
    } catch (const out_of_range&) {
      // Scenario doesn't contain this land type
    } catch (const runtime_error& e) {
      printf("warning: can\'t generate legend for custom tileset %d (%s)\n",
          it.first, e.what());
    }
  }

  // Generate dungeon maps
  for (size_t x = 0; x < dungeon_maps.size(); x++) {
    string filename = string_printf("%s/dungeon_%d.bmp", out_dir.c_str(), x);
    printf("... %s\n", filename.c_str());
    Image map = generate_dungeon_map(dungeon_maps[x], dungeon_metadata[x],
        dungeon_aps[x]);
    map.save(filename.c_str(), Image::WindowsBitmap);
  }

  // Generate land maps
  unordered_map<int16_t, string> level_id_to_filename;
  for (size_t x = 0; x < land_maps.size(); x++) {

    LevelNeighbors n;
    try {
      n = get_level_neighbors(layout, x);
    } catch (const runtime_error& e) {
      printf("warning: can\'t get neighbors for level (%s)\n", e.what());
    }

    int16_t start_x = -1, start_y = -1;
    if (x == (size_t)scen_metadata.start_level) {
      start_x = scen_metadata.start_x;
      start_y = scen_metadata.start_y;
    }

    try {
      string filename = string_printf("%s/land_%d.bmp", out_dir.c_str(), x);
      printf("... %s\n", filename.c_str());
      Image map = generate_land_map(land_maps[x], land_metadata[x], land_aps[x],
          n, start_x, start_y, scenario_resources_name);
      map.save(filename.c_str(), Image::WindowsBitmap);
      level_id_to_filename[x] = filename;

    } catch (const out_of_range& e) {
      printf("error: can\'t render with selected tileset (%s)\n", e.what());
    } catch (const runtime_error& e) {
      printf("error: can\'t render with selected tileset (%s)\n", e.what());
    }
  }

  // Generate connected land map
  for (auto layout_component : get_connected_components(layout)) {
    if (layout_component.num_valid_levels() < 2) {
      continue;
    }
    try {
      string filename = string_printf("%s/land_connected", out_dir.c_str());
      for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 16; x++) {
          if (layout_component.layout[y][x] != -1) {
            filename += string_printf("_%d", layout_component.layout[y][x]);
          }
        }
      }
      filename += ".bmp";
      printf("... %s\n", filename.c_str());

      Image connected_map = generate_layout_map(layout_component,
          level_id_to_filename);
      connected_map.save(filename.c_str(), Image::WindowsBitmap);
    } catch (const runtime_error& e) {
      printf("warning: can\'t generate connected land map: %s\n", e.what());
    }
  }

  return 0;
}



int disassemble_global_data(const string& data_dir, const string& out_dir) {

  printf("global data directory: %s\n", data_dir.c_str());
  printf("disassembly directory: %s\n", out_dir.c_str());

  // Find all the files
  string the_family_jewels_name = first_file_that_exists({
      (data_dir + "/the_family_jewels.rsf"),
      (data_dir + "/The Family Jewels.rsf"),
      (data_dir + "/THE FAMILY JEWELS.RSF"),
      (data_dir + "/the_family_jewels/rsrc"),
      (data_dir + "/The Family Jewels/rsrc"),
      (data_dir + "/THE FAMILY JEWELS/rsrc"),
      (data_dir + "/the_family_jewels/..namedfork/rsrc"),
      (data_dir + "/The Family Jewels/..namedfork/rsrc"),
      (data_dir + "/THE FAMILY JEWELS/..namedfork/rsrc")});
  string portraits_name = first_file_that_exists({
      (data_dir + "/portraits.rsf"),
      (data_dir + "/Portraits.rsf"),
      (data_dir + "/PORTRAITS.RSF"),
      (data_dir + "/portraits/rsrc"),
      (data_dir + "/Portraits/rsrc"),
      (data_dir + "/PORTRAITS/rsrc"),
      (data_dir + "/portraits/..namedfork/rsrc"),
      (data_dir + "/Portraits/..namedfork/rsrc"),
      (data_dir + "/PORTRAITS/..namedfork/rsrc")});

  printf("found data file: %s\n", the_family_jewels_name.c_str());
  printf("found data file: %s\n", portraits_name.c_str());

  // Load resources
  printf("loading picture resources\n");
  unordered_map<int16_t, Image> picts = get_picts(the_family_jewels_name);
  printf("loading icon resources\n");
  unordered_map<int16_t, ResourceFile::DecodedColorIconResource> cicns = get_cicns(the_family_jewels_name);
  printf("loading sound resources\n");
  unordered_map<int16_t, string> snds = get_snds(the_family_jewels_name);
  printf("loading text resources\n");
  unordered_map<int16_t, pair<string, bool>> texts = get_texts(the_family_jewels_name);
  printf("loading portraits\n");
  unordered_map<int16_t, ResourceFile::DecodedColorIconResource> portrait_cicns = get_cicns(portraits_name);

  // Load images
  populate_image_caches(the_family_jewels_name);

  // Load default tilesets
  unordered_map<string, TileSetDefinition> tilesets = load_default_tilesets(
      data_dir);

  // Make necessary directories for output
  {
    mkdir(out_dir.c_str(), 0755);
    string filename = string_printf("%s/media", out_dir.c_str());
    mkdir(filename.c_str(), 0755);
  }

  // Save media
  for (const auto& it : picts) {
    string filename = string_printf("%s/media/picture_%d.bmp", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    it.second.save(filename.c_str(), Image::WindowsBitmap);
  }
  for (const auto& it : cicns) {
    string filename = string_printf("%s/media/icon_%d.bmp", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    it.second.image.save(filename.c_str(), Image::WindowsBitmap);
  }
  for (const auto& it : portrait_cicns) {
    string filename = string_printf("%s/media/portrait_icon_%d.bmp", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    it.second.image.save(filename.c_str(), Image::WindowsBitmap);
  }
  for (const auto& it : snds) {
    string filename = string_printf("%s/media/snd_%d.wav", out_dir.c_str(), it.first);
    printf("... %s\n", filename.c_str());
    save_file(filename, it.second);
  }
  for (const auto& it : texts) {
    string filename = string_printf("%s/media/text_%d.%s", out_dir.c_str(),
        it.first, it.second.second ? "rtf" : "txt");
    printf("... %s\n", filename.c_str());
    save_file(filename, it.second.first);
  }

  // Generate custom tileset legends
  for (auto it : tilesets) {
    try {
      string filename = string_printf("%s/tileset_%s_legend.bmp",
          out_dir.c_str(), it.first.c_str());
      printf("... %s\n", filename.c_str());
      Image legend = generate_tileset_definition_legend(it.second, it.first,
          the_family_jewels_name);
      legend.save(filename.c_str(), Image::WindowsBitmap);
    } catch (const runtime_error& e) {
      printf("warning: can\'t generate legend for tileset %s (%s)\n",
          it.first.c_str(), e.what());
    }
  }

  return 0;
}



int main(int argc, char* argv[]) {

  printf("fuzziqer software realmz scenario disassembler\n\n");

  if (argc < 3 || argc > 4) {
    printf("usage: %s data_dir [scenario_dir] out_dir\n", argv[0]);
    return 1;
  }

  if (argc == 4) {
    return disassemble_scenario(argv[1], argv[2], argv[3]);
  } else {
    return disassemble_global_data(argv[1], argv[2]);
  }
}
