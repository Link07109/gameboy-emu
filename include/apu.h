#pragma once

#include "common.h"

typedef struct {
    u32 samples;
    u32 bytes;
    u32 sample_index;
    float data[2048];
} buffer;

typedef struct {
    u8 dac_enable: 1;
    u8 active: 1;
    u8 mute: 1;

    float level;
    u8 left_enable: 1;
    u8 right_enable: 1;


    u8 reg0;
    u8 sweep_enable: 1;
    u8 sweep_pace: 3; // set to 0 = disable iterations
    u8 sweep_counter; // the actual thing that changes
    u8 sweep_dir: 1;
    u8 sweep_step: 3;


    u8 reg1;
    u8 duty_val;
    u8 duty_step_counter; // increments at the channels sample rate (8 * chan freq)
                          // apu off sets this to 0
                          // triggering pulse channel resets this also
                          // when first starting up channel, always outputs digi 0
    u8 length_gate: 1;
    u16 length_timer_counter; // len_gate
    u8 initial_length_timer; // channel 3 = all 8 bits are timer
    u8 length_timer_enable: 1; // noise channel


    u8 reg2;
    u8 initial_volume; // reg value (4 bit for pulse, 2 bit for wave)
    i16 env_volume;
    u8 env_dir: 1;
    u8 env_sweep_pace; // how many ticks determines when env will be updated (0 disables env)
    u16 env_counter;


    u8 reg3;
    u8 period_low; // low 8 bits
    u8 period_high: 3; // high 3 bits
    u16 period_val: 11; // full 11 bit period
    u16 period_shadow;
    u16 period_div_tc; // tc = tick count
    u16 period_div_counter;


    u8 reg4;
    u8 trigger: 1;
} channel;

typedef struct {
    u8 apu_on;

    // [0] = left, [1] = right
    float capacitors[2];
    float mixer_out[2];
    float volume_out[2];
    float hpf_out[2];

    buffer* sound_buf;

    float hpf_constant;
    bool hpf_enabled;

    u8 sequence_counter; // DIV APU

    i32 div_apu_hz_counter;
    u16 div_apu_hz_tc; // 8192

    i16 sound_div_counter;
    u16 sound_div_tc; // 95

    // wave channel
    u8 ch3_next_nibble;
    u8 ch3_pos;

    // noise channel
    u16 lfsr;
    bool lfsr_narrow;
    u8 clock_shift;
    u8 clock_divider;

    // nr10 - nr44
    channel channels[4];

    // nr50
    u8 nr50;
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
void apu_tick();

u8 apu_read(u16 address);
void apu_write(u16 address, u8 value);
