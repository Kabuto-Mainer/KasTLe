#ifndef GFX_H
#define GFX_H

#include <stdint.h>

int32_t     gfx_init(int32_t w, int32_t h, int32_t scale);
void        gfx_close(void);
int32_t     gfx_get_color(uint8_t r, uint8_t g, uint8_t b);
int32_t     gfx_get_color_shadow(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void        gfx_clear(int32_t color);
void        gfx_pixel(int32_t x, int32_t y, int32_t color);
void        gfx_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void        gfx_present(void);
void        gfx_delay(int32_t ms);
int32_t     gfx_poll_event(void);
int32_t     gfx_key_pressed(int32_t scancode);
int32_t     gfx_mouse_x(void);
int32_t     gfx_mouse_y(void);
int32_t     gfx_mouse_pressed(void);


#endif /* GFX_H */
