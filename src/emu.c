#include "../include/emu.h"
#include "../include/cart.h"
#include "../include/cpu.h"
#include "../include/ui.h"
#include "../include/timer.h"
#include "../include/dma.h"
#include "../include/ppu.h"
#include "../include/apu.h"

// posix only, need something different for windows
#include <pthread.h>
#include <time.h>

/*
 * Emulator components:
 *
 * Cartridge
 * CPU
 * Address Bus
 * PPU
 * Timer
 */

static emu_context emu_ctx;

emu_context* emu_get_context() {
    return &emu_ctx;
}

void* cpu_run(void* p) {
    timer_init();
    apu_init();
    cpu_init();
    ppu_init();

    emu_ctx.running = true;
    emu_ctx.paused = false;
    emu_ctx.ticks = 0;

    while (emu_ctx.running) {
        if (emu_ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped");
            return 0;
        }
    }

    return 0;
}

int emu_run(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }
    if (!cart_load(argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }
    printf("Cart Loaded Successfully!\n");

    ui_init();

    pthread_t t1;
    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }

    u32 prev_frame = 0;
    
    while (!emu_ctx.die) {
        struct timespec interval = {
            .tv_sec = 0,
            .tv_nsec = 1000*1000 // usleep(1000);
        };
        nanosleep(&interval, NULL);

        ui_handle_events();

        if (prev_frame != ppu_get_context()->current_frame) {
            ui_update();
        }

        prev_frame = ppu_get_context()->current_frame;
    }

    ui_free();

    return 0;
}

void emu_cycles(int cpu_cycles) {
    for (int i = 0; i < cpu_cycles; i++) {
        for (int n = 0; n < 4; n++) {
            emu_ctx.ticks++;
            timer_tick();
            apu_tick();
            ppu_tick();
        }

        dma_tick();
    }
}
