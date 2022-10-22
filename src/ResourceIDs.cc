#include "ResourceIDs.hh"

void fprint(FILE* file, const ResourceIDs& res_ids, bool new_line) {
  auto  print_id = [&, first = true](const char* prefix, int16_t res_id) mutable {
    fprintf(file, "%s%d", prefix ? prefix : first ? "" : ", ", res_id);
    first = false;
  };
  
  for (int res_id = MIN_RES_ID, start = INT_MAX; res_id <= MAX_RES_ID; ++res_id) {
    if (res_ids[res_id]) {
      if (start != INT_MAX) {
        // End of range?
        if (res_id == MAX_RES_ID || !res_ids[res_id + 1]) {
          print_id("..", res_id);
        }
      } else {
        // Possibly start of range
        start = res_id;
        print_id(nullptr, res_id);
      }
    } else {
      start = INT_MAX;
    }
  }
  if (new_line) {
    fputc('\n', file);
  }
}
