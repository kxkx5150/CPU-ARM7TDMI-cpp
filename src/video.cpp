#include "arm.h"
#include "mem.h"
#include "dma.h"
#include "io.h"
#include <SDL2/SDL.h>
#include "sound.h"
#include "video.h"



VIDEO::VIDEO(GBA *_gba)
{
    gba = _gba;
}
void VIDEO::render_obj(uint8_t prio)
{
    if (!(gba->io->disp_cnt.w & OBJ_ENB))
        return;
    uint8_t  obj_index;
    uint32_t offset    = 0x3f8;
    uint32_t surf_addr = gba->io->v_count.w * 240 * 4;
    for (obj_index = 0; obj_index < 128; obj_index++) {
        uint16_t attr0 = gba->mem->oam[offset + 0] | (gba->mem->oam[offset + 1] << 8);
        uint16_t attr1 = gba->mem->oam[offset + 2] | (gba->mem->oam[offset + 3] << 8);
        uint16_t attr2 = gba->mem->oam[offset + 4] | (gba->mem->oam[offset + 5] << 8);
        offset -= 8;
        int16_t obj_y    = (attr0 >> 0) & 0xff;
        bool    affine   = (attr0 >> 8) & 0x1;
        bool    dbl_size = (attr0 >> 9) & 0x1;
        bool    hidden   = (attr0 >> 9) & 0x1;
        uint8_t obj_shp  = (attr0 >> 14) & 0x3;
        uint8_t affine_p = (attr1 >> 9) & 0x1f;
        uint8_t obj_size = (attr1 >> 14) & 0x3;
        uint8_t chr_prio = (attr2 >> 10) & 0x3;
        if (chr_prio != prio || (!affine && hidden))
            continue;
        int16_t pa, pb, pc, pd;
        pa = pd = 0x100;    // 1.0
        pb = pc = 0x000;    // 0.0
        if (affine) {
            uint32_t p_base = affine_p * 32;
            pa              = gba->mem->oam[p_base + 0x06] | (gba->mem->oam[p_base + 0x07] << 8);
            pb              = gba->mem->oam[p_base + 0x0e] | (gba->mem->oam[p_base + 0x0f] << 8);
            pc              = gba->mem->oam[p_base + 0x16] | (gba->mem->oam[p_base + 0x17] << 8);
            pd              = gba->mem->oam[p_base + 0x1e] | (gba->mem->oam[p_base + 0x1f] << 8);
        }
        uint8_t lut_idx = obj_size | (obj_shp << 2);
        uint8_t x_tiles = x_tiles_lut[lut_idx];
        uint8_t y_tiles = y_tiles_lut[lut_idx];
        int32_t rcx     = x_tiles * 4;
        int32_t rcy     = y_tiles * 4;
        if (affine && dbl_size) {
            rcx *= 2;
            rcy *= 2;
        }
        if (obj_y + rcy * 2 > 0xff)
            obj_y -= 0x100;
        if (obj_y <= (int32_t)gba->io->v_count.w && (obj_y + rcy * 2 > gba->io->v_count.w)) {
            uint8_t  obj_mode = (attr0 >> 10) & 0x3;
            bool     mosaic   = (attr0 >> 12) & 0x1;
            bool     is_256   = (attr0 >> 13) & 0x1;
            int16_t  obj_x    = (attr1 >> 0) & 0x1ff;
            bool     flip_x   = (attr1 >> 12) & 0x1;
            bool     flip_y   = (attr1 >> 13) & 0x1;
            uint16_t chr_numb = (attr2 >> 0) & 0x3ff;
            uint8_t  chr_pal  = (attr2 >> 12) & 0xf;
            uint32_t chr_base = 0x10000 | chr_numb * 32;
            obj_x <<= 7;
            obj_x >>= 7;
            int32_t x, y = gba->io->v_count.w - obj_y;
            if (!affine && flip_y)
                y ^= (y_tiles * 8) - 1;
            uint8_t tsz = is_256 ? 64 : 32;    // Tile block size (in bytes, = (8 * 8 * bpp) / 8)
            uint8_t lsz = is_256 ? 8 : 4;      // Pixel line row size (in bytes)
            int32_t ox  = pa * -rcx + pb * (y - rcy) + (x_tiles << 10);
            int32_t oy  = pc * -rcx + pd * (y - rcy) + (y_tiles << 10);
            if (!affine && flip_x) {
                ox = (x_tiles * 8 - 1) << 8;
                pa = -0x100;
            }
            uint32_t tys     = (gba->io->disp_cnt.w & MAP_1D_FLAG) ? x_tiles * tsz : 1024;    // Tile row stride
            uint32_t address = surf_addr + obj_x * 4;
            for (x = 0; x < rcx * 2; x++, ox += pa, oy += pc, address += 4) {
                if (obj_x + x < 0)
                    continue;
                if (obj_x + x >= 240)
                    break;
                uint32_t vram_addr;
                uint32_t pal_idx;
                uint16_t tile_x = ox >> 11;
                uint16_t tile_y = oy >> 11;
                if (ox < 0 || tile_x >= x_tiles)
                    continue;
                if (oy < 0 || tile_y >= y_tiles)
                    continue;
                uint16_t chr_x    = (ox >> 8) & 7;
                uint16_t chr_y    = (oy >> 8) & 7;
                uint32_t chr_addr = chr_base + tile_y * tys + chr_y * lsz;
                if (is_256) {
                    vram_addr = chr_addr + tile_x * 64 + chr_x;
                    pal_idx   = gba->mem->vram[vram_addr];
                } else {
                    vram_addr = chr_addr + tile_x * 32 + (chr_x >> 1);
                    pal_idx   = (gba->mem->vram[vram_addr] >> (chr_x & 1) * 4) & 0xf;
                }
                uint32_t pal_addr = 0x100 | pal_idx | (!is_256 ? chr_pal * 16 : 0);
                if (pal_idx)
                    *(uint32_t *)((uint8_t *)screen + address) = gba->mem->palette[pal_addr];
            }
        }
    }
}
void VIDEO::render_bg()
{
    uint8_t  mode      = gba->io->disp_cnt.w & 7;
    uint32_t surf_addr = gba->io->v_count.w * 240 * 4;
    switch (mode) {
        case 0:
        case 1:
        case 2: {
            uint8_t enb = (gba->io->disp_cnt.w >> 8) & bg_enb[mode];
            int8_t  prio, bg_idx;
            for (prio = 3; prio >= 0; prio--) {
                for (bg_idx = 3; bg_idx >= 0; bg_idx--) {
                    if (!(enb & (1 << bg_idx)))
                        continue;
                    if ((gba->io->bg[bg_idx].ctrl.w & 3) != prio)
                        continue;
                    uint32_t chr_base  = ((gba->io->bg[bg_idx].ctrl.w >> 2) & 0x3) << 14;
                    bool     is_256    = (gba->io->bg[bg_idx].ctrl.w >> 7) & 0x1;
                    uint16_t scrn_base = ((gba->io->bg[bg_idx].ctrl.w >> 8) & 0x1f) << 11;
                    bool     aff_wrap  = (gba->io->bg[bg_idx].ctrl.w >> 13) & 0x1;
                    uint16_t scrn_size = (gba->io->bg[bg_idx].ctrl.w >> 14);
                    bool     affine    = mode == 2 || (mode == 1 && bg_idx == 2);
                    uint32_t address   = surf_addr;
                    if (affine) {
                        int16_t pa = gba->io->bg_pa[bg_idx].w;
                        int16_t pb = gba->io->bg_pb[bg_idx].w;
                        int16_t pc = gba->io->bg_pc[bg_idx].w;
                        int16_t pd = gba->io->bg_pd[bg_idx].w;
                        int32_t ox = ((int32_t)gba->io->bg_refxi[bg_idx].w << 4) >> 4;
                        int32_t oy = ((int32_t)gba->io->bg_refyi[bg_idx].w << 4) >> 4;
                        gba->io->bg_refxi[bg_idx].w += pb;
                        gba->io->bg_refyi[bg_idx].w += pd;
                        uint8_t tms  = 16 << scrn_size;
                        uint8_t tmsk = tms - 1;
                        uint8_t x;
                        for (x = 0; x < 240; x++, ox += pa, oy += pc, address += 4) {
                            int16_t tmx = ox >> 11;
                            int16_t tmy = oy >> 11;
                            if (aff_wrap) {
                                tmx &= tmsk;
                                tmy &= tmsk;
                            } else {
                                if (tmx < 0 || tmx >= tms)
                                    continue;
                                if (tmy < 0 || tmy >= tms)
                                    continue;
                            }
                            uint16_t chr_x     = (ox >> 8) & 7;
                            uint16_t chr_y     = (oy >> 8) & 7;
                            uint32_t map_addr  = scrn_base + tmy * tms + tmx;
                            uint32_t vram_addr = chr_base + gba->mem->vram[map_addr] * 64 + chr_y * 8 + chr_x;
                            uint16_t pal_idx   = gba->mem->vram[vram_addr];
                            if (pal_idx)
                                *(uint32_t *)((uint8_t *)screen + address) = gba->mem->palette[pal_idx];
                        }
                    } else {
                        uint16_t oy     = gba->io->v_count.w + gba->io->bg[bg_idx].yofs.w;
                        uint16_t tmy    = oy >> 3;
                        uint16_t scrn_y = (tmy >> 5) & 1;
                        uint8_t  x;
                        for (x = 0; x < 240; x++) {
                            uint16_t ox     = x + gba->io->bg[bg_idx].xofs.w;
                            uint16_t tmx    = ox >> 3;
                            uint16_t scrn_x = (tmx >> 5) & 1;
                            uint16_t chr_x  = ox & 7;
                            uint16_t chr_y  = oy & 7;
                            uint16_t pal_idx;
                            uint16_t pal_base = 0;
                            uint32_t map_addr = scrn_base + (tmy & 0x1f) * 32 * 2 + (tmx & 0x1f) * 2;
                            switch (scrn_size) {
                                case 1:
                                    map_addr += scrn_x * 2048;
                                    break;
                                case 2:
                                    map_addr += scrn_y * 2048;
                                    break;
                                case 3:
                                    map_addr += scrn_x * 2048 + scrn_y * 4096;
                                    break;
                            }
                            uint16_t tile     = gba->mem->vram[map_addr + 0] | (gba->mem->vram[map_addr + 1] << 8);
                            uint16_t chr_numb = (tile >> 0) & 0x3ff;
                            bool     flip_x   = (tile >> 10) & 0x1;
                            bool     flip_y   = (tile >> 11) & 0x1;
                            uint8_t  chr_pal  = (tile >> 12) & 0xf;
                            if (!is_256)
                                pal_base = chr_pal * 16;
                            if (flip_x)
                                chr_x ^= 7;
                            if (flip_y)
                                chr_y ^= 7;
                            uint32_t vram_addr;
                            if (is_256) {
                                vram_addr = chr_base + chr_numb * 64 + chr_y * 8 + chr_x;
                                pal_idx   = gba->mem->vram[vram_addr];
                            } else {
                                vram_addr = chr_base + chr_numb * 32 + chr_y * 4 + (chr_x >> 1);
                                pal_idx   = (gba->mem->vram[vram_addr] >> (chr_x & 1) * 4) & 0xf;
                            }
                            uint32_t pal_addr = pal_idx | pal_base;
                            if (pal_idx)
                                *(uint32_t *)((uint8_t *)screen + address) = gba->mem->palette[pal_addr];
                            address += 4;
                        }
                    }
                }
                render_obj(prio);
            }
        } break;
        case 3: {
            uint8_t  x;
            uint32_t frm_addr = gba->io->v_count.w * 480;
            for (x = 0; x < 240; x++) {
                uint16_t pixel = gba->mem->vram[frm_addr + 0] | (gba->mem->vram[frm_addr + 1] << 8);
                uint8_t  r     = ((pixel >> 0) & 0x1f) << 3;
                uint8_t  g     = ((pixel >> 5) & 0x1f) << 3;
                uint8_t  b     = ((pixel >> 10) & 0x1f) << 3;
                uint32_t rgba  = 0xff;
                rgba |= (r | (r >> 5)) << 8;
                rgba |= (g | (g >> 5)) << 16;
                rgba |= (b | (b >> 5)) << 24;
                *(uint32_t *)((uint8_t *)screen + surf_addr) = rgba;
                surf_addr += 4;
                frm_addr += 2;
            }
        } break;
        case 4: {
            uint8_t  x, frame = (gba->io->disp_cnt.w >> 4) & 1;
            uint32_t frm_addr = 0xa000 * frame + gba->io->v_count.w * 240;
            for (x = 0; x < 240; x++) {
                uint8_t pal_idx                              = gba->mem->vram[frm_addr++];
                *(uint32_t *)((uint8_t *)screen + surf_addr) = gba->mem->palette[pal_idx];
                surf_addr += 4;
            }
        } break;
    }
}
void VIDEO::render_line()
{
    uint32_t addr;
    uint32_t addr_s = gba->io->v_count.w * 240 * 4;
    uint32_t addr_e = addr_s + 240 * 4;
    for (addr = addr_s; addr < addr_e; addr += 0x10) {
        *(uint32_t *)((uint8_t *)screen + (addr | 0x0)) = gba->mem->palette[0];
        *(uint32_t *)((uint8_t *)screen + (addr | 0x4)) = gba->mem->palette[0];
        *(uint32_t *)((uint8_t *)screen + (addr | 0x8)) = gba->mem->palette[0];
        *(uint32_t *)((uint8_t *)screen + (addr | 0xc)) = gba->mem->palette[0];
    }
    if ((gba->io->disp_cnt.w & 7) > 2) {
        render_bg();
        render_obj(0);
        render_obj(1);
        render_obj(2);
        render_obj(3);
    } else {
        render_bg();
    }
}
void VIDEO::vblank_start()
{
    if (gba->io->disp_stat.w & VBLK_IRQ)
        gba->io->trigger_irq(VBLK_FLAG);
    gba->io->disp_stat.w |= VBLK_FLAG;
}
void VIDEO::hblank_start()
{
    if (gba->io->disp_stat.w & HBLK_IRQ)
        gba->io->trigger_irq(HBLK_FLAG);
    gba->io->disp_stat.w |= HBLK_FLAG;
}
void VIDEO::vcount_match()
{
    if (gba->io->disp_stat.w & VCNT_IRQ)
        gba->io->trigger_irq(VCNT_FLAG);
    gba->io->disp_stat.w |= VCNT_FLAG;
}
