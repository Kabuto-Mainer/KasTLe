#include <SDL2/SDL.h>
#include <stdint.h>

#include "gfx.h"

static SDL_Window   *g_win    = NULL;
static SDL_Renderer *g_ren    = NULL;
static int           g_scale  = 1;
static int           g_w      = 0;
static int           g_h      = 0;

static int           g_quit         = 0;
static int           g_last_key     = 0;
static int           g_mouse_x      = 0;
static int           g_mouse_y      = 0;
static int           g_mouse_pressed = 0;


int32_t gfx_init(int32_t w, int32_t h, int32_t scale) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)   return -1;

    g_scale = (scale > 0) ? scale : 1;
    g_w = w;
    g_h = h;

    g_win = SDL_CreateWindow(
        "KasTLe Graphics",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w * g_scale, h * g_scale,
        SDL_WINDOW_SHOWN
    );
    if (g_win == NULL) {
        SDL_Quit();
        return -1;
    }

    g_ren = SDL_CreateRenderer(g_win, -1,
                               SDL_RENDERER_ACCELERATED |
                               SDL_RENDERER_PRESENTVSYNC);
    if (g_ren == NULL) {
        SDL_DestroyWindow(g_win);
        SDL_Quit();
        return -1;
    }
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 0);
    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);

    return 0;
}

void gfx_close(void) {
    if (g_ren) SDL_DestroyRenderer(g_ren);
    if (g_win) SDL_DestroyWindow(g_win);
    SDL_Quit();
    g_ren = NULL;
    g_win = NULL;
}

static void set_color(int32_t color) {
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >>  8) & 0xFF;
    uint8_t b = (color >>  0) & 0xFF;
    SDL_SetRenderDrawColor(g_ren, r, g, b, a);
}

int32_t gfx_get_color(uint8_t r, uint8_t g, uint8_t b) {
    int32_t color = b & 0xFF;
    color |= (g & 0xFF) << 8;
    color |= (r & 0xFF) << 16;
    color |= 0xFF << 24;
    return color;
}

int32_t gfx_get_color_shadow(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    int32_t color = b & 0xFF;
    color |= (g & 0xFF) << 8;
    color |= (r & 0xFF) << 16;
    color |= (a & 0xFF) << 24;
    return color;
}

void gfx_clear(int32_t color) {
    if (g_ren == NULL) return;
    set_color(color);
    SDL_RenderClear(g_ren);
}

void gfx_pixel(int32_t x, int32_t y, int32_t color) {
    if (g_ren == NULL) return;
    set_color(color);

    SDL_Rect r = {};
    r.x = x * g_scale;
    r.y = y * g_scale;
    r.w = g_scale;
    r.h = g_scale;
    SDL_RenderFillRect(g_ren, &r);
}

void gfx_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
    if (g_ren == NULL) return;
    set_color(color);

    SDL_Rect r = {};
    r.x = x * g_scale;
    r.y = y * g_scale;
    r.w = w * g_scale;
    r.h = h * g_scale;
    SDL_RenderFillRect(g_ren, &r);
}

void gfx_present(void) {
    if (g_ren == NULL) return ;
    SDL_RenderPresent(g_ren);
}

void gfx_delay(int32_t ms) {
    SDL_Delay((uint32_t) ms);
}

int32_t gfx_poll_event(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            g_quit = 1;
            break;
        case SDL_KEYDOWN:
            g_last_key = ev.key.keysym.sym;
            break;
        case SDL_MOUSEMOTION:
            g_mouse_x = ev.motion.x / g_scale;
            g_mouse_y = ev.motion.y / g_scale;
            break;
        case SDL_MOUSEBUTTONDOWN:
            g_mouse_pressed = 1;
            break;
        case SDL_MOUSEBUTTONUP:
            g_mouse_pressed = 0;
            break;
        }
    }
    return g_quit;
}

int32_t gfx_key_pressed(int32_t scancode) {
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state == NULL) return 0;
    return state[scancode] ? 1 : 0;
}

int32_t gfx_mouse_x(void)       { return g_mouse_x; }
int32_t gfx_mouse_y(void)       { return g_mouse_y; }
int32_t gfx_mouse_pressed(void) { return g_mouse_pressed; }


