#include "../include/ram.h"

typedef struct {
    u8 wram[0x2000];
    u8 hram[0x80];
} ram_context;

static ram_context ram_ctx;

u8 wram_read(u16 address) {
    address -= 0xC000;

    return ram_ctx.wram[address];
}

void wram_write(u16 address, u8 value) {
    address -= 0xC000;

    ram_ctx.wram[address] = value;
}

u8 hram_read(u16 address) {
    address -= 0xFF80;

    return ram_ctx.hram[address];
}

void hram_write(u16 address, u8 value) {
    address -= 0xFF80;

    ram_ctx.hram[address] = value;
}
