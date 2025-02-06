#include "../include/apu.h"
//#include <string.h>

static apu_context apu_ctx;

apu_context* apu_get_context() {
    return &apu_ctx;
}

void apu_init() {
    //memset(apu_ctx.wave_pattern_ram, 0, sizeof(apu_ctx.wave_pattern_ram));
}

u8 generation_circuit(channel ch) { // this outputs 3 values (dac enable, digital, active flag)
    return 0;
}

u8 dac(bool enable, u8 channel_output) { // returns analog val between -1 to 1
    return 0;
}

u8 mixer(u8 misc, u8 dac_output) { // this outputs 2 values
    return 0;
}

u8 volume(u8 misc, u8 mixer1, u8 mixer2) { // this also outputs 2 values
    return 0;
}

u8 hpf(u8 volume_output) { // there are 2 of these
    return 0;
}

u8 out(u8 hpf1, u8 hpf2) { // this is the final output
    return 0;
}

u8 pulse_channel_read(u16 address) {
    channel chan = apu_ctx.channels[address / 5];
    u16 offset = address % 5;
    switch(offset) {
        case 0: return chan.sweep;
        case 1: return chan.length_timer;
        case 2: return chan.volume;
        //case 3: return chan.period; // this is write only
        case 4: return chan.control;
    }

    return 0;
}

void pulse_channel_write(u16 address, u8 value) {
    channel chan = apu_ctx.channels[address / 5];
    u16 offset = address % 5;
    switch(offset) {
        case 0: chan.sweep = value;
        case 1: chan.length_timer = value;
        case 2: chan.volume = value;
        case 3: chan.period = value;
        case 4: chan.control = value;
    }
}

u8 global_control_regs_read(u16 address) {
    if (address == 0xFF24) { // nr50
        u8 nr50 = 0;
        nr50 |= apu_ctx.right_volume;
        nr50 |= apu_ctx.vin_right << 3;
        nr50 |= apu_ctx.left_volume << 4;
        nr50 |= apu_ctx.vin_left << 7;
        return nr50;
    }
    if (address == 0xFF25) { // nr51
        return apu_ctx.panning;
    }
    if (address == 0xFF26) { // nr52
        return apu_ctx.master_control;
    }

    return 0;
}

void global_control_regs_write(u16 address, u8 value) {
    if (address == 0xFF24) { // nr50
        apu_ctx.right_volume = value & (0b111 << 0);
        apu_ctx.vin_right = value & (1 << 3);
        apu_ctx.left_volume = value & (0b111 << 4);
        apu_ctx.vin_left = value & (1 << 7);
    }
    if (address == 0xFF25) { // nr51
        apu_ctx.panning = value;
    }
    if (address == 0xFF26) { // nr52
        apu_ctx.master_control = value;
    }
}

u8 apu_read(u16 address) {
    if (BETWEEN(address, 0xFF10, 0xFF19)) { // pulse channels
        return pulse_channel_read(address - 0xFF10);
    }
    if (BETWEEN(address, 0xFF10, 0xFF23)) { // other channels
    }
    if (BETWEEN(address, 0xFF24, 0xFF26)) { // global control regs
        return global_control_regs_read(address);
    }
    if (BETWEEN(address, 0xFF30, 0xFF3F)) { // wave pattern ram
        return apu_ctx.wave_pattern_ram[address - 0xFF30];
    }
    // read output sound
    printf("apu_read(%2X)\n", address);
    return 0;
}

void apu_write(u16 address, u8 value) {
    if (BETWEEN(address, 0xFF10, 0xFF19)) { // pulse channels
        pulse_channel_write(address - 0xFF10, value);
        return;
    }
    if (BETWEEN(address, 0xFF20, 0xFF23)) { // other channels
    }
    if (BETWEEN(address, 0xFF24, 0xFF26)) { // global control regs
        global_control_regs_write(address, value);
        return;
    }
    if (BETWEEN(address, 0xFF30, 0xFF3F)) { // wave pattern ram
        apu_ctx.wave_pattern_ram[address - 0xFF30] = value;
        return;
    }
    // output sound
    printf("apu_write(%2X)\n", address);
}
