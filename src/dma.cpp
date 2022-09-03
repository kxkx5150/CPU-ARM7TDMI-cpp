#include "arm.h"
#include "mem.h"
#include "dma.h"
#include "io.h"
#include "sound.h"


DMA::DMA(GBA *_gba)
{
    gba = _gba;
}
void DMA::dma_transfer(dma_timing_e timing)
{
    uint8_t ch;
    for (ch = 0; ch < 4; ch++) {
        if (!(gba->io->dma_ch[ch].ctrl.w & DMA_ENB) || ((gba->io->dma_ch[ch].ctrl.w >> 12) & 3) != timing)
            continue;
        if (ch == 3)
            gba->mem->eeprom_idx = 0;
        int8_t unit_size  = (gba->io->dma_ch[ch].ctrl.w & DMA_32) ? 4 : 2;
        bool   dst_reload = false;
        int8_t dst_inc    = 0;
        int8_t src_inc    = 0;
        switch ((gba->io->dma_ch[ch].ctrl.w >> 5) & 3) {
            case 0:
                dst_inc = unit_size;
                break;
            case 1:
                dst_inc = -unit_size;
                break;
            case 3:
                dst_inc    = unit_size;
                dst_reload = true;
                break;
        }
        switch ((gba->io->dma_ch[ch].ctrl.w >> 7) & 3) {
            case 0:
                src_inc = unit_size;
                break;
            case 1:
                src_inc = -unit_size;
                break;
        }
        while (dma_count[ch]--) {
            if (gba->io->dma_ch[ch].ctrl.w & DMA_32)
                gba->mem->arm_write(dma_dst_addr[ch], gba->mem->arm_read(dma_src_addr[ch]));
            else
                gba->mem->arm_writeh(dma_dst_addr[ch], gba->mem->arm_readh(dma_src_addr[ch]));
            dma_dst_addr[ch] += dst_inc;
            dma_src_addr[ch] += src_inc;
        }
        if (gba->io->dma_ch[ch].ctrl.w & DMA_IRQ)
            gba->io->trigger_irq(DMA0_FLAG << ch);
        if (gba->io->dma_ch[ch].ctrl.w & DMA_REP) {
            dma_count[ch] = gba->io->dma_ch[ch].count.w;
            if (dst_reload) {
                dma_dst_addr[ch] = gba->io->dma_ch[ch].dst.w;
            }
            continue;
        }
        gba->io->dma_ch[ch].ctrl.w &= ~DMA_ENB;
    }
}
void DMA::dma_transfer_fifo(uint8_t ch)
{
    if (!(gba->io->dma_ch[ch].ctrl.w & DMA_ENB) || ((gba->io->dma_ch[ch].ctrl.w >> 12) & 3) != SPECIAL)
        return;
    uint8_t i;
    for (i = 0; i < 4; i++) {
        gba->mem->arm_write(dma_dst_addr[ch], gba->mem->arm_read(dma_src_addr[ch]));
        if (ch == 1)
            gba->sound->fifo_a_copy();
        else
            gba->sound->fifo_b_copy();
        switch ((gba->io->dma_ch[ch].ctrl.w >> 7) & 3) {
            case 0:
                dma_src_addr[ch] += 4;
                break;
            case 1:
                dma_src_addr[ch] -= 4;
                break;
        }
    }
    if (gba->io->dma_ch[ch].ctrl.w & DMA_IRQ)
        gba->io->trigger_irq(DMA0_FLAG << ch);
}