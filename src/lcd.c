#include "../include/lcd.h"
#include "../include/dma.h"

static lcd_context lcd_ctx;

lcd_context* lcd_get_context() {
    return &lcd_ctx;
}

static unsigned long colors_default[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

void lcd_init() {
    lcd_ctx.lcdc = 0x91;
    lcd_ctx.scroll_y = 0;
    lcd_ctx.scroll_x = 0;
    lcd_ctx.ly = 0;
    lcd_ctx.ly_compare = 0;
    lcd_ctx.bg_palette = 0xFC;
    lcd_ctx.obj_palette[0] = 0xFF;
    lcd_ctx.obj_palette[1] = 0xFF;
    lcd_ctx.win_y = 0;
    lcd_ctx.win_x = 0;

    for (int i = 0; i < 4; i++) {
        lcd_ctx.bg_colors[i] = colors_default[i];
        lcd_ctx.sp1_colors[i] = colors_default[i];
        lcd_ctx.sp2_colors[i] = colors_default[i];
    }
}

void update_palette(u8 palette_data, u8 pal) {
    u32* p_colors = lcd_ctx.bg_colors;

    switch(pal) {
        case 1:
            p_colors = lcd_ctx.sp1_colors;
            break;
        case 2:
            p_colors = lcd_ctx.sp2_colors;
            break;
    }

    p_colors[0] = colors_default[palette_data & 0b11];
    p_colors[1] = colors_default[(palette_data >> 2) & 0b11];
    p_colors[2] = colors_default[(palette_data >> 4) & 0b11];
    p_colors[3] = colors_default[(palette_data >> 6) & 0b11];
}

u8 lcd_read(u16 address) {
    u8 offset = address - 0xFF40;
    u8* p = (u8*)&lcd_ctx;

    return p[offset];
}

void lcd_write(u16 address, u8 value) {
    u8 offset = address - 0xFF40;
    u8* p = (u8*)&lcd_ctx;

    p[offset] = value;

    if (offset == 6) { // 0xFF46 (DMA)
        dma_start(value);
    }

    if (address == 0xFF47) {
        update_palette(value, 0);
    } else if (address == 0xFF48) {
        update_palette(value & 0b11111100, 1);
    } else if (address == 0xFF49) {
        update_palette(value & 0b11111100, 2);
    }
}
