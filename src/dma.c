#include "../include/dma.h"
#include "../include/ppu.h"
#include "../include/bus.h"

typedef struct {
    bool active;
    u8 byte;
    u8 value;
    u8 start_delay;
} dma_context;

static dma_context dma_ctx;

void dma_start(u8 start) {
    dma_ctx.active = true;
    dma_ctx.byte = 0;
    dma_ctx.start_delay = 2;
    dma_ctx.value = start;
}

void dma_tick() {
    if (!dma_ctx.active) {
        return;
    }

    if (dma_ctx.start_delay) {
        dma_ctx.start_delay--;
        return;
    }

    ppu_oam_write(dma_ctx.byte, bus_read((dma_ctx.value * 0x100) + dma_ctx.byte));

    dma_ctx.byte++;

    dma_ctx.active = dma_ctx.byte < 0xA0;
}

bool dma_transferring() {
    return dma_ctx.active;
}
