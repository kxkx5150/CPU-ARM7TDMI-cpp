#ifndef _SOUND_H_
#define _SOUND_H_
#include <stdbool.h>
#include <stdint.h>
#include "gba.h"

#define CPU_FREQ_HZ      16777216
#define SND_FREQUENCY    32768
#define SND_CHANNELS     2
#define SND_SAMPLES      512
#define SAMP_CYCLES      (CPU_FREQ_HZ / SND_FREQUENCY)
#define BUFF_SAMPLES     ((SND_SAMPLES)*16 * 2)
#define BUFF_SAMPLES_MSK ((BUFF_SAMPLES)-1)

typedef struct
{
    bool     phase;          // Square 1/2 only
    uint16_t lfsr;           // Noise only
    double   samples;        // All
    double   length_time;    // All
    double   sweep_time;     // Square 1 only
    double   env_time;       // All except Wave
} snd_ch_state_t;

class SOUND {
  public:
    GBA *gba = nullptr;

    int8_t  fifo_a[0x20];
    int8_t  fifo_b[0x20];
    uint8_t fifo_a_len;
    uint8_t fifo_b_len;

    int16_t  snd_buffer[BUFF_SAMPLES];
    uint32_t snd_cur_play  = 0;
    uint32_t snd_cur_write = 0x200;
    int8_t   fifo_a_samp;
    int8_t   fifo_b_samp;
    uint32_t snd_cycles = 0;

    uint8_t wave_position;
    uint8_t wave_samples;

    snd_ch_state_t snd_ch_state[4];

    double duty_lut[4]   = {0.125, 0.250, 0.500, 0.750};
    double duty_lut_i[4] = {0.875, 0.750, 0.500, 0.250};

    int32_t psg_vol_lut[8] = {0x000, 0x024, 0x049, 0x06d, 0x092, 0x0b6, 0x0db, 0x100};
    int32_t psg_rsh_lut[4] = {0xa, 0x9, 0x8, 0x7};

  public:
    SOUND(GBA *_gba);

    int8_t  square_sample(uint8_t ch);
    int8_t  wave_sample();
    int8_t  noise_sample();
    void    wave_reset();
    void    sound_buffer_wrap();
    void    sound_mix(void *data, uint8_t *stream, int32_t len);
    void    fifo_a_copy();
    void    fifo_b_copy();
    void    fifo_a_load();
    void    fifo_b_load();
    int16_t clip(int32_t value);
    void    sound_clock(uint32_t cycles);
};
// void wave_reset();
// void sound_buffer_wrap();
// void sound_mix(void *data, uint8_t *stream, int32_t len);
// void sound_clock(uint32_t cycles);
// void fifo_a_copy();
// void fifo_b_copy();
// void fifo_a_load();
// void fifo_b_load();

#endif
