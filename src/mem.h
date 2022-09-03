#ifndef _CPUMEM_H_
#define _CPUMEM_H_

#include <stdint.h>
#include "gba.h"

typedef enum
{
    NON_SEQ,
    SEQUENTIAL
} access_type_e;

typedef enum
{
    IDLE,
    ERASE,
    WRITE,
    BANK_SWITCH
} flash_mode_e;


class MEM {
  public:
    GBA *gba = nullptr;

    uint8_t *wram;
    uint8_t *iwram;
    uint8_t *pram;
    uint8_t *vram;
    uint8_t *oam;
    uint8_t *eeprom;
    uint8_t *sram;
    uint8_t *flash;
    uint32_t bios_op;
    uint16_t eeprom_idx;
    uint32_t flash_bank = 0;

    flash_mode_e flash_mode = IDLE;

    bool flash_id_mode = false;
    bool flash_used    = false;
    bool eeprom_used   = false;
    bool eeprom_read   = false;

    uint32_t eeprom_addr      = 0;
    uint32_t eeprom_addr_read = 0;

    uint8_t  eeprom_buff[0x100];
    uint32_t palette[0x200];

    const uint8_t bus_size_lut[16] = {4, 4, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 1, 1};

  public:
    MEM(GBA *_gba);

    void arm_access(uint32_t address, access_type_e at);
    void arm_access_bus(uint32_t address, uint8_t size, access_type_e at);

    uint8_t bios_read(uint32_t address);
    uint8_t wram_read(uint32_t address);
    uint8_t iwram_read(uint32_t address);
    uint8_t pram_read(uint32_t address);
    uint8_t vram_read(uint32_t address);
    uint8_t oam_read(uint32_t address);
    uint8_t rom_read(uint32_t address);
    uint8_t rom_eep_read(uint32_t address, uint8_t offset);
    uint8_t flash_read(uint32_t address);

    uint8_t  arm_read_(uint32_t address, uint8_t offset);
    uint8_t  arm_readb(uint32_t address);
    uint32_t arm_readh(uint32_t address);
    uint32_t arm_read(uint32_t address);
    uint8_t  arm_readb_n(uint32_t address);
    uint32_t arm_readh_n(uint32_t address);
    uint32_t arm_read_n(uint32_t address);
    uint8_t  arm_readb_s(uint32_t address);
    uint32_t arm_readh_s(uint32_t address);
    uint32_t arm_read_s(uint32_t address);

    void wram_write(uint32_t address, uint8_t value);
    void iwram_write(uint32_t address, uint8_t value);
    void pram_write(uint32_t address, uint8_t value);
    void vram_write(uint32_t address, uint8_t value);
    void oam_write(uint32_t address, uint8_t value);
    void eeprom_write(uint32_t address, uint8_t offset, uint8_t value);
    void flash_write(uint32_t address, uint8_t value);

    void arm_write_(uint32_t address, uint8_t offset, uint8_t value);
    void arm_writeb(uint32_t address, uint8_t value);
    void arm_writeh(uint32_t address, uint16_t value);
    void arm_write(uint32_t address, uint32_t value);
    void arm_writeb_n(uint32_t address, uint8_t value);
    void arm_writeh_n(uint32_t address, uint16_t value);
    void arm_write_n(uint32_t address, uint32_t value);
    void arm_writeb_s(uint32_t address, uint8_t value);
    void arm_writeh_s(uint32_t address, uint16_t value);
    void arm_write_s(uint32_t address, uint32_t value);
};
#endif
