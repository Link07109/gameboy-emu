#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <time.h>

#include "../include/ui.h"
#include "../include/emu.h"
#include "../include/bus.h"
#include "../include/ppu.h"
#include "../include/gamepad.h"
#include "../include/apu.h"

// graphics
SDL_Window* sdl_window;
SDL_Renderer* sdl_renderer;
SDL_Texture* sdl_texture;
SDL_Surface* screen;

SDL_Window* sdl_debug_window;
SDL_Renderer* sdl_debug_renderer;
SDL_Texture* sdl_debug_texture;
SDL_Surface* debug_screen;

static int scale = 5;
static int debug_scale = 4;

// audio
SDL_AudioSpec obtained;
int device;
buffer hw_buf;

void delay(u32 ms) {
    SDL_Delay(ms);
}

u32 get_ticks() {
    return SDL_GetTicks();
}

void audio_play(void* buf, u32 count) {
    int overqueued = SDL_GetQueuedAudioSize(device) - obtained.size;
    float delay_nanos = (float)overqueued / 4.0 / obtained.freq * 1000000000.0; // 1billion nano = 1 second

    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = (long)delay_nanos
    };

    if (overqueued > 0) {
        //nanosleep(&interval, NULL);
        //SDL_ClearQueuedAudio(device);
        return;
    }

    if (SDL_QueueAudio(device, buf, count) != 0) {
        fprintf(stderr, "Could not write SDL audio data: %s\n", SDL_GetError());
        exit(-9);
    }
}

void audio_init() {
    SDL_Init(SDL_INIT_AUDIO);
    printf("AUDIO INIT\n");

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = NULL;
    desired.format = AUDIO_F32SYS;

    device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (device == 0) {
        fprintf(stderr, "Could Not Open Audio Device: %s\n", SDL_GetError());
        exit(-9);
    }

    SDL_PauseAudioDevice(device, 0);
    printf("== audio stats:\n");
    printf("freq: %d\n", obtained.freq);
    printf("samples: %d\n", obtained.samples);
    printf("size: %d\n", obtained.size);
    printf("format: %hu\n", obtained.format);

    int calc_bytes = obtained.samples * sizeof(float) * obtained.channels;
    printf("calc bytes: %d\n", calc_bytes);
    assert(obtained.size == calc_bytes);
    assert(obtained.samples == 1024);

    hw_buf.samples = obtained.samples;
    hw_buf.bytes = obtained.size;
    hw_buf.sample_index = 0;
    apu_get_context()->sound_buf = &hw_buf;
}

void ui_init() {
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");
    audio_init();

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdl_window, &sdl_renderer);
    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    /*
    SDL_CreateWindowAndRenderer(DEBUG_SCREEN_WIDTH, DEBUG_SCREEN_HEIGHT, 0, &sdl_debug_window, &sdl_debug_renderer);
    debug_screen = SDL_CreateRGBSurface(0, DEBUG_SCREEN_WIDTH + (16*debug_scale), DEBUG_SCREEN_HEIGHT + (16*debug_scale), 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    sdl_debug_texture = SDL_CreateTexture(sdl_debug_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DEBUG_SCREEN_WIDTH + (16*debug_scale), DEBUG_SCREEN_HEIGHT + (16*debug_scale));
    
    int x, y;
    SDL_GetWindowPosition(sdl_window, &x, &y);
    SDL_SetWindowPosition(sdl_debug_window, x + SCREEN_WIDTH + 10, y);
    */
}

static unsigned long tile_colors[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

void display_tile(SDL_Surface* surface, u16 start_location, u16 tile_num, int x, int y) {
    SDL_Rect rc;

    for (int tile_y = 0; tile_y < 16; tile_y += 2) {
        u8 b1 = bus_read(start_location + (tile_num * 16) + tile_y);
        u8 b2 = bus_read(start_location + (tile_num * 16) + tile_y + 1);

        for (int bit = 7; bit >= 0; bit--) {
            u8 hi = !!(b1 & (1 << bit)) << 1;
            u8 lo = !!(b2 & (1 << bit));

            u8 color = hi | lo;

            rc.x = x + ((7 - bit) * debug_scale);
            rc.y = y + (tile_y / 2 * debug_scale);
            rc.w = debug_scale;
            rc.h = debug_scale;

            SDL_FillRect(surface, &rc, tile_colors[color]);
        }
    }
}

void update_debug_window() {
    int x_draw = 0;
    int y_draw = 0;
    int tile_num = 0;
    
    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = debug_screen->w;
    rc.h = debug_screen->h;
    SDL_FillRect(debug_screen, &rc, 0xFF111111);

    u16 addr = 0x8000;

    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 16; x++) {
            display_tile(debug_screen, addr, tile_num, x_draw + (x * debug_scale), y_draw + (y * debug_scale));
            x_draw += 8 * debug_scale;
            tile_num++;
        }

        y_draw += 8 * debug_scale;
        x_draw = 0;
    }

    SDL_UpdateTexture(sdl_debug_texture, NULL, debug_screen->pixels, debug_screen->pitch);
    SDL_RenderClear(sdl_debug_renderer);
    SDL_RenderCopy(sdl_debug_renderer, sdl_debug_texture, NULL, NULL);
    SDL_RenderPresent(sdl_debug_renderer);
}

void ui_update() {
    SDL_Rect rc;
    rc.x = rc.y = 0;
    rc.w = rc.h = 2048;

    u32* video_buffer = ppu_get_context()->video_buffer;

    for (int line_num = 0; line_num < YRES; line_num++) {
        for (int x = 0; x < XRES; x++) {
            rc.x = x * scale;
            rc.y = line_num * scale;
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(screen, &rc, video_buffer[x + (line_num * XRES)]);
        }
    }


    SDL_UpdateTexture(sdl_texture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdl_renderer);
    SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);

    //update_debug_window();
}

void ui_on_key(bool down, u32 key_code) {
    switch (key_code) {
        case SDLK_j: gamepad_get_state()->a = down; break;
        case SDLK_k: gamepad_get_state()->b = down; break;
        case SDLK_RETURN: gamepad_get_state()->start = down; break;
        case SDLK_BACKSPACE: gamepad_get_state()->select = down; break;
        case SDLK_w: gamepad_get_state()->up = down; break;
        case SDLK_s: gamepad_get_state()->down = down; break;
        case SDLK_a: gamepad_get_state()->left = down; break;
        case SDLK_d: gamepad_get_state()->right = down; break;

        case SDLK_SPACE: 
            printf("play sound key\n");
            SDL_PauseAudioDevice(device, 0);
            break;
        case SDLK_p:
            printf("pause sound key\n");
            SDL_PauseAudioDevice(device, 1);
            break;
    }
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
        if (e.type == SDL_KEYDOWN) {
            ui_on_key(true, e.key.keysym.sym);
        }
        if (e.type == SDL_KEYUP) {
            ui_on_key(false, e.key.keysym.sym);
        }

        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            emu_get_context()->die = true;
        }
    }
}
