#include "../include/bus.h"
#include "../include/cart.h"
#include "../include/ram.h"
#include "../include/cpu.h"
#include "../include/io.h"
#include "../include/ppu.h"
#include "../include/dma.h"

u8 bus_read(u16 address) {
    if (address < 0x8000) { // ROM
        return cart_read(address);
    } else if (address < 0xA000) { // CHR RAM / BG Map (VRAM)
        return ppu_vram_read(address);
    } else if (address < 0xC000) { // Cartridge RAM
        return cart_read(address);
    } else if (address < 0xE000) { // WRAM
        return wram_read(address);
    } else if (address < 0xFE00) { // Echo RAM
        return 0;
    } else if (address < 0xFEA0) { // OAM
        if (dma_transferring()) {
            return 0xFF;
        }
        return ppu_oam_read(address);
    } else if (address < 0xFF00) { // Reserved
        return 0;
    } else if (address < 0xFF80) { // I/O Registers
        return io_read(address);
    } else if (address == 0xFFFF) { // Interrupt Enable Register
        return cpu_get_ie_reg();
    }

    return hram_read(address); // High RAM (Zero Page)
}

void bus_write(u16 address, u8 value) {
    if (address < 0x8000) { // ROM
        cart_write(address, value);
    } else if (address < 0xA000) { // CHR RAM / BG Map (VRAM)
        ppu_vram_write(address, value);
    } else if (address < 0xC000) { // Cartridge RAM (EXT-RAM)
        cart_write(address, value);
    } else if (address < 0xE000) { // WRAM
        wram_write(address, value);
    } else if (address < 0xFE00) { // Echo RAM
        // literally do nothing lmao
    } else if (address < 0xFEA0) { // OAM
        if (dma_transferring()) {
            return;
        }
        ppu_oam_write(address, value);
    } else if (address < 0xFF00) { // Reserved
        // literally do nothing lmao
    } else if (address < 0xFF80) { // I/O Registers
        io_write(address, value);
    } else if (address == 0xFFFF) { // Interrupt Enable Register
        cpu_set_ie_reg(value);
    } else hram_write(address, value); // High RAM (Zero Page)
}

u16 bus_read16(u16 address) {
	u16 lo = bus_read(address);
	u16 hi = bus_read(address + 1);

	return lo | (hi << 8);
}

void bus_write16(u16 address, u16 value) {
	bus_write(address + 1, (value >> 8) & 0xFF);
	bus_write(address, value & 0xFF);
}
