#include "../include/cart.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    char filename[1024];
    u32 rom_size;
    u8* rom_data;
    rom_header* header;

    // MBC1 data
    u8* rom_bank_x;
    u8 banking_mode;

    u8 rom_bank_value;
    u8 ram_bank_value;

    bool ram_enabled;
    bool ram_banking;

    u8* ram_bank; // currently selected
    u8* ram_banks[16]; // all of em

    // for battery
    bool battery;
    bool need_save; // save battery backup
} cart_context;

static cart_context cart_ctx;

bool cart_need_save() {
    return cart_ctx.need_save;
}

bool cart_mbc1() {
    return BETWEEN(cart_ctx.header->type, 1, 3);
}

bool cart_battery() {
    // mbc1 only for now
    return cart_ctx.header->type == 3;
}

static const char* ROM_TYPES[] = {
    "ROM ONLY",
    "MBC1",
    "MBC1+RAM",
    "MBC1+RAM+BATTERY",
    "0x04 ???",
    "MBC2",
    "MBC2+BATTERY",
    "0x07 ???",
    "ROM+RAM 1",
    "ROM+RAM_BATTERY 1",
    "0x0A ???",
    "MMM01",
    "MMM01+RAM",
    "MMM01+RAM+BATTERY",
    "0x0E ???",
    "MBC3+TIMER+BATTERY",
    "MBC3+TIMER+RAM+BATTERY 2",
    "MBC3",
    "MBC3+RAM 2",
    "MBC3+RAM+BATTERY 2",
    "0x14 ???",
    "0x15 ???",
    "0x16 ???",
    "0x17 ???",
    "0x18 ???",
    "MBC5",
    "MBC5+RAM",
    "MBC5+RAM+BATTERY",
    "MBC5+RUMBLE",
    "MBC5+RUMBLE+RAM",
    "MBC5+RUMBLE+RAM+BATTERY",
    "0x1F ???",
    "MBC6",
    "0x21 ???",
    "MBC7+SENSOR+RUMBLE+RAM+BATTERY",

    //"POCKET CAMERA",
    //"BANDAI TAMA5",
    //"HuC3",
    //"HuC1_RAM_BATTERY",
};

// new licensee codes
static const char* LIC_CODE[0xA5] = {
    [0x00] = "None",
    [0x01] = "Nintendo Research & Development 1",
    [0x08] = "Capcom",
    [0x13] = "EA (Electronic Arts)",
    [0x18] = "Hudson Soft",
    [0x19] = "B-AI",
    [0x20] = "KSS",
    [0x22] = "Planning Office WADA",
    [0x24] = "PCM Complete",
    [0x25] = "San-X",
    [0x28] = "Kemco",
    [0x29] = "SETA Corporation",
    [0x30] = "Viacom",
    [0x31] = "Nintendo",
    [0x32] = "Bandai",
    [0x33] = "Ocean Software/Acclaim Entertainment",
    [0x34] = "Konami",
    [0x35] = "HectorSoft",
    [0x37] = "Taito",
    [0x38] = "Hudson Soft",
    [0x39] = "Banpresto",
    [0x41] = "Ubi Soft1",
    [0x42] = "Atlus",
    [0x44] = "Malibu Interactive",
    [0x46] = "Angel",
    [0x47] = "Bullet-Proof Software2",
    [0x49] = "Irem",
    [0x50] = "Absolute",
    [0x51] = "Acclaim Entertainment",
    [0x52] = "Activision",
    [0x53] = "Sammy USA Corporation",
    [0x54] = "Konami",
    [0x55] = "Hi Tech Expressions",
    [0x56] = "LJN",
    [0x57] = "Matchbox",
    [0x58] = "Mattel",
    [0x59] = "Milton Bradley Company",
    [0x60] = "Titus Interactive",
    [0x61] = "Virgin Games Ltd.3",
    [0x64] = "Lucasfilm Games4",
    [0x67] = "Ocean",
    [0x69] = "EA (Electronic Arts)",
    [0x70] = "Infogrames5",
    [0x71] = "Interplay Entertainment",
    [0x72] = "Broderbund",
    [0x73] = "Sculptured Software6",
    [0x75] = "The Sales Curve Limited7",
    [0x78] = "THQ",
    [0x79] = "Accolade",
    [0x80] = "Misawa Entertainment",
    [0x83] = "lozc",
    [0x86] = "Tokuma Shoten",
    [0x87] = "Tsukada Original",
    [0x91] = "Chunsoft Co.8",
    [0x92] = "Video System",
    [0x93] = "Ocean Software/Acclaim Entertainment",
    [0x95] = "Varie",
    [0x96] = "Yonezawa/s'pal",
    [0x97] = "Keneko",
    [0x99] = "Pack-In-Video",
    //[0x9H] = "Bottom Up",
    [0xA4] = "Konami (Yu-Gi-Oh!)",
    //[0xBL] = "MTO",
    //[0xDK] = "Kodansha",
};

// we need to account for the old license names too you know...
const char* cart_lic_name() {
    if (cart_ctx.header->new_lic_code <= 0xA4) {
        return LIC_CODE[cart_ctx.header->lic_code];
    }
    return "UNKNOWN";
}

const char* cart_type_name() {
    if (cart_ctx.header->type <= 0x22) {
        return ROM_TYPES[cart_ctx.header->type];
    }
    return "UNKNOWN";
}

void cart_setup_banking() {
    for (int i = 0; i < 16; i++) {
        cart_ctx.ram_banks[i] = 0;

        if ((cart_ctx.header->ram_size == 2 && i == 0) ||
                (cart_ctx.header->ram_size == 3 && i < 4) ||
                (cart_ctx.header->ram_size == 4 && i < 16) ||
                (cart_ctx.header->ram_size == 5 && i < 8)) {
            cart_ctx.ram_banks[i] = malloc(0x2000);
            memset(cart_ctx.ram_banks[i], 0, 0x2000);
        }
    }

    cart_ctx.ram_bank = cart_ctx.ram_banks[0];
    cart_ctx.rom_bank_x = cart_ctx.rom_data + 0x4000; // rom bank 1
}

void cart_battery_load() {
    char fn[1048];
    sprintf(fn, "%s.battery", cart_ctx.filename);
    FILE* fp = fopen(fn, "rb");

    if (!fp) {
        fprintf(stderr, "FAILED TO OPEN: %s\n", fn);
        return;
    }

    fread(cart_ctx.ram_bank, 0x2000, 1, fp);
    fclose(fp);
}

void cart_battery_save() {
    char fn[1048];
    sprintf(fn, "%s.battery", cart_ctx.filename);
    FILE* fp = fopen(fn, "wb");

    if (!fp) {
        fprintf(stderr, "FAILED TO OPEN: %s\n", fn);
        return;
    }

    fwrite(cart_ctx.ram_bank, 0x2000, 1, fp);
    fclose(fp);
}

void print_cart_info(cart_context ctx) {
    printf("\t Title    : %s\n", ctx.header->title);
    printf("\t Type     : %2.2X (%s)\n", ctx.header->type, cart_type_name());
    printf("\t ROM Size : %d KB\n", 32 << ctx.header->rom_size);
    printf("\t RAM Size : %2.2X\n", ctx.header->ram_size);
    printf("\t LIC Code : %2.2X (%s)\n", ctx.header->lic_code, cart_lic_name());
    printf("\t ROM Vers : %2.2X\n", ctx.header->version);
}

bool cart_load(char* cart) {
    snprintf(cart_ctx.filename, sizeof(cart_ctx.filename), "%s", cart);

    FILE* fp = fopen(cart, "r");

    if (!fp) {
        printf("Failed to to open: %s\n", cart);
        return false;
    }

    printf("Opened: %s\n", cart_ctx.filename);

    fseek(fp, 0, SEEK_END);
    cart_ctx.rom_size = ftell(fp);
    rewind(fp);

    cart_ctx.rom_data = malloc(cart_ctx.rom_size);
    fread(cart_ctx.rom_data, cart_ctx.rom_size, 1, fp);
    fclose(fp);

    cart_ctx.header = (rom_header*)(cart_ctx.rom_data + 0x100);
    cart_ctx.header->title[15] = 0; // null terminate string just in case
    cart_ctx.battery = cart_battery();
    cart_ctx.need_save = false;

    printf("Cartridge Loaded:\n");
    print_cart_info(cart_ctx);

    cart_setup_banking();

    u16 checksum = 0;
    for (u16 address = 0x0134; address <= 0x014C; address++) {
        checksum -= cart_ctx.rom_data[address] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", cart_ctx.header->checksum, (checksum & 0xFF) ? "PASSED" : "FAILED");

    if (cart_ctx.battery) {
        cart_battery_load();
    }

    return true;
}

u8 cart_read(u16 address) {
    if (!cart_mbc1() || address < 0x4000) {
        return cart_ctx.rom_data[address];
    }

    if ((address & 0xE000) == 0xA000) {
        if (!cart_ctx.ram_enabled) { // error!
            return 0xFF;
        }
        if (!cart_ctx.ram_bank) { // error!
            return 0xFF;
        }

        return cart_ctx.ram_bank[address - 0xA000];
    }
    
    return cart_ctx.rom_bank_x[address - 0x4000];
}

void cart_write(u16 address, u8 value) {
    if (!cart_mbc1()) { // error!
        return;
    }

    if (address < 0x2000) {
        cart_ctx.ram_enabled = ((value & 0xF) == 0xA);
    }

    if ((address & 0xE000) == 0x2000) { // rom bank number
        if (value == 0) {
            value = 1;
        }

        value &= 0b11111; // only last 5 bits used

        cart_ctx.rom_bank_value = value;
        cart_ctx.rom_bank_x = cart_ctx.rom_data + (0x4000 * cart_ctx.rom_bank_value);
    }

    if ((address & 0xE000) == 0x4000) { // ram bank number
        cart_ctx.ram_bank_value = value & 0b11;

        if (cart_ctx.ram_banking) {
            if (cart_need_save()) {
                cart_battery_save();
            }
            cart_ctx.ram_bank = cart_ctx.ram_banks[cart_ctx.ram_bank_value];
        }
    }

    if ((address & 0xE000) == 0x6000) { // banking mode select
        cart_ctx.banking_mode = value & 1;
        cart_ctx.ram_banking = cart_ctx.banking_mode;

        if (cart_ctx.ram_banking) {
            if (cart_need_save()) {
                cart_battery_save();
            }
            cart_ctx.ram_bank = cart_ctx.ram_banks[cart_ctx.ram_bank_value];
        }
    }

    if ((address & 0xE000) == 0xA000) { // memory region
        if (!cart_ctx.ram_enabled) {
            return;
        }

        if (!cart_ctx.ram_bank) {
            return;
        }

        cart_ctx.ram_bank[address - 0xA000] = value;

        if (cart_ctx.battery) {
            cart_ctx.need_save = true;
        }
    }
}
