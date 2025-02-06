#include "../include/io.h"
#include "../include/timer.h"
#include "../include/cpu.h"
#include "../include/lcd.h"
#include "../include/gamepad.h"
#include "../include/apu.h"

static char serial_data[2];

u8 io_read(u16 address) {
    if (address == 0xFF00) { // Joypad
        return gamepad_get_output();
    }
    if (address == 0xFF01) { // SB - Serial Transfer Data (R/W)
        return serial_data[0];
    }
    if (address == 0xFF02) { // SC - Serial Transfer Control (R/W)
        return serial_data[1];
    }
    if (BETWEEN(address, 0xFF04, 0xFF07)) { // Timer
        return timer_read(address);
    }
    if (address == 0xFF0F) { // Interrupt Flags
        return cpu_get_int_flags();
    }
    if (BETWEEN(address, 0xFF10, 0xFF3F)) { // Audio
        return apu_read(address);
    }
    if (BETWEEN(address, 0xFF40, 0xFF4B)) { // LCD
        return lcd_read(address);
    }

    return 0;
}

void io_write(u16 address, u8 val) {
    if (address == 0xFF00) { // Joypad
        gamepad_set_selected(val);
        return;
    }
    if (address == 0xFF01) { // SB - Serial Transfer Data (R/W)
        serial_data[0] = val;
        return;
    }
    if (address == 0xFF02) { // SC - Serial Transfer Control (R/W)
        serial_data[1] =  val;
        return;
    }
    if (BETWEEN(address, 0xFF04, 0xFF07)) { // Timer
        timer_write(address, val);
        return;
    }
    if (address == 0xFF0F) { // Interrupt Flags
        cpu_set_int_flags(val);
        return;
    }
    if (BETWEEN(address, 0xFF10, 0xFF3F)) { // Audio
        apu_write(address, val);
        return;
    }
    if (BETWEEN(address, 0xFF40, 0xFF4B)) { // LCD
        lcd_write(address, val);
        return;
    }
}
