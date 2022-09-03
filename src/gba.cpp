#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "gba.h"
#include "arm.h"
#include "dma.h"
#include "mem.h"
#include "io.h"
#include "timer.h"
#include "video.h"
#include "sound.h"
#include "../BIOS/bios.h"

#define LINES_TOTAL    228
#define LINES_VISIBLE  160
#define CYC_LINE_TOTAL 1232
#define CYC_LINE_HBLK0 1006
#define CYC_LINE_HBLK1 (CYC_LINE_TOTAL - CYC_LINE_HBLK0)

GBA *g_gba = nullptr;
GBA::GBA()
{
    g_gba = this;
    cpu   = new CPU(this);
    mem   = new MEM(this);
    dma   = new DMA(this);
    io    = new IO(this);
    sound = new SOUND(this);
    timer = new TIMER(this);
    video = new VIDEO(this);
}
uint32_t GBA::to_pow2(uint32_t val)
{
    val--;
    val |= (val >> 1);
    val |= (val >> 2);
    val |= (val >> 4);
    val |= (val >> 8);
    val |= (val >> 16);
    return val + 1;
}
void sound_cb(void *data, uint8_t *stream, int32_t len)
{
    g_gba->sound->sound_mix(data, stream, len);
}
void GBA::sdl_init()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    window             = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 320, 0);
    renderer           = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture            = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, 240, 160);
    tex_pitch          = 240 * 4;
    SDL_AudioSpec spec = {.freq     = SND_FREQUENCY,    // 32KHz
                          .format   = AUDIO_S16SYS,     // Signed 16 bits System endiannes
                          .channels = SND_CHANNELS,     // Stereo
                          .samples  = SND_SAMPLES,      // 16ms
                          .callback = sound_cb,
                          .userdata = NULL};
    SDL_OpenAudio(&spec, NULL);
    SDL_PauseAudio(0);
}
void GBA::sdl_uninit()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudio();
    SDL_Quit();
}
bool GBA::open_rom(char *romname)
{
    FILE *image;
    image = fopen(romname, "rb");
    if (image == NULL) {
        printf("Error: ROM file couldn't be opened.\n");
        return false;
    }
    fseek(image, 0, SEEK_END);

    cart_rom_size = ftell(image);
    cart_rom_mask = to_pow2(cart_rom_size) - 1;
    if (cart_rom_size > max_rom_sz)
        cart_rom_size = max_rom_sz;

    fseek(image, 0, SEEK_SET);
    fread(rom, cart_rom_size, 1, image);
    fclose(image);
    return true;
}
void GBA::run_frame()
{
    io->disp_stat.w &= ~VBLK_FLAG;
    SDL_LockTexture(texture, NULL, &video->screen, &tex_pitch);

    for (io->v_count.w = 0; io->v_count.w < LINES_TOTAL; io->v_count.w++) {
        io->disp_stat.w &= ~(HBLK_FLAG | VCNT_FLAG);

        if (io->v_count.w == io->disp_stat.b.b1)
            video->vcount_match();

        if (io->v_count.w == LINES_VISIBLE) {
            io->bg_refxi[2].w = io->bg_refxe[2].w;
            io->bg_refyi[2].w = io->bg_refye[2].w;
            io->bg_refxi[3].w = io->bg_refxe[3].w;
            io->bg_refyi[3].w = io->bg_refye[3].w;
            video->vblank_start();
            dma->dma_transfer(VBLANK);
        }

        cpu->arm_exec(CYC_LINE_HBLK0);

        if (io->v_count.w < LINES_VISIBLE) {
            video->render_line();
            dma->dma_transfer(HBLANK);
        }

        video->hblank_start();
        cpu->arm_exec(CYC_LINE_HBLK1);
        sound->sound_clock(CYC_LINE_TOTAL);
    }

    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    sound->sound_buffer_wrap();
}
void GBA::start()
{
    bool run = true;
    while (run) {
        run_frame();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            io->key_input.w &= ~BTN_U;
                            break;
                        case SDLK_DOWN:
                            io->key_input.w &= ~BTN_D;
                            break;
                        case SDLK_LEFT:
                            io->key_input.w &= ~BTN_L;
                            break;
                        case SDLK_RIGHT:
                            io->key_input.w &= ~BTN_R;
                            break;
                        case SDLK_a:
                            io->key_input.w &= ~BTN_A;
                            break;
                        case SDLK_s:
                            io->key_input.w &= ~BTN_B;
                            break;
                        case SDLK_q:
                            io->key_input.w &= ~BTN_LT;
                            break;
                        case SDLK_w:
                            io->key_input.w &= ~BTN_RT;
                            break;
                        case SDLK_TAB:
                            io->key_input.w &= ~BTN_SEL;
                            break;
                        case SDLK_RETURN:
                            io->key_input.w &= ~BTN_STA;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            io->key_input.w |= BTN_U;
                            break;
                        case SDLK_DOWN:
                            io->key_input.w |= BTN_D;
                            break;
                        case SDLK_LEFT:
                            io->key_input.w |= BTN_L;
                            break;
                        case SDLK_RIGHT:
                            io->key_input.w |= BTN_R;
                            break;
                        case SDLK_a:
                            io->key_input.w |= BTN_A;
                            break;
                        case SDLK_s:
                            io->key_input.w |= BTN_B;
                            break;
                        case SDLK_q:
                            io->key_input.w |= BTN_LT;
                            break;
                        case SDLK_w:
                            io->key_input.w |= BTN_RT;
                            break;
                        case SDLK_TAB:
                            io->key_input.w |= BTN_SEL;
                            break;
                        case SDLK_RETURN:
                            io->key_input.w |= BTN_STA;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_QUIT:
                    run = false;
                    break;
            }
        }
    }
}
int GBA::init(char *argv[])
{
    cpu->arm_init();
    memcpy(bios, bios_bin, sizeof(bios_bin));
    if (!open_rom(argv[1]))
        return 0;

    sdl_init();
    cpu->arm_reset();

    start();

    sdl_uninit();
    cpu->arm_uninit();
    return 0;
}