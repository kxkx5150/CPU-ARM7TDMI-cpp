#include <stdint.h>
#include "io.h"
#include "sound.h"

#define PSG_MAX     0x7f
#define PSG_MIN     -0x80
#define SAMP_MAX    0x1ff
#define SAMP_MIN    -0x200
#define SAMPLE_TIME (1.0 / (SND_FREQUENCY))


SOUND::SOUND(GBA *_gba)
{
    gba = _gba;
}
int8_t SOUND::square_sample(uint8_t ch)
{
    if (!(gba->io->snd_psg_enb.w & (CH_SQR1 << ch)))
        return 0;
    uint8_t  sweep_time = (gba->io->sqr_ch[ch].sweep.w >> 4) & 0x7;
    uint8_t  duty       = (gba->io->sqr_ch[ch].tone.w >> 6) & 0x3;
    uint8_t  env_step   = (gba->io->sqr_ch[ch].tone.w >> 8) & 0x7;
    uint8_t  envelope   = (gba->io->sqr_ch[ch].tone.w >> 12) & 0xf;
    uint8_t  snd_len    = (gba->io->sqr_ch[ch].tone.w >> 0) & 0x3f;
    uint16_t freq_hz    = (gba->io->sqr_ch[ch].ctrl.w >> 0) & 0x7ff;

    double frequency         = 131072.0 / (2048 - freq_hz);
    double length            = (64 - snd_len) / 256.0;
    double sweep_interval    = 0.0078 * (sweep_time + 1);
    double envelope_interval = env_step / 64.0;
    double cycle_samples     = SND_FREQUENCY / frequency;

    if (gba->io->sqr_ch[ch].ctrl.w & CH_LEN) {
        snd_ch_state[ch].length_time += SAMPLE_TIME;
        if (snd_ch_state[ch].length_time >= length) {

            gba->io->snd_psg_enb.w &= ~(CH_SQR1 << ch);

            return 0;
        }
    }

    if (ch == 0) {
        snd_ch_state[0].sweep_time += SAMPLE_TIME;
        if (snd_ch_state[0].sweep_time >= sweep_interval) {
            snd_ch_state[0].sweep_time -= sweep_interval;

            uint8_t sweep_shift = gba->io->sqr_ch[0].sweep.w & 7;
            if (sweep_shift) {
                uint32_t disp = freq_hz >> sweep_shift;
                if (gba->io->sqr_ch[0].sweep.w & SWEEP_DEC)
                    freq_hz -= disp;
                else
                    freq_hz += disp;
                if (freq_hz < 0x7ff) {

                    gba->io->sqr_ch[0].ctrl.w &= ~0x7ff;
                    gba->io->sqr_ch[0].ctrl.w |= freq_hz;
                } else {

                    gba->io->snd_psg_enb.w &= ~CH_SQR1;
                }
            }
        }
    }

    if (env_step) {
        snd_ch_state[ch].env_time += SAMPLE_TIME;
        if (snd_ch_state[ch].env_time >= envelope_interval) {
            snd_ch_state[ch].env_time -= envelope_interval;
            if (gba->io->sqr_ch[ch].tone.w & ENV_INC) {
                if (envelope < 0xf)
                    envelope++;
            } else {
                if (envelope > 0x0)
                    envelope--;
            }
            gba->io->sqr_ch[ch].tone.w &= ~0xf000;
            gba->io->sqr_ch[ch].tone.w |= envelope << 12;
        }
    }

    snd_ch_state[ch].samples++;
    if (snd_ch_state[ch].phase) {

        double phase_change = cycle_samples * duty_lut[duty];
        if (snd_ch_state[ch].samples > phase_change) {
            snd_ch_state[ch].samples -= phase_change;
            snd_ch_state[ch].phase = false;
        }
    } else {

        double phase_change = cycle_samples * duty_lut_i[duty];
        if (snd_ch_state[ch].samples > phase_change) {
            snd_ch_state[ch].samples -= phase_change;
            snd_ch_state[ch].phase = true;
        }
    }
    return snd_ch_state[ch].phase ? (envelope / 15.0) * PSG_MAX : (envelope / 15.0) * PSG_MIN;
}
int8_t SOUND::wave_sample()
{
    if (!((gba->io->snd_psg_enb.w & CH_WAVE) && (gba->io->wave_ch.wave.w & WAVE_PLAY)))
        return 0;
    uint8_t  snd_len = (gba->io->wave_ch.volume.w >> 0) & 0xff;
    uint8_t  volume  = (gba->io->wave_ch.volume.w >> 13) & 0x7;
    uint16_t freq_hz = (gba->io->wave_ch.ctrl.w >> 0) & 0x7ff;

    double frequency     = 2097152.0 / (2048 - freq_hz);
    double length        = (256 - snd_len) / 256.0;
    double cycle_samples = SND_FREQUENCY / frequency;

    if (gba->io->wave_ch.ctrl.w & CH_LEN) {
        snd_ch_state[2].length_time += SAMPLE_TIME;
        if (snd_ch_state[2].length_time >= length) {

            gba->io->snd_psg_enb.w &= ~CH_WAVE;

            return 0;
        }
    }
    snd_ch_state[2].samples++;
    if (snd_ch_state[2].samples >= cycle_samples) {
        snd_ch_state[2].samples -= cycle_samples;
        if (--wave_samples)
            wave_position = (wave_position + 1) & 0x3f;
        else
            wave_reset();
    }
    int8_t samp = wave_position & 1 ? ((gba->io->wave_ram[(wave_position >> 1) & 0x1f] >> 0) & 0xf) - 8
                                    : ((gba->io->wave_ram[(wave_position >> 1) & 0x1f] >> 4) & 0xf) - 8;
    switch (volume) {
        case 0:
            samp = 0;
            break;
        case 1:
            samp >>= 0;
            break;
        case 2:
            samp >>= 1;
            break;
        case 3:
            samp >>= 2;
            break;
        default:
            samp = (samp >> 2) * 3;
            break;
    }
    return samp >= 0 ? (samp / 7.0) * PSG_MAX : (samp / -8.0) * PSG_MIN;
}
int8_t SOUND::noise_sample()
{
    if (!(gba->io->snd_psg_enb.w & CH_NOISE))
        return 0;
    uint8_t env_step = (gba->io->noise_ch.env.w >> 8) & 0x7;
    uint8_t envelope = (gba->io->noise_ch.env.w >> 12) & 0xf;
    uint8_t snd_len  = (gba->io->noise_ch.env.w >> 0) & 0x3f;
    uint8_t freq_div = (gba->io->noise_ch.ctrl.w >> 0) & 0x7;
    uint8_t freq_rsh = (gba->io->noise_ch.ctrl.w >> 4) & 0xf;

    double frequency         = freq_div ? (524288 / freq_div) >> (freq_rsh + 1) : (524288 * 2) >> (freq_rsh + 1);
    double length            = (64 - snd_len) / 256.0;
    double envelope_interval = env_step / 64.0;
    double cycle_samples     = SND_FREQUENCY / frequency;

    if (gba->io->noise_ch.ctrl.w & CH_LEN) {
        snd_ch_state[3].length_time += SAMPLE_TIME;
        if (snd_ch_state[3].length_time >= length) {

            gba->io->snd_psg_enb.w &= ~CH_NOISE;

            return 0;
        }
    }

    if (env_step) {
        snd_ch_state[3].env_time += SAMPLE_TIME;
        if (snd_ch_state[3].env_time >= envelope_interval) {
            snd_ch_state[3].env_time -= envelope_interval;
            if (gba->io->noise_ch.env.w & ENV_INC) {
                if (envelope < 0xf)
                    envelope++;
            } else {
                if (envelope > 0x0)
                    envelope--;
            }
            gba->io->noise_ch.env.w &= ~0xf000;
            gba->io->noise_ch.env.w |= envelope << 12;
        }
    }
    uint8_t carry = snd_ch_state[3].lfsr & 1;
    snd_ch_state[3].samples++;
    if (snd_ch_state[3].samples >= cycle_samples) {
        snd_ch_state[3].samples -= cycle_samples;
        snd_ch_state[3].lfsr >>= 1;
        uint8_t high = (snd_ch_state[3].lfsr & 1) ^ carry;
        if (gba->io->noise_ch.ctrl.w & NOISE_7)
            snd_ch_state[3].lfsr |= (high << 6);
        else
            snd_ch_state[3].lfsr |= (high << 14);
    }
    return carry ? (envelope / 15.0) * PSG_MAX : (envelope / 15.0) * PSG_MIN;
}
void SOUND::wave_reset()
{
    if (gba->io->wave_ch.wave.w & WAVE_64) {

        wave_position = 0;
        wave_samples  = 64;
    } else {

        wave_position = (gba->io->wave_ch.wave.w >> 1) & 0x20;
        wave_samples  = 32;
    }
}
void SOUND::sound_buffer_wrap()
{
    if ((snd_cur_play / BUFF_SAMPLES) == (snd_cur_write / BUFF_SAMPLES)) {
        snd_cur_play &= BUFF_SAMPLES_MSK;
        snd_cur_write &= BUFF_SAMPLES_MSK;
    }
}
void SOUND::sound_mix(void *data, uint8_t *stream, int32_t len)
{
    uint16_t i;
    for (i = 0; i < len; i += 4) {
        *(int16_t *)(stream + (i | 0)) = snd_buffer[snd_cur_play++ & BUFF_SAMPLES_MSK] << 6;
        *(int16_t *)(stream + (i | 2)) = snd_buffer[snd_cur_play++ & BUFF_SAMPLES_MSK] << 6;
    }

    snd_cur_play += ((int32_t)(snd_cur_write - snd_cur_play) >> 8) & ~1;
}
void SOUND::fifo_a_copy()
{
    if (fifo_a_len + 4 > 0x20)
        return;
    fifo_a[fifo_a_len++] = gba->io->snd_fifo_a_0;
    fifo_a[fifo_a_len++] = gba->io->snd_fifo_a_1;
    fifo_a[fifo_a_len++] = gba->io->snd_fifo_a_2;
    fifo_a[fifo_a_len++] = gba->io->snd_fifo_a_3;
}
void SOUND::fifo_b_copy()
{
    if (fifo_b_len + 4 > 0x20)
        return;
    fifo_b[fifo_b_len++] = gba->io->snd_fifo_b_0;
    fifo_b[fifo_b_len++] = gba->io->snd_fifo_b_1;
    fifo_b[fifo_b_len++] = gba->io->snd_fifo_b_2;
    fifo_b[fifo_b_len++] = gba->io->snd_fifo_b_3;
}
void SOUND::fifo_a_load()
{
    if (fifo_a_len) {
        fifo_a_samp = fifo_a[0];
        fifo_a_len--;
        uint8_t i;
        for (i = 0; i < fifo_a_len; i++) {
            fifo_a[i] = fifo_a[i + 1];
        }
    }
}
void SOUND::fifo_b_load()
{
    if (fifo_b_len) {
        fifo_b_samp = fifo_b[0];
        fifo_b_len--;
        uint8_t i;
        for (i = 0; i < fifo_b_len; i++) {
            fifo_b[i] = fifo_b[i + 1];
        }
    }
}
int16_t SOUND::clip(int32_t value)
{
    if (value > SAMP_MAX)
        value = SAMP_MAX;
    if (value < SAMP_MIN)
        value = SAMP_MIN;
    return value;
}
void SOUND::sound_clock(uint32_t cycles)
{
    snd_cycles += cycles;
    int16_t samp_pcm_l = 0;
    int16_t samp_pcm_r = 0;
    int16_t samp_ch4   = (fifo_a_samp << 1) >> !(gba->io->snd_pcm_vol.w & 4);
    int16_t samp_ch5   = (fifo_b_samp << 1) >> !(gba->io->snd_pcm_vol.w & 8);
    if (gba->io->snd_pcm_vol.w & CH_DMAA_L)
        samp_pcm_l = clip(samp_pcm_l + samp_ch4);
    if (gba->io->snd_pcm_vol.w & CH_DMAB_L)
        samp_pcm_l = clip(samp_pcm_l + samp_ch5);
    if (gba->io->snd_pcm_vol.w & CH_DMAA_R)
        samp_pcm_r = clip(samp_pcm_r + samp_ch4);
    if (gba->io->snd_pcm_vol.w & CH_DMAB_R)
        samp_pcm_r = clip(samp_pcm_r + samp_ch5);
    while (snd_cycles >= SAMP_CYCLES) {
        int16_t samp_ch0   = square_sample(0);
        int16_t samp_ch1   = square_sample(1);
        int16_t samp_ch2   = wave_sample();
        int16_t samp_ch3   = noise_sample();
        int32_t samp_psg_l = 0;
        int32_t samp_psg_r = 0;
        if (gba->io->snd_psg_vol.w & CH_SQR1_L)
            samp_psg_l = clip(samp_psg_l + samp_ch0);
        if (gba->io->snd_psg_vol.w & CH_SQR2_L)
            samp_psg_l = clip(samp_psg_l + samp_ch1);
        if (gba->io->snd_psg_vol.w & CH_WAVE_L)
            samp_psg_l = clip(samp_psg_l + samp_ch2);
        if (gba->io->snd_psg_vol.w & CH_NOISE_L)
            samp_psg_l = clip(samp_psg_l + samp_ch3);
        if (gba->io->snd_psg_vol.w & CH_SQR1_R)
            samp_psg_r = clip(samp_psg_r + samp_ch0);
        if (gba->io->snd_psg_vol.w & CH_SQR2_R)
            samp_psg_r = clip(samp_psg_r + samp_ch1);
        if (gba->io->snd_psg_vol.w & CH_WAVE_R)
            samp_psg_r = clip(samp_psg_r + samp_ch2);
        if (gba->io->snd_psg_vol.w & CH_NOISE_R)
            samp_psg_r = clip(samp_psg_r + samp_ch3);
        samp_psg_l *= psg_vol_lut[(gba->io->snd_psg_vol.w >> 4) & 7];
        samp_psg_r *= psg_vol_lut[(gba->io->snd_psg_vol.w >> 0) & 7];
        samp_psg_l >>= psg_rsh_lut[(gba->io->snd_pcm_vol.w >> 0) & 3];
        samp_psg_r >>= psg_rsh_lut[(gba->io->snd_pcm_vol.w >> 0) & 3];
        snd_buffer[snd_cur_write++ & BUFF_SAMPLES_MSK] = clip(samp_psg_l + samp_pcm_l);
        snd_buffer[snd_cur_write++ & BUFF_SAMPLES_MSK] = clip(samp_psg_r + samp_pcm_r);
        snd_cycles -= SAMP_CYCLES;
    }
}