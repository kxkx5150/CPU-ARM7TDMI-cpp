#ifndef _DMA_H_
#define _DMA_H_

#include <stdint.h>
#include "gba.h"

#define DMA_REP (1 << 9)
#define DMA_32  (1 << 10)
#define DMA_IRQ (1 << 14)
#define DMA_ENB (1 << 15)

typedef enum
{
    IMMEDIATELY = 0,
    VBLANK      = 1,
    HBLANK      = 2,
    SPECIAL     = 3
} dma_timing_e;


class DMA {
  public:
    GBA *gba = nullptr;

    uint32_t dma_src_addr[4];
    uint32_t dma_dst_addr[4];
    uint32_t dma_count[4];

  public:
    DMA(GBA *_gba);

    void dma_transfer(dma_timing_e timing);
    void dma_transfer_fifo(uint8_t ch);
};
#endif
