#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../include/apu.h"

static apu_context apu_ctx;

apu_context* apu_get_context() {
    return &apu_ctx;
}

void audio_play(void* buf, u32 count);

static const i32 GB_CLOCK = 4194304;
static const u8 duty_lookup[4] = { 0x02, 0x03, 0x0F, 0xFC };

static void lfsr_reset() {
    apu_ctx.lfsr = ((1 << 15) - 1);
    apu_ctx.lfsr_narrow = false;
}

static int lfsr_next_value() {
    u16 xor_result = (apu_ctx.lfsr & 0b01) ^ ((apu_ctx.lfsr & 0b10) >> 1);
    apu_ctx.lfsr = (apu_ctx.lfsr>> 1) | (xor_result << 14);

    if (apu_ctx.lfsr_narrow) {
        apu_ctx.lfsr &= ~(1 << 6);
        apu_ctx.lfsr |= xor_result << 6;
    }

    return apu_ctx.lfsr;
}

static void update_filter() {
    //double cap_constant = pow(apu_ctx.hpf_constant, (double)GB_CLOCK / apu_ctx.sample_rate); // constant pow of 95.108934240362 
    // cap_constant = 0.9960133089108
    apu_ctx.capacitor_factor = 65275;//round(65536.0 * cap_constant); // = 65274.728212781
}

void update_length_timer(channel* chan) {
    if (!chan->length_timer_enable) {
        return;
    }

    chan->length_timer_counter--;

    if (chan->length_timer_counter <= 0) {
        chan->active = 0;

        chan->env_volume = 0;
        chan->env_sweep_pace = 0; // apu_write() this?
    }
}

static bool sweep_check_overflow(u16 freq) {
    channel* ch1 = &apu_ctx.channels[0];

    if (freq > 2047) {
        ch1->active = 0;
        return true;
    }
    
    return false;
}

static u16 freq_calc() {
    channel* ch1 = &apu_ctx.channels[0];
    u16 new_freq;

    if (ch1->sweep_dir) {
        new_freq = ch1->period_shadow + (ch1->period_shadow >> ch1->sweep_step);
    } else {
        new_freq = ch1->period_shadow - (ch1->period_shadow >> ch1->sweep_step);
    }

    sweep_check_overflow(new_freq);

    return new_freq;
}

static void div_apu_clock() {
    bool clock_sweep = false;
    bool clock_len = false;
    bool clock_env = false;

    switch (apu_ctx.sequence_counter & 7) { // same as % 8
        case 0:
            clock_len = true;
            break;
        case 1:
            break;
        case 2:
            clock_len = true;
            clock_sweep = true;
            break;
        case 3:
            break;
        case 4:
            clock_len = true;
            break;
        case 5:
            break;
        case 6:
            clock_len = true;
            clock_sweep = true;
            break;
        case 7:
            clock_env = true;
            break;
    }
    apu_ctx.sequence_counter++;

    channel* ch1 = &apu_ctx.channels[0];
    if (clock_sweep) {
        ch1->sweep_counter--;

        if (ch1->sweep_counter <= 0) { // "sweep iteration"
            ch1->sweep_counter = ch1->sweep_pace;
            if (ch1->sweep_pace == 0) {
                ch1->sweep_counter = 8;
            }

            if (ch1->sweep_enable && ch1->sweep_pace) {
                u16 new_freq = freq_calc();

                if (!sweep_check_overflow(new_freq) && ch1->sweep_step) {
                    ch1->period_shadow = new_freq;
                    ch1->period_val = new_freq; // this probably not needed
                    apu_write(0xFF13, new_freq); // low 8 bits
                    apu_write(0xFF14, (ch1->length_timer_enable << 6) | (new_freq >> 8)); // high 3 bits

                    // again! (BUT DONT WRITE TO STUFF)
                    freq_calc();
                }
            }
        }
    }

    for (int i = 0; clock_len && i < 4; i++) {
        update_length_timer(&apu_ctx.channels[i]);
    }

    for (int i = 0; clock_env && i < 4; i++) {
        channel* chan = &apu_ctx.channels[i];

        if (chan == &apu_ctx.channels[2]) {
            continue;
        }

        if (!chan->env_sweep_pace) {
            continue;
        }

        chan->env_counter--;

        if (chan->env_counter <=0 ) {
            chan->env_counter = chan->env_sweep_pace;

            if (!chan->active) {
                continue;
            }

            if (!chan->env_dir) {
                if (chan->env_volume > 0) {
                    chan->env_volume--;
                }
            } else {
                if (chan->env_volume < 15) {
                    ch1->env_volume++;
                }
            }

        }
    }

    /*
    if (apu_ctx.master_fade) {
        apu_ctx.master_volume += apu_ctx.master_fade;

        if ((apu_ctx.master_fade > 0 && apu_ctx.master_volume >= apu_ctx.master_dstvol) ||
                (apu_ctx.master_fade < 0 && apu_ctx.master_volume <= apu_ctx.master_dstvol)) {

            apu_ctx.master_fade = 0;
            apu_ctx.master_volume = apu_ctx.master_dstvol;
        }
    }
    */
}

void apu_reset() {
    int mute_tmp[4];
    for (int i = 0; i < 4; i++) {
        mute_tmp[i] = apu_ctx.channels[i].mute;
    }

    memset(apu_ctx.channels, 0, sizeof(apu_ctx.channels));

    printf("== apu_write @ apu_reset() starting!\n");
    for (int i = 0xFF10; i < 0xFF26; i++) {
        if (i == 0xFF15 || i == 0xFF1F) {
            continue;
        }
        apu_write(i, 0);
    }
    printf("== apu_write @ apu_reset() done!\n");

    for (int i = 0; i < 4; i++) {
        apu_ctx.channels[i].initial_length_timer = 0;
        apu_ctx.channels[i].length_timer_enable = 0;

        apu_ctx.channels[i].duty_step_counter = 0;

        apu_ctx.channels[i].initial_volume = 0;
        apu_ctx.channels[i].env_volume = 0;

        apu_ctx.channels[i].dac_enable = 1;
        apu_ctx.channels[i].active = 0;
        apu_ctx.channels[i].mute = mute_tmp[i];
    }

    apu_ctx.ch3_pos = 0;
    apu_ctx.sequence_counter = 0;
}

void apu_init() {
    apu_ctx.apu_on = 1;

    apu_ctx.hpf_constant = 0.999958;
    apu_ctx.hpf_enabled = 1;
    apu_ctx.capacitor_factor = 0x10000; // 65536.0

    apu_ctx.panning = 0;
    apu_ctx.master_control = 0;

    apu_ctx.right_volume = 0;
    apu_ctx.vin_right = 0;
    apu_ctx.left_volume = 0;
    apu_ctx.vin_left = 0;

    apu_ctx.div_apu_hz_tc = 8192; // T-cycles before div-apu clock (8192)
    apu_ctx.div_apu_hz_counter = apu_ctx.div_apu_hz_tc;

    apu_ctx.sound_div_tc = 95; // T-cycles before flush buffer (95)
    apu_ctx.sound_div_counter = apu_ctx.sound_div_tc;
    update_filter();

    apu_ctx.ch3_pos = 0;
    apu_ctx.ch3_next_nibble = 0;

    apu_ctx.master_volume = 100;
    apu_ctx.master_fade = 0;

    apu_ctx.capacitors[0] = 0;
    apu_ctx.capacitors[1] = 0;

    apu_reset();
    lfsr_reset();

    memset(apu_ctx.wave_pattern_ram, 0, sizeof(apu_ctx.wave_pattern_ram));
    printf("== apu_init() complete!\n");
}

static u8 global_control_regs_read(u16 address) {
    if (address == 0xFF24) return apu_ctx.nr50 | 0x00; // nr50
    if (address == 0xFF25) return apu_ctx.panning | 0x00; // nr51
    if (address == 0xFF26) return apu_ctx.master_control | 0x70; // nr52

    return 0xFF;
}

static void global_control_regs_write(u16 address, u8 value) {
    if (address == 0xFF24) { // nr50
        apu_ctx.nr50 = value;

        apu_ctx.right_volume = value & 0b111;
        apu_ctx.vin_right = !!(value & (1 << 3));
        apu_ctx.left_volume = (value >> 4) & 0b111;
        apu_ctx.vin_left = !!(value & (1 << 7));

        if (apu_ctx.left_volume == 0) apu_ctx.left_volume = 1;
        // if (apu_ctx.left_volume == 7) apu_ctx.left_volume = 8; // do this check somewhere else

        if (apu_ctx.right_volume == 0) apu_ctx.right_volume = 1;
        // if (apu_ctx.right_volume == 7) apu_ctx.right_volume = 8; // do this check somewhere else
    }
    if (address == 0xFF25) { // nr51
        apu_ctx.panning = value;

        apu_ctx.channels[0].right_enable = !!(value & 0x01);
        apu_ctx.channels[1].right_enable = !!(value & 0x02);
        apu_ctx.channels[2].right_enable = !!(value & 0x04);
        apu_ctx.channels[3].right_enable = !!(value & 0x08);

        apu_ctx.channels[0].left_enable = !!(value & 0x10);
        apu_ctx.channels[1].left_enable = !!(value & 0x20);
        apu_ctx.channels[2].left_enable = !!(value & 0x40);
        apu_ctx.channels[3].left_enable = !!(value & 0x80);
    }
    if (address == 0xFF26) { // nr52
        apu_ctx.master_control = value & 0x80;

        printf("NR52 WRITE: %02X \n", value);

        bool temp = !!(value & 0x80);
        if (!temp) {
            apu_reset(); // turn off apu
        }
        apu_ctx.apu_on = temp;
    }
}

static u8 pulse_channel_read(u16 address) {
    u8 offset = (address - 0xFF10) % 5;
    channel* pulse_channel = &apu_ctx.channels[(address - 0xFF10) / 5];

    u8 reg0_or = 0x80;
    if (pulse_channel == &apu_ctx.channels[1]) {
        reg0_or = 0xFF;
    }

    switch(offset) {
        case 0: return pulse_channel->reg0 | reg0_or;
        case 1: return pulse_channel->reg1 | 0x3F;
        case 2: return pulse_channel->reg2 | 0x00;
        case 3: return pulse_channel->reg3 | 0xFF;//printf("NR13 and NR23 are write-only!\n"); break;
        case 4: return pulse_channel->reg4 | 0xBF;
    }

    return 0;
}

static void trigger(channel* chan) {
    if (chan->dac_enable) {
        chan->active = 1;
    }

    if (chan->length_timer_counter == 0) {
        chan->length_timer_counter = 64;
        if (chan == &apu_ctx.channels[2]) {
            chan->length_timer_counter = 256;
        }
    }

    // pulse
    chan->period_div_counter = chan->period_div_tc * 4;
    // wave
    if (chan == &apu_ctx.channels[2]) {
        apu_ctx.ch3_pos = 0; // wave ram index is reset, but is NOT refilled
        chan->period_div_counter = chan->period_div_tc * 2;
    }
    // noise
    if (chan == &apu_ctx.channels[3]) {
        chan->period_div_counter = chan->period_div_tc;
    }

    chan->env_counter = chan->env_sweep_pace;
    chan->env_volume = chan->initial_volume;
    chan->duty_step_counter = 0;

    // sweep shit
    if (chan != &apu_ctx.channels[0]) {
        return;
    }

    chan->period_shadow = chan->period_val;
    chan->sweep_counter = chan->sweep_pace;
    if (chan->sweep_pace == 0) {
        chan->sweep_counter = 8;
    }

    if (chan->sweep_pace || chan->sweep_step) {
        chan->sweep_enable = 1;
    } else {
        chan->sweep_enable = 0;
    }

    if (chan->sweep_step == 0) {
        return;
    }
    freq_calc();
}

static void pulse_channel_reg1_write(u8 value, channel* chan) {
    chan->duty_val = duty_lookup[(value >> 6) & 3];
    chan->initial_length_timer = value & 0x3F; // 0b00111111
    chan->length_timer_counter = 64 - chan->initial_length_timer;
}

static void pulse_channel_reg2_write(u8 value, channel* chan) {
    chan->initial_volume = value >> 4;
    chan->env_dir = !!((value >> 3) & 1);

    u8 envspd = value & 0x7; // 0b111
    chan->env_sweep_pace = envspd;
    chan->env_counter = envspd;
    if (envspd == 0) {
        chan->env_counter = 8;
    }

    chan->dac_enable = !!(value & 0xF8);
    if (!chan->dac_enable) {
        chan->active = 0;
    }

    if (chan->active) {
        trigger(chan);
    }
}

static void pulse_channel_reg3_write(u8 value, channel* chan) {
    chan->period_low = value;

    chan->period_val = (chan->period_high << 8) | chan->period_low;
    chan->period_div_tc = 2048 - chan->period_val;
}

static void pulse_channel_reg4_write(u8 value, channel* chan) {
    chan->period_high = value & 0x7; // 0b111 lower 3 bits

    chan->period_val = (chan->period_high << 8) | chan->period_low; // make 11 bit value
    chan->period_div_tc = 2048 - chan->period_val;

    u8 old_length_enable = chan->length_timer_enable;
    chan->length_timer_enable = !!(value & 0x40); // 0b01000000 6th bit
    chan->trigger = !!(value & 0x80); // 0b10000000 7th bit

    if (chan->trigger) {
        trigger(chan);
    }

    // Enabling in first half of length period clocks length
    if (!old_length_enable && chan->length_timer_enable && (apu_ctx.sequence_counter & 1) == 1) {
        update_length_timer(chan);
    }
}

static void pulse_channel_write(u16 address, u8 value) {
    u8 offset = (address - 0xFF10) % 5;
    u8 index = (address - 0xFF10) / 5;
    channel* pulse_channel = &apu_ctx.channels[index];

    switch(offset) {
        case 0:
            if (pulse_channel == &apu_ctx.channels[1]) {
                printf("Pulse Channel 2 Doesn't Have Sweep!\n");
                break;
            }

            pulse_channel->reg0 = value;

            u8 pace = ((value >> 4) & 0x7);
            pulse_channel->sweep_pace = pace;
            pulse_channel->sweep_counter = pace;
            if (pace == 0) {
                pulse_channel->sweep_counter = 8;
            }

            pulse_channel->sweep_dir = (value >> 3) & 1;
            pulse_channel->sweep_step = value & 0x7; // 0b111
            break;
        case 1: 
            pulse_channel->reg1 = value;
            pulse_channel_reg1_write(value, pulse_channel);
            break;
        case 2:
            pulse_channel->reg2 = value;
            pulse_channel_reg2_write(value, pulse_channel);
            break;
        case 3: // nr13 (write only)
            pulse_channel->reg3 = value;
            pulse_channel_reg3_write(value, pulse_channel);
            break;
        case 4: 
            pulse_channel->reg4 = value;
            pulse_channel_reg4_write(value, pulse_channel);
            break;
    }
}

static u8 wave_channel_read(u16 address) {
    u8 offset = address - 0xFF1A;
    channel* wave_channel = &apu_ctx.channels[2];

    switch (offset) {
        case 0: return wave_channel->reg0 | 0x7F; // nr30
        case 1: return wave_channel->reg1 | 0xFF;//printf("NR31 is write-only!\n"); break;
        case 2: return wave_channel->reg2 | 0x9F; // nr32
        case 3: return wave_channel->reg3 | 0xFF;//printf("NR33 is write-only!\n"); break;
        case 4: return wave_channel->reg4 | 0xBF; // nr34
    }

    return 0;
}

static void wave_channel_write(u16 address, u8 value) {
    u8 offset = address - 0xFF1A;
    channel* wave_channel = &apu_ctx.channels[2];

    switch(offset) {
        case 0: // nr30
            wave_channel->reg0 = value;

            wave_channel->dac_enable = !!(value & 0x80);
            if (!wave_channel->dac_enable) {
                wave_channel->active = 0; // turn off the channel (this might be wrong)
            }
            break;
        case 1: // nr31 (write only)
            wave_channel->reg1 = value;

            wave_channel->initial_length_timer = value;
            wave_channel->length_timer_counter = 256 - wave_channel->initial_length_timer;
            break;
        case 2: { // nr32
            wave_channel->reg2 = value;

            // 00 = mute
            // 01 = 100% (use samples read from Wave RAM as-is)
            // 10 = 50% (shift samples read from Wave RAM right once)
            // 11 = 25% (shift samples read from Wave RAM right twice)
            u8 vol = (value >> 5) & 0b11;
            i8 wave_volume_lt[4] = { 4, 0, 1, 2 };
            wave_channel->env_volume = wave_volume_lt[vol];
            wave_channel->initial_volume = wave_volume_lt[vol];

            /*
            if (vol == 0) {
                wave_channel->mute = 1;
            } else {
                wave_channel->mute = 0;
            }
            */
            break;
        }
        case 3: // nr33 (write-only) (this is 2x faster than nr13 and nr23)
            wave_channel->reg3 = value;
            pulse_channel_reg3_write(value, wave_channel);
            break;
        case 4: // nr34
            wave_channel->reg4 = value;
            pulse_channel_reg4_write(value, wave_channel);
            break;
    }
}

static u8 noise_channel_read(u16 address) {
    u8 offset = address - 0xFF20;
    channel* noise_channel = &apu_ctx.channels[3];

    switch (offset) {
        case 0: return noise_channel->reg0 | 0xFF;//printf("NR41 is write-only!\n"); break;
        case 1: return noise_channel->reg1 | 0x00; // nr42 (same as nr12)
        case 2: return noise_channel->reg2 | 0x00; // nr43
        case 3: return noise_channel->reg3 | 0xBF; // nr44
    }

    return 0;
}

static void noise_channel_write(u16 address, u8 value) {
    u8 offset = address - 0xFF20;
    channel* noise_channel = &apu_ctx.channels[3];

    switch (offset) {
        case 0: // nr41 (ch4 length timer) (write only)
            noise_channel->reg0 = value;
            pulse_channel_reg1_write(value, noise_channel);
            break;
        case 1: // nr42 (same as nr12)
            noise_channel->reg1 = value;
            pulse_channel_reg2_write(value, noise_channel);
            break;
        // controlled by lfsr
        case 2: { // nr43 (ch4 freq and randomness)
            noise_channel->reg2 = value;

            apu_ctx.clock_shift = value >> 4;

            noise_channel->period_div_tc = 16 << apu_ctx.clock_shift;
            apu_ctx.lfsr_narrow = !!(value & 8);

            apu_ctx.clock_divider = value & 7; // see formula
            if (apu_ctx.clock_divider) {
                noise_channel->period_div_tc *= apu_ctx.clock_divider;
            } else {
                noise_channel->period_div_tc /= 2;
            }
            noise_channel->period_div_counter = noise_channel->period_div_tc;
        } break;
        case 3: { // nr44 (ch4 control)
            noise_channel->reg3 = value;

            u8 old_length_enable = noise_channel->length_timer_enable;

            noise_channel->length_timer_enable = !!(value & 0x40); // 0b01000000
            noise_channel->trigger = !!(value & 0x80);

            if (noise_channel->trigger) {
                apu_ctx.lfsr = ((1 << 15) - 1);
                trigger(noise_channel);
            }

            // Enabling in first half of length period clocks length
            if (!old_length_enable && noise_channel->length_timer_enable && (apu_ctx.sequence_counter & 1) == 1) {
                update_length_timer(noise_channel);
            }
        } break;
    }
}

u8 apu_read(u16 address) {
    if (BETWEEN(address, 0xFF10, 0xFF19)) { // pulse channels
        return pulse_channel_read(address);
    }
    if (BETWEEN(address, 0xFF1A, 0xFF1E)) { // wave channel
        return wave_channel_read(address);
    }
    if (BETWEEN(address, 0xFF20, 0xFF23)) { // noise channel
        return noise_channel_read(address);
    }
    if (BETWEEN(address, 0xFF24, 0xFF26)) { // global control regs
        return global_control_regs_read(address);
    }
    if (BETWEEN(address, 0xFF30, 0xFF3F)) { // wave pattern ram
        return apu_ctx.wave_pattern_ram[address - 0xFF30];
    }
    printf("Invalid apu_read($%2X)\n", address);
    return 0xFF;
}

void apu_write(u16 address, u8 value) {
    if (BETWEEN(address, 0xFF30, 0xFF3F)) { // wave pattern ram
        //printf("Wave RAM Write\n");
        apu_ctx.wave_pattern_ram[address - 0xFF30] = value;
        return;
    }

    if (!apu_ctx.apu_on && address != 0xFF26 && address != 0xFF20) {
        // TODO: add exception for length timers also
        printf("APU is off, apu_write($%02X, %02X) ignored!\n", address, value);
        return;
    }

    if (BETWEEN(address, 0xFF10, 0xFF19)) { // pulse channels
        //printf("Pulse Channel Write\n");
        pulse_channel_write(address, value);
        return;
    }
    if (BETWEEN(address, 0xFF1A, 0xFF1E)) { // wave channel
        //printf("Wave Channel Write\n");
        wave_channel_write(address, value);
        return;
    }
    if (BETWEEN(address, 0xFF20, 0xFF23)) { // noise channel
        //printf("Noise Channel Write\n");
        noise_channel_write(address, value);
        return;
    }
    if (BETWEEN(address, 0xFF24, 0xFF26)) { // global control regs
        //printf("Global Regs Write\n");
        global_control_regs_write(address, value);
        return;
    }
    printf("Invalid apu_write($%2X, %2X)\n", address, value);
}

// returns -1 to 1 analog value
static float dac(channel* chan, u8 amplitude) {
    if (!chan->dac_enable) {
        return 0;
    }
    
    float volume = amplitude * chan->env_volume;
    if (chan == &apu_ctx.channels[2]) { // wave channel
        volume = amplitude >> chan->env_volume;
    }
    return volume / 7.5 - 1;
}

// adds all of the channel levels and sums them together into left and right
// modifies the values of mixer_out
// [0] = left, [1] = right
static void mixer() {
    apu_ctx.mixer_out[0] = 0;
    apu_ctx.mixer_out[1] = 0;

    for (int i = 0; i < 4; i++) {
        if (apu_ctx.channels[i].mute) {
            continue;
        }
        if (apu_ctx.channels[i].left_enable) {
            apu_ctx.mixer_out[0] += apu_ctx.channels[i].level;
        }
        if (apu_ctx.channels[i].right_enable) {
            apu_ctx.mixer_out[1] += apu_ctx.channels[i].level;
        }
    }

    apu_ctx.mixer_out[0] /= 4;
    apu_ctx.mixer_out[1] /= 4;
}

// volume scaler (amplifier)
// once from nr50 (never mutes non-silent input)
// once from volume knob (this can tho)
// after this, they go through hpf
static void volume(float vol_mult) {
    i8 left_vol = apu_ctx.left_volume; // from nr50
    i8 right_vol = apu_ctx.right_volume; // from nr50

    if (left_vol == 0) {
        left_vol = 1;
    } else if (left_vol == 7) {
        left_vol = 8;
    }

    if (right_vol == 0) {
        right_vol = 1;
    } else if (right_vol == 7) {
        right_vol = 8;
    }

    apu_ctx.volume_out[0] = apu_ctx.mixer_out[0] * left_vol * vol_mult;
    apu_ctx.volume_out[1] = apu_ctx.mixer_out[1] * right_vol * vol_mult;
}

// high pass filter
// FIX: i think this might be fucked
static float hpf(float volume_output, u8 index) {
    i16 out = 0;

    if (apu_ctx.hpf_enabled) {
        out = (volume_output - apu_ctx.capacitors[index]);

        apu_ctx.capacitors[index] = volume_output - out * apu_ctx.capacitor_factor;
    } else {
        out = volume_output;
    }

    return out;
}

static void set_sound_buffer() {
    if (apu_ctx.sound_buf->sample_index >= apu_ctx.sound_buf->samples*2) {
        apu_queue_audio();
        apu_ctx.sound_buf->sample_index = 0;
    }

    apu_ctx.sound_buf->data[apu_ctx.sound_buf->sample_index++] = apu_ctx.volume_out[0];
    apu_ctx.sound_buf->data[apu_ctx.sound_buf->sample_index++] = apu_ctx.volume_out[1];
}

static void prepare_output_buffer() {
    mixer();
    volume(.01);
    set_sound_buffer();
}

void apu_queue_audio() {
    assert(apu_ctx.sound_buf->data != NULL);
    audio_play(apu_ctx.sound_buf->data, apu_ctx.sound_buf->bytes);
    memset(apu_ctx.sound_buf->data, 0, apu_ctx.sound_buf->bytes);
}

static u8 get_nibble(u8 pos) {
    u8 index = pos / 2;
    u8 shift = (pos % 2) ? 4 : 0;
    return (apu_read(0xFF30 + index) >> shift) & 0xF;
}

void apu_tick() {
    apu_ctx.div_apu_hz_counter--;

    if (apu_ctx.div_apu_hz_counter <= 0) {
        apu_ctx.div_apu_hz_counter = apu_ctx.div_apu_hz_tc;
        div_apu_clock();
    }

    if (!apu_ctx.apu_on) {
        return;
    }


    for (int i = 0; i < 2; i++) {
        channel* pulse_channel = &apu_ctx.channels[i];

        if (!pulse_channel->active) {
            continue;
        }

        i8 amplitude = (pulse_channel->duty_val >> pulse_channel->duty_step_counter) & 1;
        pulse_channel->level = 0;//dac(pulse_channel, amplitude);

        pulse_channel->period_div_counter--;

        if (pulse_channel->period_div_counter <= 0) {
            pulse_channel->period_div_counter = pulse_channel->period_div_tc * 4;

            pulse_channel->duty_step_counter++;
            pulse_channel->duty_step_counter &= 7; // % 8
        }
    }


    channel* wave_channel = &apu_ctx.channels[2];

    if (wave_channel->active) {
        wave_channel->period_div_counter--;

        if (wave_channel->period_div_counter <= 0) {
            wave_channel->period_div_counter = wave_channel->period_div_tc * 2;

            apu_ctx.ch3_pos++;
            apu_ctx.ch3_pos %= 32;
            apu_ctx.ch3_next_nibble = get_nibble(apu_ctx.ch3_pos);

            wave_channel->level = dac(wave_channel, apu_ctx.ch3_next_nibble);
        }
    }


    channel* noise_channel = &apu_ctx.channels[3];

    if (noise_channel->active) {
        noise_channel->period_div_counter--;

        if (noise_channel->period_div_counter <= 0) {
            //noise_channel->period_div_tc = (apu_ctx.clock_divider > 0 ? (apu_ctx.clock_divider << 4) : 8) << apu_ctx.clock_shift;
            noise_channel->period_div_counter = (apu_ctx.clock_divider > 0 ? (apu_ctx.clock_divider << 4) : 8) << apu_ctx.clock_shift; //noise_channel->period_div_tc;

            noise_channel->level = dac(noise_channel, ~lfsr_next_value() & 1);
        }
    }


    apu_ctx.sound_div_counter--;

    if (apu_ctx.sound_div_counter <= 0) {
        apu_ctx.sound_div_counter = apu_ctx.sound_div_tc;
        prepare_output_buffer();
    }
}
