#include "bus.c"
#include "cart.c"
#include "cpu.c"
#include "cpu_fetch.c"
#include "cpu_proc.c"
#include "cpu_util.c"
#include "dbg.c"
#include "dma.c"
#include "emu.c"
#include "instructions.c"
#include "interrupts.c"
#include "io.c"
#include "lcd.c"
#include "ppu.c"
#include "ppu_pipeline.c"
#include "ppu_sm.c"
#include "ram.c"
#include "stack.c"
#include "timer.c"
#include "ui.c"

int main(int argc, char** argv) {
    return emu_run(argc, argv);
}
