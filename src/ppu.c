#include "../include/ppu.h"
#include "../include/lcd.h"
#include "../include/ppu_sm.h"
#include <string.h>

static ppu_context ppu_ctx;

ppu_context* ppu_get_context() {
    return &ppu_ctx;
}

void ppu_init() {
    ppu_ctx.current_frame = 0;
    ppu_ctx.line_ticks = 0;
    ppu_ctx.video_buffer = malloc(YRES * XRES * sizeof(u32));
    
    ppu_ctx.pfc.line_x = 0;
    ppu_ctx.pfc.pushed_x = 0;
    ppu_ctx.pfc.fetch_x = 0;
    ppu_ctx.pfc.pixel_fifo.size = 0;
    ppu_ctx.pfc.pixel_fifo.head = ppu_ctx.pfc.pixel_fifo.tail = NULL;
    ppu_ctx.pfc.cur_fetch_state = FS_TILE;

    lcd_init();
    LCDS_MODE_SET(MODE_OAM);

    memset(ppu_ctx.oam_ram, 0, sizeof(ppu_ctx.oam_ram));
    memset(ppu_ctx.video_buffer, 0, YRES * XRES * sizeof(u32));
}

void ppu_tick() {
    ppu_ctx.line_ticks++;

    switch (LCDS_MODE) {
        case MODE_OAM:
            ppu_mode_oam();
            break;
        case MODE_XFER:
            ppu_mode_xfer();
            break;
        case MODE_VBLANK:
            ppu_mode_vblank();
            break;
        case MODE_HBLANK:
            ppu_mode_hblank();
            break;
    }
}

u8 ppu_oam_read(u16 address) {
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    u8* p = (u8*)ppu_ctx.oam_ram;
    return p[address];
}

void ppu_oam_write(u16 address, u8 value) {
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    u8* p = (u8*)ppu_ctx.oam_ram;
    p[address] = value;
}

u8 ppu_vram_read(u16 address) {
    return ppu_ctx.vram[address - 0x8000];
}

void ppu_vram_write(u16 address, u8 value) {
    ppu_ctx.vram[address - 0x8000] = value;
}
