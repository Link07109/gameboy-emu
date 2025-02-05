#include "../include/cpu.h"
#include "../include/emu.h"
#include "../include/bus.h"

extern cpu_context cpu_ctx;

void fetch_data() {
    cpu_ctx.mem_dest = 0;
    cpu_ctx.dest_is_mem = false;

    if (cpu_ctx.cur_inst == NULL) {
        return;
    }

    switch(cpu_ctx.cur_inst->mode) {
        case AM_IMP: return;

        case AM_R:
            cpu_ctx.fetched_data = cpu_read_reg(cpu_ctx.cur_inst->reg_1);
            return;

        case AM_R_R:
            cpu_ctx.fetched_data = cpu_read_reg(cpu_ctx.cur_inst->reg_2);
            return;

        case AM_R_D8:
            cpu_ctx.fetched_data = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);
            cpu_ctx.regs.pc++;
            return;

        case AM_R_D16:
        case AM_D16: {
            u16 lo = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(cpu_ctx.regs.pc + 1);
            emu_cycles(1);

            cpu_ctx.fetched_data = lo | (hi << 8);

            cpu_ctx.regs.pc += 2;
            return;
        }

        case AM_MR_R:
            cpu_ctx.fetched_data = cpu_read_reg(cpu_ctx.cur_inst->reg_2);
            cpu_ctx.mem_dest = cpu_read_reg(cpu_ctx.cur_inst->reg_1);
            cpu_ctx.dest_is_mem = true;

            if (cpu_ctx.cur_inst->reg_1 == RT_C) {
                cpu_ctx.mem_dest |= 0xFF00;
            }
            return;

        case AM_R_MR: {
            u16 addr = cpu_read_reg(cpu_ctx.cur_inst->reg_2);

            if (cpu_ctx.cur_inst->reg_2 == RT_C) {
                addr |= 0xFF00;
            }

            cpu_ctx.fetched_data = bus_read(addr);
            emu_cycles(1);
        } return;

        case AM_R_HLI:
            cpu_ctx.fetched_data = bus_read(cpu_read_reg(cpu_ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;

        case AM_R_HLD:
            cpu_ctx.fetched_data = bus_read(cpu_read_reg(cpu_ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;

        case AM_HLI_R:
            cpu_ctx.fetched_data = cpu_read_reg(cpu_ctx.cur_inst->reg_2);
            cpu_ctx.mem_dest = cpu_read_reg(cpu_ctx.cur_inst->reg_1);
            cpu_ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;

        case AM_HLD_R:
            cpu_ctx.fetched_data = cpu_read_reg(cpu_ctx.cur_inst->reg_2);
            cpu_ctx.mem_dest = cpu_read_reg(cpu_ctx.cur_inst->reg_1);
            cpu_ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;

        case AM_R_A8:
            cpu_ctx.fetched_data = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);
            cpu_ctx.regs.pc++;
            return;

        case AM_A8_R:
            cpu_ctx.mem_dest = bus_read(cpu_ctx.regs.pc) | 0xFF00;
            cpu_ctx.dest_is_mem = true;
            emu_cycles(1);
            cpu_ctx.regs.pc++;
            return;

        case AM_HL_SPR:
        case AM_D8:
            cpu_ctx.fetched_data = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);
            cpu_ctx.regs.pc++;
            return;

        case AM_A16_R:
        case AM_D16_R: {
            u16 lo = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(cpu_ctx.regs.pc + 1);
            emu_cycles(1);

            cpu_ctx.mem_dest = lo | (hi << 8);
            cpu_ctx.dest_is_mem = true;

            cpu_ctx.regs.pc += 2;
            cpu_ctx.fetched_data = cpu_read_reg(cpu_ctx.cur_inst->reg_2);
        } return;

        case AM_MR_D8:
            cpu_ctx.fetched_data = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);
            cpu_ctx.regs.pc++;
            cpu_ctx.mem_dest = cpu_read_reg(cpu_ctx.cur_inst->reg_1);
            cpu_ctx.dest_is_mem = true;
            return;

        case AM_MR:
            cpu_ctx.mem_dest = cpu_read_reg(cpu_ctx.cur_inst->reg_1);
            cpu_ctx.dest_is_mem = true;
            cpu_ctx.fetched_data = bus_read(cpu_read_reg(cpu_ctx.cur_inst->reg_1));
            emu_cycles(1);
            return;
            
        case AM_R_A16: {
            u16 lo = bus_read(cpu_ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(cpu_ctx.regs.pc + 1);
            emu_cycles(1);

            cpu_ctx.regs.pc += 2;
            cpu_ctx.fetched_data = bus_read(lo | (hi << 8));
            emu_cycles(1);
            return;
        }

        default:
            printf("Unknown Addressing Mode! %d\n", cpu_ctx.cur_inst->mode);
            exit(-7); // fatal error
            return;
    }
}
