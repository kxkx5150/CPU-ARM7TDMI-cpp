#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <cstdint>
#include "gba.h"

class VIDEO {
  public:
    GBA *gba = nullptr;

    void *screen;

    const uint8_t bg_enb[3]       = {0xf, 0x7, 0xc};
    const uint8_t x_tiles_lut[16] = {1, 2, 4, 8, 2, 4, 4, 8, 1, 1, 2, 4, 0, 0, 0, 0};
    const uint8_t y_tiles_lut[16] = {1, 2, 4, 8, 1, 1, 2, 4, 2, 4, 4, 8, 0, 0, 0, 0};

  public:
    VIDEO(GBA *_gba);

    void render_obj(uint8_t prio);
    void render_bg();
    void render_line();
    void vblank_start();
    void hblank_start();
    void vcount_match();
};

#endif
