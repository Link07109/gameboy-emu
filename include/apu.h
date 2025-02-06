#pragma once

#include "common.h"

// 4 channels CH1-CH4
// each channel has components that control different parts of sound generation:
// sweep, freq, waveform, length timer, volume
// these are controlled by writing to the audio registers
//
// triggering = turn on if it was off, and start playing its wave from beginning
// most changes to channel params take effect immediately, but some require re-triggering the channel
//
// volume & env
// master volume control: separate settings for left and right outputs
// and each ch vol can be individually set as well
// env can be config for CH1 CH2 CH4, which allows for automatic setting of volume over time
// params that can be controlled: initial vol, envelope direction (not its slope), duration
// internally all env are ticked at 64hz, and every 1-7 of those ticks, vol will be inc or dec
//
// length timer
// all channels can be set to auto shutdown after a certain amount of time
// if enabled, timer ticks up at 256hz (tied to div-apu)
// when timer reaches 256 (CH3) or 64 (the other ones), channel is turned off
//
// freq
// periods will be used instead of freq bc apu works in durations

// audio regs
// NRxy
// x = channel (5 = global)
// y = reg id within that channel
// for example NR13 = channel 1, reg 3
// generally:
// y = 0 => channel spec feature if present
// y = 1 => length timer (and duty cycle)
// y = 2 => vol and env
// y = 3 => period (mayb only partially)
// y = 4 => trigger (control) and length timer enable bits, + leftover period bits
// BUT THERE ARE SOME EXCEPTIONS

// channel 1 (pulse with sweep)
// ff10-ff14 chart above

// channel 2 (pulse)
// ff15 sweep (doesnt exist)
// ff16-ff19 chart above

// channel 3 (voluntary wave)
// ff1a-ff1e

// channel 4 (noise)
// ff20-ff23

// global control regs
// ff24-ff26
// ff24 = master volume & VIN panning
// ff25 = sound panning
// ff26 = audio master control

// ?
// ff27-ff29

// wave pattern ram
// ff30-ff3f

typedef struct { // these are all registers
    // ff10 = channel 1 sweep
    // bit 0-2 = individual step
    // bit 3 = direction
    // bit 4-6 = pace
    // bit 7 = nothing
    u8 sweep;

    // ff11 = channel 1 length timer & duty cycle
    // bit 0-5 = initial length timer (write only)
    // bit 6-7 = wave duty (read/write)
    u8 length_timer;

    // ff12 = channel 1 volume & env
    // 0-2 = sweep pace
    // 3 = env dir
    // 4-7 = initial volume
    u8 volume;

    // ff13 = channel 1 period low (write only)
    // low 8 bits
    u8 period;

    // ff14 = channel 1 period high & control
    // 0-2 = period
    // 3-5 = nothing
    // 6-7 = trigger
    u8 control;
} channel;

    // ff1a = channel 3 dac enable
    //
    // ff1b = channel 3 length timer (write only)
    //
    // ff1c = channel 3 output level
    //
    // ff1d = channel 3 period low (write only)
    //
    // ff1e = channel 3 period high & control
    // 0-2 = period
    // 6 = length enable
    // 7 = trigger

typedef struct {
    channel channels[4];

    // nr50
    u8 right_volume: 3;
    u8 vin_right: 1;
    u8 left_volume: 3;
    u8 vin_left: 1;

    // nr51
    u8 panning; // right ch1-4, left ch1-4

    //nr 52
    u8 master_control; // ch1-4 on? --- audio on/off

    u8 wave_pattern_ram[16]; // each entry holds 2 samples, each 4 bits
} apu_context;

apu_context* apu_get_context();

void apu_init();
// channel outputs digital
u8 generation_circuit(channel ch); // this outputs 3 values (dac enable, digital, active flag)
u8 dac(bool enable, u8 channel_output); // returns analog val between -1 to 1
u8 mixer(u8 misc, u8 dac_output); // this outputs 2 values
u8 volume(u8 misc, u8 mixer1, u8 mixer2); // this also outputs 2 values
u8 hpf(u8 volume_output); // there are 2 of these
u8 out(u8 hpf1, u8 hpf2); // this is the final output

u8 apu_read(u16 address);
void apu_write(u16 address, u8 value);
