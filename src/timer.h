#ifndef _TIMER_H_
#define _TIMER_H_
#include <stdint.h>
#include "gba.h"

#define TMR_CASCADE (1 << 2)
#define TMR_IRQ     (1 << 6)
#define TMR_ENB     (1 << 7)


class TIMER {
  public:
    GBA *gba = nullptr;

    const uint8_t pscale_shift_lut[4] = {0, 6, 8, 10};

    uint32_t tmr_icnt[4];
    uint8_t  tmr_enb;

  public:
    TIMER(GBA *_gba);

    void timers_clock(uint32_t cycles);
};

#endif
