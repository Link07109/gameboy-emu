#include "../include/cpu.h"
#include "../include/bus.h"
#include "../include/emu.h"
#include "../include/interrupts.h"
#include "../include/dbg.h"
#include "../include/timer.h"

cpu_context cpu_ctx = { 0 };

void cpu_init() {
    cpu_ctx.regs.pc = 0x100;
    cpu_ctx.regs.sp = 0xFFFE;
    *((short*)&cpu_ctx.regs.a) = 0xB001;
    *((short*)&cpu_ctx.regs.b) = 0x1300;
    *((short*)&cpu_ctx.regs.d) = 0xD800;
    *((short*)&cpu_ctx.regs.h) = 0x4D01;
    cpu_ctx.ie_register = 0;
    cpu_ctx.int_flags = 0;
    cpu_ctx.int_master_enabled = false;
    cpu_ctx.enabling_ime = false;

    timer_get_context()->div = 0xABCC;
}

static void fetch_instruction() {
    cpu_ctx.cur_opcode = bus_read(cpu_ctx.regs.pc++);
    cpu_ctx.cur_inst = instruction_by_opcode(cpu_ctx.cur_opcode);
}

void fetch_data();

static void execute() {
    IN_PROC proc = inst_get_processor(cpu_ctx.cur_inst->type);

    if (!proc) {
        NO_IMPL
    }

    proc(&cpu_ctx);
}

bool cpu_step() {
    if (!cpu_ctx.halted) {
        //u16 pc = cpu_ctx.regs.pc;
        fetch_instruction();
        emu_cycles(1);
        fetch_data();

        /*
        char flags[16];
        sprintf(flags, "%c%c%c%c", 
            cpu_ctx.regs.f & (1 << 7) ? 'Z' : '-',
            cpu_ctx.regs.f & (1 << 6) ? 'N' : '-',
            cpu_ctx.regs.f & (1 << 5) ? 'H' : '-',
            cpu_ctx.regs.f & (1 << 4) ? 'C' : '-'
        );

        char inst[16];
        inst_to_str(&cpu_ctx, inst);

        printf("%08lX - %04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n", 
            emu_get_context()->ticks,
            pc, inst, cpu_ctx.cur_opcode,
            bus_read(pc + 1), bus_read(pc + 2), cpu_ctx.regs.a, flags, cpu_ctx.regs.b, cpu_ctx.regs.c,
            cpu_ctx.regs.d, cpu_ctx.regs.e, cpu_ctx.regs.h, cpu_ctx.regs.l);
        */

        if (cpu_ctx.cur_inst == NULL) {
            printf("Unknown Instruction! %2.2X\n", cpu_ctx.cur_opcode);
            exit(-7);
        }

        dbg_update();
        dbg_print();

        execute();
    } else {
        emu_cycles(1);

        if (cpu_ctx.int_flags) {
            cpu_ctx.halted = false;
        }
    }

    if (cpu_ctx.int_master_enabled) {
        cpu_handle_interrupts(&cpu_ctx);
        cpu_ctx.enabling_ime = false;
    }

    if (cpu_ctx.enabling_ime) {
        cpu_ctx.int_master_enabled = true;
    }

    return true;
}

void cpu_request_interrupt(interrupt_type t) {
        cpu_ctx.int_flags |= t;
}
