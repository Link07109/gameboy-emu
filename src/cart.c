#include "../include/cart.h"

typedef struct {
    char filename[1024];
    u32 rom_size;
    u8* rom_data;
    rom_header* header;
} cart_context;

static cart_context cart_ctx;

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

    printf("Cartridge Loaded:\n");
    print_cart_info(cart_ctx);

    u16 checksum = 0;
    for (u16 address = 0x0134; address <= 0x014C; address++) {
        checksum -= cart_ctx.rom_data[address] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", cart_ctx.header->checksum, (checksum & 0xFF) ? "PASSED" : "FAILED");

    return true;
}

// for now just ROM ONLY supported ( so no zelda :c )
u8 cart_read(u16 address) {
    return cart_ctx.rom_data[address];
}

void cart_write(u16 address, u8 value) {
    printf("UNSUPPORTED cart_write(%04X)\n", address);
    //NO_IMPL
}
