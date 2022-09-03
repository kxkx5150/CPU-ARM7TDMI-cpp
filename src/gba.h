#ifndef _GBA_H_
#define _GBA_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>


class CPU;
class MEM;
class DMA;
class IO;
class SOUND;
class TIMER;
class VIDEO;

class GBA {
  public:
    CPU   *cpu   = nullptr;
    MEM   *mem   = nullptr;
    DMA   *dma   = nullptr;
    IO    *io    = nullptr;
    SOUND *sound = nullptr;
    TIMER *timer = nullptr;
    VIDEO *video = nullptr;

    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;

    int32_t  tex_pitch;
    uint8_t *bios;
    int64_t  cart_rom_size;
    uint32_t cart_rom_mask;
    uint8_t *rom;

    const int64_t max_rom_sz = 32 * 1024 * 1024;

  public:
    GBA();

    void     start();
    uint32_t to_pow2(uint32_t val);
    void     sdl_init();
    void     sdl_uninit();
    bool     open_rom(char *romname);
    int      init(char *argv[]);

    void run_frame();
};
#endif
