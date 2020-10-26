#include <inttypes.h>
#include <stdlib.h>

#include <phosg/Encoding.hh>
#include <phosg/Filesystem.hh>
#include <phosg/Image.hh>
#include <phosg/Strings.hh>
#include <stdexcept>
#include <vector>

#include "resource_fork.hh"

using namespace std;



struct MonkeyShinesRoom {
  struct EnemyEntry {
    uint16_t y_pixels;
    uint16_t x_pixels;
    int16_t y_min;
    int16_t x_min;
    int16_t y_max;
    int16_t x_max;
    int16_t y_speed; // in pixels per frame
    int16_t x_speed; // in pixels per frame
    int16_t type;
    uint16_t flags;

    // sprite flags are:
    // - increasing frames or cycling frames
    // - slow animation
    // - two sets horizontal
    // - two sets vertical
    // - normal sprite, energy drainer, or door

    void byteswap() {
      this->y_pixels = bswap16(this->y_pixels);
      this->x_pixels = bswap16(this->x_pixels);
      this->y_min = bswap16(this->y_min);
      this->x_min = bswap16(this->x_min);
      this->y_max = bswap16(this->y_max);
      this->x_max = bswap16(this->x_max);
      this->y_speed = bswap16(this->y_speed);
      this->x_speed = bswap16(this->x_speed);
      this->type = bswap16(this->type);
      this->flags = bswap16(this->flags);
    }
  } __attribute__((packed));

  struct BonusEntry {
    uint16_t y_pixels;
    uint16_t x_pixels;
    int32_t unknown[3]; // these appear to be unused
    int16_t type;
    uint16_t id;

    void byteswap() {
      this->y_pixels = bswap16(this->y_pixels);
      this->x_pixels = bswap16(this->x_pixels);
      this->type = bswap16(this->type);
      this->id = bswap16(this->id);
    }
  } __attribute__((packed));

  uint16_t enemy_count;
  uint16_t bonus_count;
  EnemyEntry enemies[10];
  BonusEntry bonuses[25];
  uint16_t tile_ids[0x20 * 0x14]; // width=32, height=20
  uint16_t player_start_y; // unused except in rooms 1000 and 10000
  uint16_t player_start_x; // unused except in rooms 1000 and 10000
  uint16_t background_ppat_id;

  void byteswap() {
    this->enemy_count = bswap16(this->enemy_count);
    this->bonus_count = bswap16(this->bonus_count);
    for (size_t x = 0; x < sizeof(this->enemies) / sizeof(this->enemies[0]); x++) {
      this->enemies[x].byteswap();
    }
    for (size_t x = 0; x < sizeof(this->bonuses) / sizeof(this->bonuses[0]); x++) {
      this->bonuses[x].byteswap();
    }
    for (size_t x = 0; x < 0x20 * 0x14; x++) {
      this->tile_ids[x] = bswap16(this->tile_ids[x]);
    }
    this->player_start_y = bswap16(this->player_start_y);
    this->player_start_x = bswap16(this->player_start_x);
    this->background_ppat_id = bswap16(this->background_ppat_id);
  }
} __attribute__((packed));

struct MonkeyShinesWorld {
  uint16_t num_exit_keys;
  uint16_t num_bonus_keys;
  uint16_t num_bonuses;
  int16_t exit_door_room;
  int16_t bonus_door_room;

  // hazard types are:
  // 1 - burned
  // 2 - electrocuted
  // 3 - bee sting
  // 4 - fall
  // 5 - monster
  uint16_t hazard_types[16];
  uint8_t hazards_explode[16]; // really just an array of bools
  // hazard death sounds are:
  // 12 - normal
  // 13 - death from long fall
  // 14 - death from bee sting
  // 15 - death from bomb
  // 16 - death by electrocution
  // 20 - death by lava
  uint16_t hazard_death_sounds[16];
  // explosion sounds can be any of the above or 18 (bomb explosion)
  uint16_t hazard_explosion_sounds[16];

  void byteswap() {
    this->num_exit_keys = bswap16(this->num_exit_keys);
    this->num_bonus_keys = bswap16(this->num_bonus_keys);
    this->num_bonuses = bswap16(this->num_bonuses);
    this->exit_door_room = bswap16(this->exit_door_room);
    this->bonus_door_room = bswap16(this->bonus_door_room);
    for (size_t z = 0; z < 16; z++) {
      this->hazard_types[z] = bswap16(this->hazard_types[z]);
      this->hazard_death_sounds[z] = bswap16(this->hazard_death_sounds[z]);
      this->hazard_explosion_sounds[z] = bswap16(this->hazard_explosion_sounds[z]);
    }
  }
};



vector<unordered_map<int16_t, pair<int16_t, int16_t>>> generate_room_placement_maps(
    const vector<int16_t>& room_ids) {
  unordered_set<int16_t> remaining_room_ids(room_ids.begin(), room_ids.end());

  // the basic idea is that when bonzo moves right or left out of a room, the
  // room number is increased or decreased by 1; when he moves up or down out of
  // a room, it's increased or decreased by 100. there's no notion of rooms
  // linking to each other; it's done implicitly by the room ids (resource ids).
  // to convert it into something we can actually use, we have to find all the
  // connected components of this graph.

  // it occurs to me that this function might be a good basic interview question

  // this recursive lambda adds a single room to a placement map, then uses the
  // flood-fill algorithm to find all the rooms it's connected to. this
  // declaration looks super-gross because lambda can't be recursive if you
  // declare them with auto... sigh
  function<void(unordered_map<int16_t, pair<int16_t, int16_t>>&, int16_t room_id,
      int16_t x_offset, int16_t y_offset)> process_room = [&](
      unordered_map<int16_t, pair<int16_t, int16_t>>& ret, int16_t room_id,
      int16_t x_offset, int16_t y_offset) {
    if (!remaining_room_ids.erase(room_id)) {
      return;
    }

    ret.emplace(room_id, make_pair(x_offset, y_offset));
    process_room(ret, room_id - 1, x_offset - 1, y_offset);
    process_room(ret, room_id + 1, x_offset + 1, y_offset);
    process_room(ret, room_id - 100, x_offset, y_offset - 1);
    process_room(ret, room_id + 100, x_offset, y_offset + 1);
  };

  // this function generates a placement map with nonnegative offsets that
  // contains the given room
  vector<unordered_map<int16_t, pair<int16_t, int16_t>>> ret;
  auto process_component = [&](int16_t start_room_id) {
    ret.emplace_back();
    auto& placement_map = ret.back();
    process_room(placement_map, start_room_id, 0, 0);
    if (placement_map.empty()) {
      ret.pop_back();
    } else {
      // make all offsets nonnegative
      int16_t delta_x = 0, delta_y = 0;
      for (const auto& it : placement_map) {
        if (it.second.first < delta_x) {
          delta_x = it.second.first;
        }
        if (it.second.second < delta_y) {
          delta_y = it.second.second;
        }
      }
      for (auto& it : placement_map) {
        it.second.first -= delta_x;
        it.second.second -= delta_y;
      }
    }
  };

  // start at room 1000 (for main level) and 10000 (for bonus level) and go
  // outward. these both seem to be hardcoded
  process_component(1000);
  process_component(10000);

  // if there are any rooms left over, process them individually
  while (!remaining_room_ids.empty()) {
    size_t starting_size = remaining_room_ids.size();
    process_component(*remaining_room_ids.begin());
    if (remaining_room_ids.size() >= starting_size) {
      throw logic_error("did not make progress generating room placement maps");
    }
  }

  return ret;
}



int main(int argc, char** argv) {
  if (argc < 2) {
    throw invalid_argument("no filename given");
  }
  const string filename = argv[1];
  const string out_prefix = (argc < 3) ? filename : argv[2];

  ResourceFile rf(filename + "/..namedfork/rsrc");
  const uint32_t room_type = 0x506C766C; // Plvl
  auto room_resource_ids = rf.all_resources_of_type(room_type);
  auto sprites_pict = rf.decode_PICT(130); // hardcoded ID for all worlds
  auto& sprites = sprites_pict.image;

  // assemble index for animated sprites
  unordered_map<int16_t, pair<shared_ptr<const Image>, size_t>> enemy_image_locations;
  {
    size_t next_type_id = 0;
    for (int16_t id = 1000; ; id++) {
      if (!rf.resource_exists(RESOURCE_TYPE_PICT, id)) {
        break;
      }
      auto pict = rf.decode_PICT(id);
      shared_ptr<const Image> img(new Image(pict.image));
      for (size_t z = 0; z < img->get_height(); z += 80) {
        enemy_image_locations.emplace(next_type_id, make_pair(img, z));
        next_type_id++;
      }
    }
  }

  // decode the default ppat (we'll use it if a room references a missing ppat,
  // which apparently happens quite a lot - it looks like the ppat id field used
  // to be the room id field and they just never updated it after implementing
  // the custom backgrounds feature)
  unordered_map<int16_t, const Image> background_ppat_cache;
  const Image* default_background_ppat = &background_ppat_cache.emplace(1000,
      rf.decode_ppat(1000).first).first->second;

  size_t component_number = 0;
  auto placement_maps = generate_room_placement_maps(room_resource_ids);
  for (const auto& placement_map : placement_maps) {
    // first figure out the width and height of this component
    uint16_t w_rooms = 0, h_rooms = 0;
    bool component_contains_start = false, component_contains_bonus_start = false;
    for (const auto& it : placement_map) {
      if (it.second.first >= w_rooms) {
        w_rooms = it.second.first + 1;
      }
      if (it.second.second >= h_rooms) {
        h_rooms = it.second.second + 1;
      }
      if (it.first == 1000) {
        component_contains_start = true;
      } else if (it.first == 10000) {
        component_contains_bonus_start = true;
      }
    }

    // then render the rooms
    Image result(20 * 32 * w_rooms, 20 * 20 * h_rooms);
    result.clear(0x20, 0x20, 0x20, 0xFF);
    for (auto it : placement_map) {
      int16_t room_id = it.first;
      size_t room_x = it.second.first;
      size_t room_y = it.second.second;
      size_t room_px = 20 * 32 * room_x;
      size_t room_py = 20 * 20 * room_y;

      string room_data = rf.get_resource_data(room_type, room_id);
      if (room_data.size() != sizeof(MonkeyShinesRoom)) {
        fprintf(stderr, "warning: room 0x%04hX is not the correct size (expected %zu bytes, got %zu bytes)\n",
            room_id, sizeof(MonkeyShinesRoom), room_data.size());
        result.fill_rect(room_px, room_py, 32 * 20, 20 * 20, 0xFF, 0x00, 0xFF, 0xFF);
        continue;
      }

      MonkeyShinesRoom* room = reinterpret_cast<MonkeyShinesRoom*>(
          const_cast<char*>(room_data.data()));
      room->byteswap();

      // render the appropriate ppat in the background of every room
      // we don't use Image::blit() here just in case the room dimensions aren't
      // a multiple of the ppat dimensions
      const Image* background_ppat = NULL;
      try {
        background_ppat = &background_ppat_cache.at(room->background_ppat_id);
      } catch (const out_of_range&) {
        try {
          background_ppat = &background_ppat_cache.emplace(room->background_ppat_id,
              rf.decode_ppat(room->background_ppat_id).first).first->second;
        } catch (const exception& e) {
          fprintf(stderr, "warning: room %hd uses ppat %hd but it can\'t be decoded (%s)\n",
              room_id, room->background_ppat_id, e.what());
          background_ppat = default_background_ppat;
        }
      }

      if (background_ppat) {
        for (size_t y = 0; y < 400; y++) {
          for (size_t x = 0; x < 640; x++) {
            uint64_t r, g, b;
            background_ppat->read_pixel(x % background_ppat->get_width(),
                y % background_ppat->get_height(), &r, &g, &b);
            result.write_pixel(room_px + x, room_py + y, r, g, b);
          }
        }

      } else {
        result.fill_rect(room_px, room_py, 640, 400, 0xFF, 0x00, 0xFF, 0xFF);
      }

      // render tiles. each tile is 20x20
      for (size_t y = 0; y < 20; y++) {
        for (size_t x = 0; x < 32; x++) {

          // looks like there are 21 rows of sprites in PICT 130, with 16 on each row

          uint16_t tile_id = room->tile_ids[x * 20 + y];
          if (tile_id == 0) {
            continue;
          }
          tile_id--;

          size_t tile_x = 0xFFFFFFFF;
          size_t tile_y = 0xFFFFFFFF;
          // 0x00-0x1F: walls
          // 0x20-0x4F: jump-through platforms
          // 0x50-0x8F: scenery block 1
          // 
          // 0xD0-0xEF: scenery block 2
          if (tile_id < 0x90) { // standard tiles
            tile_x = tile_id % 16;
            tile_y = tile_id / 16;
          } else if (tile_id < 0xA0) { // 2-frame animated tiles
            tile_x = tile_id & 0x0F;
            tile_y = 11;
          } else if (tile_id < 0xB0) { // rollers (usually)
            tile_x = tile_id & 0x0F;
            tile_y = 15;
          } else if (tile_id < 0xB2) { // collapsing floor
            tile_x = 0;
            tile_y = 17 + (tile_id & 1);
          } else if (tile_id < 0xC0) { // 2-frame animated tiles
            tile_x = tile_id & 0x0F;
            tile_y = 11;
          } else if (tile_id < 0xD0) { // 2-frame animated tiles
            tile_x = tile_id & 0x0F;
            tile_y = 13;
          } else if (tile_id < 0xF0) { // scenery block 2
            tile_x = tile_id & 0x0F;
            tile_y = (tile_id - 0x40) / 16;
          }
          // TODO: there may be more cases than the above; figure them out

          if (tile_x == 0xFFFFFFFF || tile_y == 0xFFFFFFFF) {
            result.fill_rect(room_px + x * 20, room_py + y * 20, 20, 20, 0xFF,
                0x00, 0xFF, 0xFF);
            fprintf(stderr, "warning: no known tile for %02hX (room %hd, x=%zu, y=%zu)\n",
                tile_id, room_id, x, y);
          } else {
            for (size_t py = 0; py < 20; py++) {
              for (size_t px = 0; px < 20; px++) {
                uint64_t r, g, b;
                sprites.read_pixel(tile_x * 20 + px, tile_y * 40 + py + 20,
                    &r, &g, &b);
                if (r && g && b) {
                  sprites.read_pixel(tile_x * 20 + px, tile_y * 40 + py,
                      &r, &g, &b);
                  result.write_pixel(room_px + x * 20 + px, room_py + y * 20 + py,
                      r, g, b, 0xFF);
                }
              }
            }
          }
          // result.draw_text(room_px + x * 20, room_py + y * 20, NULL, NULL,
          //     0x80, 0x80, 0x80, 0xFF, 0x00, 0x00, 0x00, 0x00, "%02hX", tile_id);
        }
      }

      // render enemies
      for (size_t z = 0; z < room->enemy_count; z++) {
        // it looks like the y coords are off by 80 pixels because of the HUD,
        // which renders at the top. hilarious?
        size_t enemy_px = room_px + room->enemies[z].x_pixels;
        size_t enemy_py = room_py + room->enemies[z].y_pixels - 80;
        try {
          const auto& image_loc = enemy_image_locations.at(room->enemies[z].type);
          const auto& enemy_pict = image_loc.first;
          size_t enemy_pict_py = image_loc.second;
          for (size_t py = 0; py < 40; py++) {
            for (size_t px = 0; px < 40; px++) {
              uint64_t r, g, b, mr, mg, mb, er, eg, eb;
              enemy_pict->read_pixel(px, enemy_pict_py + py, &r, &g, &b);
              enemy_pict->read_pixel(px, enemy_pict_py + py + 40, &mr, &mg, &mb);
              result.read_pixel(enemy_px + px, enemy_py + py, &er, &eg, &eb);
              result.write_pixel(enemy_px + px, enemy_py + py,
                  (r & mr) | (er & ~mr), (g & mg) | (eg & ~mg),
                  (b & mb) | (eb & ~mb), 0xFF);
            }
          }
        } catch (const out_of_range&) {
          result.fill_rect(enemy_px, enemy_px, 20, 20, 0xFF, 0x80, 0x00, 0xFF);
          result.draw_text(enemy_px, enemy_px, NULL, NULL, 0x00, 0x00, 0x00,
              0xFF, 0x00, 0x00, 0x00, 0x00, "%04hX", room->enemies[z].type);
        }

        // draw a bounding box to show where its range of motion is
        size_t x_min = room->enemies[z].x_speed ? room->enemies[z].x_min : room->enemies[z].x_pixels;
        size_t x_max = (room->enemies[z].x_speed ? room->enemies[z].x_max : room->enemies[z].x_pixels) + 39;
        size_t y_min = (room->enemies[z].y_speed ? room->enemies[z].y_min : room->enemies[z].y_pixels) - 80;
        size_t y_max = (room->enemies[z].y_speed ? room->enemies[z].y_max : room->enemies[z].y_pixels) + 39 - 80;
        result.draw_horizontal_line(room_px + x_min, room_px + x_max,
            room_py + y_min, 0, 0xFF, 0x80, 0x00);
        result.draw_horizontal_line(room_px + x_min, room_px + x_max,
            room_py + y_max, 0, 0xFF, 0x80, 0x00);
        result.draw_vertical_line(room_px + x_min, room_py + y_min,
            room_py + y_max, 0, 0xFF, 0x80, 0x00);
        result.draw_vertical_line(room_px + x_max, room_py + y_min,
            room_py + y_max, 0, 0xFF, 0x80, 0x00);

        // draw its initial velocity as a line from the center
        if (room->enemies[z].x_speed || room->enemies[z].y_speed) {
          result.fill_rect(enemy_px + 19, enemy_py + 19, 3, 3, 0xFF, 0x80, 0x00, 0xFF);
          result.draw_line(enemy_px + 20, enemy_py + 20,
              enemy_px + 20 + room->enemies[z].x_speed * 10,
              enemy_py + 20 + room->enemies[z].y_speed * 10, 0xFF, 0x80, 0x00, 0xFF);
        }
      }

      // annotate bonuses with ids
      for (size_t z = 0; z < room->bonus_count; z++) {
        const auto& bonus = room->bonuses[z];
        result.draw_text(room_px + bonus.x_pixels, room_py + bonus.y_pixels - 80, NULL,
            NULL, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, "%02hX", bonus.id);
      }

      // if this is a starting room, mark the player start location
      if (room_id == 1000 || room_id == 10000) {
        size_t x_min = room->player_start_x;
        size_t x_max = room->player_start_x + 39;
        size_t y_min = room->player_start_y - 80;
        size_t y_max = room->player_start_y + 39 - 80;
        result.draw_horizontal_line(room_px + x_min, room_px + x_max,
            room_py + y_min, 0, 0x00, 0xFF, 0x80);
        result.draw_horizontal_line(room_px + x_min, room_px + x_max,
            room_py + y_max, 0, 0x00, 0xFF, 0x80);
        result.draw_vertical_line(room_px + x_min, room_py + y_min,
            room_py + y_max, 0, 0x00, 0xFF, 0x80);
        result.draw_vertical_line(room_px + x_max, room_py + y_min,
            room_py + y_max, 0, 0x00, 0xFF, 0x80);
        result.draw_text(room_px + x_min + 2, room_py + y_min + 2, NULL, NULL, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0x00, 0x00, 0x80, "START");
      }

      result.draw_text(room_px + 2, room_py + 2, NULL, NULL, 0xFF, 0xFF, 0xFF, 0xFF,
          0x00, 0x00, 0x00, 0x80, "Room %hd", room_id);
    }

    string result_filename;
    if (component_contains_start && component_contains_bonus_start) {
      result_filename = out_prefix + "_world_and_bonus.bmp";
    } else if (component_contains_start) {
      result_filename = out_prefix + "_world.bmp";
    } else if (component_contains_bonus_start) {
      result_filename = out_prefix + "_bonus.bmp";
    } else {
      result_filename = string_printf("%s_%zu.bmp", out_prefix.c_str(),
          component_number);
      component_number++;
    }
    result.save(result_filename.c_str(), Image::ImageFormat::WindowsBitmap);
    fprintf(stderr, "... %s\n", result_filename.c_str());
  }

  return 0;
}
