#include "arm.h"
#include "dma.h"
#include "io.h"
#include "sound.h"
#include "timer.h"


TIMER::TIMER(GBA *_gba)
{
    gba = _gba;
}
void TIMER::timers_clock(uint32_t cycles)
{
    uint8_t idx;
    bool    overflow = false;
    for (idx = 0; idx < 4; idx++) {
        if (!(gba->io->tmr[idx].ctrl.w & TMR_ENB)) {
            overflow = false;
            continue;
        }
        if (gba->io->tmr[idx].ctrl.w & TMR_CASCADE) {
            if (overflow)
                gba->io->tmr[idx].count.w++;
        } else {
            uint8_t  shift = pscale_shift_lut[gba->io->tmr[idx].ctrl.w & 3];
            uint32_t inc   = (tmr_icnt[idx] += cycles) >> shift;
            gba->io->tmr[idx].count.w += inc;
            tmr_icnt[idx] -= inc << shift;
        }
        if ((overflow = (gba->io->tmr[idx].count.w > 0xffff))) {
            gba->io->tmr[idx].count.w = gba->io->tmr[idx].reload.w + (gba->io->tmr[idx].count.w - 0x10000);
            if (((gba->io->snd_pcm_vol.w >> 10) & 1) == idx) {
                gba->sound->fifo_a_load();
                if (gba->sound->fifo_a_len <= 0x10)
                    gba->dma->dma_transfer_fifo(1);
            }
            if (((gba->io->snd_pcm_vol.w >> 14) & 1) == idx) {
                gba->sound->fifo_b_load();
                if (gba->sound->fifo_b_len <= 0x10)
                    gba->dma->dma_transfer_fifo(2);
            }
        }
        if ((gba->io->tmr[idx].ctrl.w & TMR_IRQ) && overflow)
            gba->io->trigger_irq(TMR0_FLAG << idx);
    }
}