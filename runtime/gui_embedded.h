#pragma once
/* Generated from minigui.h + ws_gui.h. Do not edit by hand. */
static const char *WS_GUI_SOURCE = R"WS_GUI_RUNTIME(
#define MINIGUI_IMPLEMENTATION
/*
 * minigui.h - a tiny cross-platform GUI library for C and C++.
 *
 * One header. No external runtime files, DLLs, Qt, SDL, or GTK.
 * Windows: Win32 (GDI). Linux/X11: Xlib (also works on Wayland via XWayland).
 *
 * USAGE:
 *   #define MINIGUI_IMPLEMENTATION   // in exactly ONE .c file
 *   #include "minigui.h"
 *
 *   static void draw(MgWindow *w, void *user) {
 *       (void)user;
 *       mg_label(w, 20, 20, "Hello!");
 *       if (mg_button(w, 20, 50, 140, 34, "Press me")) { ... }
 *   }
 *
 *   int main(void) {
 *       mg_set_font(2, MG_RGB(255, 220, 150));  -- size and color
 *       return mg_run("Hello", 640, 480, draw, NULL);
 *   }
 *
 *   If you need full control, write your own loop:
 *   mg_create() -> while (mg_open()) { mg_begin(); ...; mg_end(); mg_sleep(16); }
 *   -> mg_destroy().
 *
 * LINUX:   gcc demo.c -o demo -lX11
 * WINDOWS: gcc demo.c -o demo.exe -lgdi32 -luser32
 *
 * License: do whatever you want; the author doesn't mind.
 */

#ifndef MINIGUI_H
#define MINIGUI_H

/* Needed for nanosleep/clock_gettime with strict -std=c11 -pedantic. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int MgColor;
typedef struct MgWindow MgWindow;

#define MG_RGB(r, g, b) (0xFF000000u | ((unsigned int)(r) << 16) | ((unsigned int)(g) << 8) | (unsigned int)(b))

/* Special key codes (all others are ordinary ASCII codes, lowercase letters) */
#define MG_KEY_BACK   8
#define MG_KEY_ENTER  13
#define MG_KEY_ESC    27
#define MG_KEY_LEFT   128
#define MG_KEY_RIGHT  129
#define MG_KEY_UP     130
#define MG_KEY_DOWN   131
#define MG_KEY_DELETE 132
#define MG_KEY_SHIFT  133
#define MG_KEY_CTRL   134

/* --- window / frame --- */
MgWindow *mg_create(const char *title, int width, int height);
void      mg_destroy(MgWindow *w);
int       mg_open(MgWindow *w);
void      mg_begin(MgWindow *w);
void      mg_end(MgWindow *w);

/* The simplest path: one function instead of create/loop/clear/destroy.
 * drawing(w, user) is called every frame. */
int mg_run(const char *title, int width, int height,
           void (*drawing)(MgWindow *w, void *user), void *user);
void      mg_sleep(int milliseconds);
unsigned int mg_time_ms(void);
void      mg_quit(MgWindow *w);   /* close the window from any function */

/* --- font: size and color --- */
void      mg_set_font(int size, MgColor color);   /* size 1..8 and color in one call */
void      mg_set_font_size(int size);             /* 1 = 8x12, 2 = 16x24, ... */
int       mg_get_font_size(void);
void      mg_set_font_color(MgColor color);
MgColor   mg_get_font_color(void);

void      mg_set_theme(MgColor background, MgColor panel, MgColor text, MgColor accent);
void      mg_set_title(MgWindow *w, const char *title);
void      mg_set_pos(MgWindow *w, int x, int y);
void      mg_set_size(MgWindow *w, int width, int height);
int       mg_width(MgWindow *w);
int       mg_height(MgWindow *w);
void      mg_fullscreen(MgWindow *w, int on);
int       mg_is_fullscreen(MgWindow *w);
void      mg_set_text_scale(int scale);   /* alias for mg_set_font_size */
int       mg_get_text_scale(void);        /* alias for mg_get_font_size */

/* --- drawing --- */
void mg_clear(MgWindow *w, MgColor c);
void mg_pixel(MgWindow *w, int x, int y, MgColor c);
void mg_rect(MgWindow *w, int x, int y, int rw, int rh, MgColor c);
void mg_rect_border(MgWindow *w, int x, int y, int rw, int rh, MgColor c, int thickness);
void mg_line(MgWindow *w, int x0, int y0, int x1, int y1, MgColor c);
void mg_circle(MgWindow *w, int cx, int cy, int r, MgColor c);
void mg_text(MgWindow *w, int x, int y, const char *text, MgColor c, int size);
                              /* size = 0 uses the global font size */
int  mg_text_width(const char *text, int size);
void mg_textf(MgWindow *w, int x, int y, MgColor c, int size, const char *fmt, ...);
void mg_image(MgWindow *w, int x, int y, int iw, int ih, const MgColor *pixels);

/* --- input --- */
int         mg_mouse_x(MgWindow *w);
int         mg_mouse_y(MgWindow *w);
int         mg_mouse_down(MgWindow *w);
int         mg_mouse_pressed(MgWindow *w);
int         mg_key_down(MgWindow *w, int code);
int         mg_key_pressed(MgWindow *w, int code);
const char *mg_typed(MgWindow *w);

/* --- widgets --- */
void mg_panel(MgWindow *w, int x, int y, int rw, int rh);
void mg_label(MgWindow *w, int x, int y, const char *text);
void mg_label_ex(MgWindow *w, int x, int y, const char *text, MgColor c, int size);
void mg_labelf(MgWindow *w, int x, int y, const char *fmt, ...);   /* like printf */
int  mg_button(MgWindow *w, int x, int y, int rw, int rh, const char *label);
int  mg_checkbox(MgWindow *w, int x, int y, const char *label, int *checked);
int  mg_textfield(MgWindow *w, int x, int y, int rw, int rh, char *buffer, int capacity);
int  mg_slider(MgWindow *w, int x, int y, int rw, int rh, int *value, int min_value, int max_value);

#ifdef __cplusplus
}
#endif

/* ========================================================================== */
/* IMPLEMENTATION                                                              */
/* ========================================================================== */
#ifdef MINIGUI_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <locale.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#else
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKB.h>
#include <X11/keysym.h>
#endif

/* --- default theme --- */
static MgColor mg_c_bg     = 0xFF14161Cu;
static MgColor mg_c_panel  = 0xFF222633u;
static MgColor mg_c_panel2 = 0xFF2C3142u;
static MgColor mg_c_text   = 0xFFE6E9F0u;
static MgColor mg_c_accent = 0xFF7C9CFFu;
static MgColor mg_c_border = 0xFF3A4054u;
static int mg_text_scale = 1;   /* default text scale: 1 = 8x12 */

void mg_set_theme(MgColor background, MgColor panel, MgColor text, MgColor accent)
{
    mg_c_bg = background;
    mg_c_panel = panel;
    mg_c_text = text;
    mg_c_accent = accent;
    mg_c_border = 0xFF3A4054u;
    mg_c_panel2 = (panel & 0xFF000000u) |
                  ((((panel >> 16) & 0xFFu) + 16 > 255 ? 255 : ((panel >> 16) & 0xFFu) + 16) << 16) |
                  ((((panel >> 8) & 0xFFu) + 16 > 255 ? 255 : ((panel >> 8) & 0xFFu) + 16) << 8) |
                  (((panel & 0xFFu) + 16 > 255 ? 255 : (panel & 0xFFu) + 16));
}

/* --- Built-in 8x12 font from DejaVu Sans Mono: ASCII + Cyrillic,
 *     uppercase and lowercase separate. No external files. --- */
#define MG_FONT_W 8
#define MG_FONT_H 12
#define MG_FONT_ADV 9
static const unsigned char mg_font_data[161][MG_FONT_H] = {
    /*         U+0020 */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* !       U+0021 */ {0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00},
    /* "       U+0022 */ {0x34,0x34,0x34,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* #       U+0023 */ {0x48,0x68,0xFE,0x7E,0x24,0x7F,0x37,0x12,0x12,0x00,0x00,0x00},
    /* $       U+0024 */ {0x18,0x3C,0x16,0x16,0x3C,0x78,0x40,0x72,0x3E,0x10,0x00,0x00},
    /* %       U+0025 */ {0x0E,0x0B,0x0B,0x6E,0x1C,0x76,0xD0,0xD8,0x70,0x00,0x00,0x00},
    /* &       U+0026 */ {0x04,0x06,0x04,0x0C,0x9A,0xD3,0x63,0x66,0xFC,0x00,0x00,0x00},
    /* '       U+0027 */ {0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* (       U+0028 */ {0x10,0x18,0x08,0x08,0x08,0x08,0x08,0x08,0x18,0x10,0x10,0x00},
    /* )       U+0029 */ {0x08,0x18,0x18,0x10,0x10,0x10,0x10,0x18,0x18,0x08,0x04,0x00},
    /* *       U+002A */ {0x4A,0x3C,0x18,0x7E,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* +       U+002B */ {0x00,0x00,0x18,0x18,0x7E,0x7A,0x18,0x18,0x00,0x00,0x00,0x00},
    /* ,       U+002C */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x08,0x08,0x00},
    /* -       U+002D */ {0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00},
    /* .       U+002E */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00},
    /* /       U+002F */ {0x60,0x20,0x30,0x10,0x18,0x08,0x0C,0x04,0x06,0x02,0x00,0x00},
    /* 0       U+0030 */ {0x34,0x66,0x62,0x42,0x5A,0x62,0x66,0x26,0x3C,0x00,0x00,0x00},
    /* 1       U+0031 */ {0x1E,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x7C,0x00,0x00,0x00},
    /* 2       U+0032 */ {0x36,0x60,0x60,0x20,0x30,0x18,0x0C,0x06,0x7E,0x00,0x00,0x00},
    /* 3       U+0033 */ {0x36,0x60,0x60,0x38,0x38,0x60,0x60,0x62,0x3E,0x00,0x00,0x00},
    /* 4       U+0034 */ {0x30,0x38,0x2C,0x24,0x22,0x7E,0x7E,0x20,0x20,0x00,0x00,0x00},
    /* 5       U+0035 */ {0x3E,0x06,0x06,0x3E,0x60,0x60,0x60,0x22,0x3E,0x00,0x00,0x00},
    /* 6       U+0036 */ {0x2C,0x06,0x02,0x3E,0x66,0x42,0x42,0x66,0x3C,0x00,0x00,0x00},
    /* 7       U+0037 */ {0x7E,0x20,0x20,0x30,0x10,0x18,0x18,0x08,0x0C,0x00,0x00,0x00},
    /* 8       U+0038 */ {0x66,0x62,0x66,0x3C,0x3E,0x62,0x42,0x66,0x3C,0x00,0x00,0x00},
    /* 9       U+0039 */ {0x36,0x62,0x62,0x62,0x7E,0x7C,0x60,0x30,0x1C,0x00,0x00,0x00},
    /* :       U+003A */ {0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00},
    /* ;       U+003B */ {0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x08,0x08,0x00},
    /* <       U+003C */ {0x00,0x00,0x60,0x3C,0x06,0x0E,0x78,0x40,0x00,0x00,0x00,0x00},
    /* =       U+003D */ {0x00,0x00,0x00,0x7F,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00},
    /* >       U+003E */ {0x00,0x00,0x07,0x3C,0x60,0x70,0x1E,0x03,0x00,0x00,0x00,0x00},
    /* ?       U+003F */ {0x36,0x60,0x20,0x30,0x18,0x08,0x00,0x08,0x18,0x00,0x00,0x00},
    /* @       U+0040 */ {0x3C,0x46,0xC2,0xFB,0xC9,0xCD,0xC9,0xFB,0x02,0x06,0x78,0x00},
    /* A       U+0041 */ {0x18,0x1C,0x34,0x24,0x26,0x7E,0x7E,0x42,0xC3,0x00,0x00,0x00},
    /* B       U+0042 */ {0x3E,0x62,0x62,0x3E,0x76,0x42,0x42,0x66,0x3E,0x00,0x00,0x00},
    /* C       U+0043 */ {0x6C,0x06,0x06,0x02,0x02,0x02,0x06,0x4C,0x78,0x00,0x00,0x00},
    /* D       U+0044 */ {0x3E,0x62,0x62,0x42,0x42,0x62,0x62,0x32,0x1E,0x00,0x00,0x00},
    /* E       U+0045 */ {0x7E,0x06,0x06,0x7E,0x36,0x06,0x06,0x06,0x7E,0x00,0x00,0x00},
    /* F       U+0046 */ {0x7E,0x06,0x06,0x7E,0x36,0x06,0x06,0x06,0x06,0x00,0x00,0x00},
    /* G       U+0047 */ {0x6C,0x06,0x02,0x02,0x72,0x42,0x42,0x66,0x7C,0x00,0x00,0x00},
    /* H       U+0048 */ {0x42,0x42,0x42,0x7E,0x7E,0x42,0x42,0x42,0x42,0x00,0x00,0x00},
    /* I       U+0049 */ {0x3E,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00},
    /* J       U+004A */ {0x3C,0x20,0x20,0x20,0x20,0x20,0x20,0x32,0x1E,0x00,0x00,0x00},
    /* K       U+004B */ {0x62,0x32,0x1A,0x0E,0x1E,0x12,0x32,0x62,0xC2,0x00,0x00,0x00},
    /* L       U+004C */ {0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x7E,0x00,0x00,0x00},
    /* M       U+004D */ {0x67,0x67,0x77,0x5B,0x5B,0x4B,0x43,0x43,0x43,0x00,0x00,0x00},
    /* N       U+004E */ {0x46,0x46,0x4E,0x4A,0x5A,0x52,0x72,0x62,0x62,0x00,0x00,0x00},
    /* O       U+004F */ {0x36,0x66,0x42,0x42,0x42,0x42,0x62,0x66,0x3C,0x00,0x00,0x00},
    /* P       U+0050 */ {0x7E,0x66,0x46,0x66,0x7E,0x06,0x06,0x06,0x06,0x00,0x00,0x00},
    /* Q       U+0051 */ {0x36,0x66,0x42,0x42,0x42,0x42,0x62,0x66,0x3C,0x30,0x20,0x00},
    /* R       U+0052 */ {0x3E,0x62,0x62,0x62,0x3E,0x32,0x62,0x42,0xC2,0x00,0x00,0x00},
    /* S       U+0053 */ {0x66,0x02,0x02,0x0E,0x38,0x60,0x40,0x62,0x3E,0x00,0x00,0x00},
    /* T       U+0054 */ {0x7F,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00},
    /* U       U+0055 */ {0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x66,0x3C,0x00,0x00,0x00},
    /* V       U+0056 */ {0x43,0x42,0x66,0x26,0x24,0x34,0x3C,0x18,0x18,0x00,0x00,0x00},
    /* W       U+0057 */ {0xC1,0xC3,0xDB,0x5B,0x5A,0x7E,0x66,0x66,0x66,0x00,0x00,0x00},
    /* X       U+0058 */ {0x66,0x24,0x3C,0x18,0x18,0x3C,0x24,0x66,0xC3,0x00,0x00,0x00},
    /* Y       U+0059 */ {0x42,0x66,0x24,0x1C,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00},
    /* Z       U+005A */ {0x7E,0x60,0x30,0x10,0x18,0x08,0x04,0x06,0xFE,0x00,0x00,0x00},
    /* [       U+005B */ {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x18,0x38,0x00},
    /* \       U+005C */ {0x02,0x06,0x04,0x0C,0x08,0x18,0x10,0x30,0x20,0x60,0x00,0x00},
    /* ]       U+005D */ {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x18,0x1C,0x00},
    /* ^       U+005E */ {0x18,0x34,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* _       U+005F */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    /* `       U+0060 */ {0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* a       U+0061 */ {0x00,0x00,0x3E,0x60,0x78,0x7E,0x62,0x62,0x7C,0x00,0x00,0x00},
    /* b       U+0062 */ {0x06,0x06,0x3E,0x66,0x46,0x46,0x46,0x66,0x3E,0x00,0x00,0x00},
    /* c       U+0063 */ {0x00,0x00,0x7C,0x06,0x06,0x06,0x06,0x0C,0x78,0x00,0x00,0x00},
    /* d       U+0064 */ {0x60,0x68,0x7C,0x66,0x62,0x62,0x62,0x66,0x7C,0x00,0x00,0x00},
    /* e       U+0065 */ {0x00,0x00,0x3C,0x66,0x7E,0x7E,0x02,0x46,0x7C,0x00,0x00,0x00},
    /* f       U+0066 */ {0x18,0x08,0x7E,0x18,0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00},
    /* g       U+0067 */ {0x00,0x00,0x7C,0x66,0x62,0x62,0x62,0x66,0x7C,0x60,0x34,0x1C},
    /* h       U+0068 */ {0x06,0x06,0x3E,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00},
    /* i       U+0069 */ {0x00,0x00,0x1C,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00},
    /* j       U+006A */ {0x00,0x00,0x1C,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x18,0x0E},
    /* k       U+006B */ {0x06,0x06,0x66,0x36,0x1E,0x1E,0x36,0x66,0x46,0x00,0x00,0x00},
    /* l       U+006C */ {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x18,0x70,0x00,0x00,0x00},
    /* m       U+006D */ {0x00,0x00,0x7E,0x5A,0x5A,0x5A,0x5A,0x5B,0x5A,0x00,0x00,0x00},
    /* n       U+006E */ {0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00},
    /* o       U+006F */ {0x00,0x00,0x3C,0x66,0x42,0x42,0x62,0x66,0x3C,0x00,0x00,0x00},
    /* p       U+0070 */ {0x00,0x00,0x3E,0x66,0x46,0x46,0x46,0x66,0x3E,0x02,0x06,0x02},
    /* q       U+0071 */ {0x00,0x00,0x7C,0x66,0x62,0x62,0x62,0x66,0x7C,0x60,0x60,0x40},
    /* r       U+0072 */ {0x00,0x00,0x7C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x00,0x00},
    /* s       U+0073 */ {0x00,0x00,0x3C,0x06,0x0E,0x3C,0x60,0x60,0x3E,0x00,0x00,0x00},
    /* t       U+0074 */ {0x14,0x08,0x7E,0x08,0x08,0x08,0x08,0x08,0x78,0x00,0x00,0x00},
    /* u       U+0075 */ {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00},
    /* v       U+0076 */ {0x00,0x00,0x42,0x66,0x26,0x24,0x3C,0x1C,0x18,0x00,0x00,0x00},
    /* w       U+0077 */ {0x00,0x00,0xC1,0xC3,0x5B,0x5A,0x7E,0x66,0x26,0x00,0x00,0x00},
    /* x       U+0078 */ {0x00,0x00,0x66,0x24,0x1C,0x18,0x3C,0x26,0x62,0x00,0x00,0x00},
    /* y       U+0079 */ {0x00,0x00,0x42,0x66,0x24,0x24,0x3C,0x18,0x18,0x18,0x0C,0x06},
    /* z       U+007A */ {0x00,0x00,0x7E,0x20,0x30,0x18,0x0C,0x04,0x7E,0x00,0x00,0x00},
    /* {       U+007B */ {0x18,0x18,0x18,0x18,0x0E,0x0C,0x18,0x18,0x18,0x18,0x30,0x00},
    /* |       U+007C */ {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},
    /* }       U+007D */ {0x18,0x18,0x18,0x18,0x30,0x30,0x18,0x18,0x18,0x08,0x0E,0x00},
    /* ~       U+007E */ {0x00,0x00,0x00,0x00,0x7E,0x70,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ?       U+0410 */ {0x18,0x1C,0x34,0x24,0x26,0x7E,0x7E,0x42,0xC3,0x00,0x00,0x00},
    /* ?       U+0411 */ {0x7E,0x02,0x02,0x3E,0x7E,0x42,0x42,0x66,0x3E,0x00,0x00,0x00},
    /* ?       U+0412 */ {0x3E,0x62,0x62,0x3E,0x76,0x42,0x42,0x66,0x3E,0x00,0x00,0x00},
    /* ?       U+0413 */ {0x7E,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x00,0x00,0x00},
    /* ?       U+0414 */ {0x7E,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0xFF,0xC1,0xC1,0x00},
    /* ?       U+0415 */ {0x7E,0x06,0x06,0x7E,0x36,0x06,0x06,0x06,0x7E,0x00,0x00,0x00},
    /* ?       U+0416 */ {0x5B,0x7A,0x3E,0x3C,0x3C,0x7E,0x5A,0x5B,0xDB,0x00,0x00,0x00},
    /* ?       U+0417 */ {0x36,0x60,0x60,0x38,0x38,0x60,0x60,0x62,0x3E,0x00,0x00,0x00},
    /* ?       U+0418 */ {0x62,0x72,0x72,0x52,0x4A,0x4A,0x4E,0x46,0x46,0x00,0x00,0x00},
    /* ?       U+0419 */ {0x1C,0x72,0x72,0x52,0x4A,0x4A,0x4E,0x46,0x46,0x00,0x00,0x00},
    /* ?       U+041A */ {0x62,0x32,0x1A,0x0E,0x1E,0x12,0x32,0x62,0xC2,0x00,0x00,0x00},
    /* ?       U+041B */ {0x7C,0x44,0x44,0x44,0x44,0x44,0x44,0x46,0x43,0x00,0x00,0x00},
    /* ?       U+041C */ {0x67,0x67,0x77,0x5B,0x5B,0x4B,0x43,0x43,0x43,0x00,0x00,0x00},
    /* ?       U+041D */ {0x42,0x42,0x42,0x7E,0x7E,0x42,0x42,0x42,0x42,0x00,0x00,0x00},
    /* ?       U+041E */ {0x36,0x66,0x42,0x42,0x42,0x42,0x62,0x66,0x3C,0x00,0x00,0x00},
    /* ?       U+041F */ {0x7E,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x00,0x00,0x00},
    /* ?       U+0420 */ {0x7E,0x66,0x46,0x66,0x7E,0x06,0x06,0x06,0x06,0x00,0x00,0x00},
    /* ?       U+0421 */ {0x6C,0x06,0x06,0x02,0x02,0x02,0x06,0x4C,0x78,0x00,0x00,0x00},
    /* ?       U+0422 */ {0x7F,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00},
    /* ?       U+0423 */ {0x42,0x66,0x24,0x3C,0x3C,0x18,0x18,0x08,0x0E,0x00,0x00,0x00},
    /* ?       U+0424 */ {0x3C,0x7E,0x5B,0xDB,0xDB,0x5B,0x7A,0x3C,0x18,0x00,0x00,0x00},
    /* ?       U+0425 */ {0x66,0x24,0x3C,0x18,0x18,0x3C,0x24,0x66,0xC3,0x00,0x00,0x00},
    /* ?       U+0426 */ {0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0xFF,0xC0,0xC0,0x00},
    /* ?       U+0427 */ {0x42,0x62,0x42,0x66,0x7E,0x40,0x60,0x60,0x40,0x00,0x00,0x00},
    /* ?       U+0428 */ {0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x7E,0x00,0x00,0x00},
    /* ?       U+0429 */ {0x6B,0x6B,0x6B,0x6B,0x6B,0x6B,0x6B,0x6B,0xFF,0xC0,0xC0,0x00},
    /* ?       U+042A */ {0x07,0x04,0x04,0x3C,0x7C,0x44,0x44,0x64,0x3C,0x00,0x00,0x00},
    /* ?       U+042B */ {0x43,0x43,0x43,0x4F,0x5F,0x53,0x53,0x5B,0x4F,0x00,0x00,0x00},
    /* ?       U+042C */ {0x06,0x06,0x06,0x3E,0x76,0x46,0x46,0x66,0x3E,0x00,0x00,0x00},
    /* ?       U+042D */ {0x32,0x20,0x60,0x7C,0x7C,0x60,0x60,0x32,0x1E,0x00,0x00,0x00},
    /* ?       U+042E */ {0x7B,0x4B,0xCF,0xCF,0xCF,0xCF,0x4F,0x6B,0x3B,0x00,0x00,0x00},
    /* ?       U+042F */ {0x7C,0x46,0x42,0x46,0x7C,0x4C,0x44,0x46,0x42,0x00,0x00,0x00},
    /* ?       U+0430 */ {0x00,0x00,0x3E,0x60,0x78,0x7E,0x62,0x62,0x7C,0x00,0x00,0x00},
    /* ?       U+0431 */ {0x06,0x02,0x3E,0x66,0x42,0x42,0x62,0x66,0x3C,0x00,0x00,0x00},
    /* ?       U+0432 */ {0x00,0x00,0x3E,0x26,0x36,0x3E,0x66,0x66,0x3E,0x00,0x00,0x00},
    /* ?       U+0433 */ {0x00,0x00,0x3C,0x06,0x06,0x06,0x06,0x06,0x04,0x00,0x00,0x00},
    /* ?       U+0434 */ {0x00,0x00,0x3C,0x66,0x64,0x66,0x66,0x66,0x7E,0x43,0x42,0x00},
    /* ?       U+0435 */ {0x00,0x00,0x3C,0x66,0x7E,0x7E,0x02,0x46,0x7C,0x00,0x00,0x00},
    /* ?       U+0436 */ {0x00,0x00,0x5A,0x7E,0x3C,0x3C,0x7E,0x5A,0x5B,0x00,0x00,0x00},
    /* ?       U+0437 */ {0x00,0x00,0x3E,0x60,0x38,0x38,0x60,0x60,0x3E,0x00,0x00,0x00},
    /* ?       U+0438 */ {0x00,0x00,0x66,0x76,0x76,0x7A,0x6E,0x66,0x66,0x00,0x00,0x00},
    /* ?       U+0439 */ {0x18,0x00,0x66,0x76,0x76,0x7A,0x6E,0x66,0x66,0x00,0x00,0x00},
    /* ?       U+043A */ {0x00,0x00,0x66,0x36,0x1E,0x1E,0x36,0x66,0x46,0x00,0x00,0x00},
    /* ?       U+043B */ {0x00,0x00,0x7C,0x64,0x64,0x64,0x66,0x66,0x63,0x00,0x00,0x00},
    /* ?       U+043C */ {0x00,0x00,0xC3,0xE3,0xE7,0xFF,0xDB,0xC3,0xC3,0x00,0x00,0x00},
    /* ?       U+043D */ {0x00,0x00,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00,0x00,0x00},
    /* ?       U+043E */ {0x00,0x00,0x3C,0x66,0x42,0x42,0x62,0x66,0x3C,0x00,0x00,0x00},
    /* ?       U+043F */ {0x00,0x00,0x7E,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00},
    /* ?       U+0440 */ {0x00,0x00,0x3E,0x66,0x46,0x46,0x46,0x66,0x3E,0x02,0x06,0x02},
    /* ?       U+0441 */ {0x00,0x00,0x7C,0x06,0x06,0x06,0x06,0x0C,0x78,0x00,0x00,0x00},
    /* ?       U+0442 */ {0x00,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00},
    /* ?       U+0443 */ {0x00,0x00,0x42,0x66,0x24,0x24,0x3C,0x18,0x18,0x18,0x0C,0x06},
    /* ?       U+0444 */ {0x18,0x18,0x3E,0x5A,0x5A,0x5B,0x5A,0x7E,0x3C,0x18,0x18,0x08},
    /* ?       U+0445 */ {0x00,0x00,0x66,0x24,0x1C,0x18,0x3C,0x26,0x62,0x00,0x00,0x00},
    /* ?       U+0446 */ {0x00,0x00,0x22,0x22,0x22,0x22,0x22,0x22,0x7E,0x40,0x40,0x00},
    /* ?       U+0447 */ {0x00,0x00,0x66,0x66,0x66,0x7C,0x68,0x60,0x60,0x00,0x00,0x00},
    /* ?       U+0448 */ {0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A,0x7E,0x00,0x00,0x00},
    /* ?       U+0449 */ {0x00,0x00,0x4B,0x4B,0x4B,0x4B,0x4B,0x4B,0xFF,0xC0,0x80,0x00},
    /* ?       U+044A */ {0x00,0x00,0x07,0x04,0x1C,0x7C,0xC4,0x44,0x7C,0x00,0x00,0x00},
    /* ?       U+044B */ {0x00,0x00,0x42,0x43,0x42,0x5E,0x53,0x5B,0x5E,0x00,0x00,0x00},
    /* ?       U+044C */ {0x00,0x00,0x06,0x06,0x06,0x3E,0x66,0x66,0x3E,0x00,0x00,0x00},
    /* ?       U+044D */ {0x00,0x00,0x3E,0x20,0x68,0x7C,0x60,0x30,0x1E,0x00,0x00,0x00},
    /* ?       U+044E */ {0x00,0x00,0x7B,0x4B,0xCF,0xCF,0x4B,0x4B,0x33,0x00,0x00,0x00},
    /* ?       U+044F */ {0x00,0x00,0x3C,0x26,0x26,0x3C,0x2C,0x24,0x26,0x00,0x00,0x00},
    /* ?       U+0401 */ {0x14,0x08,0x7E,0x08,0x08,0x08,0x08,0x08,0x78,0x00,0x00,0x00},
    /* ?       U+0451 */ {0x14,0x00,0x3C,0x66,0x7E,0x7E,0x02,0x46,0x7C,0x00,0x00,0x00}
};

/* UTF-8 decoder: returns a character code and advances the pointer. */
static unsigned int mg_utf8_next(const char **pp)
{
    const unsigned char *s = (const unsigned char *)*pp;
    unsigned int c = s[0];
    int n, i;
    if (c < 0x80u) { ++(*pp); return c; }
    if ((c & 0xE0u) == 0xC0u) { n = 2; c &= 0x1Fu; }
    else if ((c & 0xF0u) == 0xE0u) { n = 3; c &= 0x0Fu; }
    else if ((c & 0xF8u) == 0xF0u) { n = 4; c &= 0x07u; }
    else { ++(*pp); return 0xFFFDu; }
    for (i = 1; i < n; ++i) {
        if ((s[i] & 0xC0u) != 0x80u) { ++(*pp); return 0xFFFDu; }
        c = (c << 6) | (s[i] & 0x3Fu);
    }
    *pp += n;
    return c;
}

/* Encode a character as UTF-8. Returns the number of bytes written. */
static int mg_utf8_encode(unsigned int cp, char *out)
{
    if (cp < 0x80u) { out[0] = (char)cp; return 1; }
    if (cp < 0x800u) {
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp < 0x10000u) {
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    out[0] = (char)(0xF0u | (cp >> 18));
    out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    out[3] = (char)(0x80u | (cp & 0x3Fu));
    return 4;
}

/* Number of characters (not bytes!) in a string. */
static int mg_utf8_count(const char *s)
{
    int n = 0;
    if (!s) return 0;
    while (*s) { mg_utf8_next(&s); ++n; }
    return n;
}

/* Byte offset corresponding to the character at index idx. */
static int mg_utf8_offset(const char *s, int idx)
{
    int i = 0;
    const char *p = s;
    while (*p && i < idx) { mg_utf8_next(&p); ++i; }
    return (int)(p - s);
}

/* Glyph selection: ASCII, Cyrillic (uppercase and lowercase separate), '?'. */
static const unsigned char *mg_glyph(unsigned int cp)
{
    if (cp >= 32u && cp <= 126u) return mg_font_data[cp - 32u];
    if (cp == 0x0401u) return mg_font_data[159];                     /* Yo */
    if (cp == 0x0451u) return mg_font_data[160];                     /* yo */
    if (cp >= 0x0410u && cp <= 0x042Fu) return mg_font_data[95 + (cp - 0x0410u)];
    if (cp >= 0x0430u && cp <= 0x044Fu) return mg_font_data[127 + (cp - 0x0430u)];
    return mg_font_data['?' - 32];
}

/* --- window structure --- */
struct MgWindow {
    int width, height;
    unsigned char *pixels;          /* 32 bpp: AARRGGBB in memory = BB GG RR AA */
    int mouse_x, mouse_y;
    int mouse_down, mouse_prev_down;
    int mouse_rdown, mouse_rprev_down;
    unsigned char keys[512], prev_keys[512];
    unsigned char typed[64];
    int typed_len;
    int quit;
    unsigned int focus_id, drag_id;
    int focus_cursor;
    int fullscreen;
#ifdef _WIN32
    HWND hwnd;
    HINSTANCE hinst;
    BITMAPINFO bmi;
    RECT saved_rect;
#else
    Display *dpy;
    Window xwin;
    GC gc;
    XImage *img;
    Atom wm_delete;
    int screen;
    int saved_width, saved_height;
#endif
};

/* --- internal helpers --- */
static int mg_hit(MgWindow *w, int x, int y, int rw, int rh)
{
    return w->mouse_x >= x && w->mouse_x < x + rw &&
           w->mouse_y >= y && w->mouse_y < y + rh;
}

static int mg_clamp_int(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static unsigned int mg_id(const char *label, int x, int y, const void *ptr)
{
    unsigned int h = 2166136261u;
    if (label) {
        while (*label) {
            h ^= (unsigned char)*label++;
            h *= 16777619u;
        }
    }
    if (ptr) {
        uintptr_t a = (uintptr_t)ptr;
        int i;
        for (i = 0; i < (int)sizeof(uintptr_t); ++i) {
            h ^= (unsigned int)(a & 0xFFu);
            h *= 16777619u;
            a >>= 8;
        }
    }
    h ^= (unsigned int)x; h *= 16777619u;
    h ^= (unsigned int)y; h *= 16777619u;
    return h;
}

static void mg_resize_internal(MgWindow *w, int nw, int nh)
{
    if (nw <= 0 || nh <= 0) return;
    if (nw == w->width && nh == w->height && w->pixels) return;
#ifdef _WIN32
    {
        unsigned char *p = (unsigned char *)realloc(w->pixels, (size_t)nw * (size_t)nh * 4u);
        if (!p) return;
        w->pixels = p;
        w->bmi.bmiHeader.biWidth = (LONG)nw;
        w->bmi.bmiHeader.biHeight = -(LONG)nh;
    }
#else
    {
        XImage *new_img = XCreateImage(w->dpy, DefaultVisual(w->dpy, w->screen),
                                       24, ZPixmap, 0, NULL, (unsigned int)nw, (unsigned int)nh, 32, 0);
        char *data;
        if (!new_img) return;
        /* XCreateImage(data=NULL) does NOT allocate memory -- we do it ourselves. */
        if (!new_img->data) {
            data = (char *)malloc((size_t)new_img->bytes_per_line * (size_t)new_img->height);
            if (!data) { XDestroyImage(new_img); return; }
            new_img->data = data;
        }
        if (w->img) XDestroyImage(w->img);
        w->img = new_img;
        w->pixels = (unsigned char *)w->img->data;
    }
#endif
    w->width = nw;
    w->height = nh;
}

/* =========================== DRAWING =========================== */

void mg_clear(MgWindow *w, MgColor c)
{
    size_t n = (size_t)w->width * (size_t)w->height;
    unsigned int *p = (unsigned int *)w->pixels;
    size_t i;
    for (i = 0; i < n; ++i) p[i] = c;
}

void mg_pixel(MgWindow *w, int x, int y, MgColor c)
{
    if (x < 0 || y < 0 || x >= w->width || y >= w->height) return;
    ((unsigned int *)w->pixels)[(size_t)y * (size_t)w->width + (size_t)x] = c;
}

void mg_rect(MgWindow *w, int x, int y, int rw, int rh, MgColor c)
{
    int x0 = mg_clamp_int(x, 0, w->width);
    int y0 = mg_clamp_int(y, 0, w->height);
    int x1 = mg_clamp_int(x + rw, 0, w->width);
    int y1 = mg_clamp_int(y + rh, 0, w->height);
    int yy, xx;
    if (x1 <= x0 || y1 <= y0) return;
    for (yy = y0; yy < y1; ++yy) {
        unsigned int *row = (unsigned int *)w->pixels + (size_t)yy * (size_t)w->width;
        for (xx = x0; xx < x1; ++xx) row[xx] = c;
    }
}

void mg_rect_border(MgWindow *w, int x, int y, int rw, int rh, MgColor c, int thickness)
{
    int t = thickness > 0 ? thickness : 1;
    mg_rect(w, x, y, rw, t, c);
    mg_rect(w, x, y + rh - t, rw, t, c);
    mg_rect(w, x, y, t, rh, c);
    mg_rect(w, x + rw - t, y, t, rh, c);
}

void mg_line(MgWindow *w, int x0, int y0, int x1, int y1, MgColor c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        mg_pixel(w, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        {
            int e2 = err * 2;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx)  { err += dx; y0 += sy; }
        }
    }
}

void mg_circle(MgWindow *w, int cx, int cy, int r, MgColor c)
{
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        mg_pixel(w, cx + x, cy + y, c); mg_pixel(w, cx - x, cy + y, c);
        mg_pixel(w, cx + y, cy + x, c); mg_pixel(w, cx - y, cy + x, c);
        mg_pixel(w, cx + x, cy - y, c); mg_pixel(w, cx - x, cy - y, c);
        mg_pixel(w, cx + y, cy - x, c); mg_pixel(w, cx - y, cy - x, c);
        ++y;
        if (err < 0) err += 2 * y + 1;
        else { --x; err += 2 * (y - x) + 1; }
    }
}

static int mg_font_size_clamp(int size)
{
    return size > 0 ? (size < 8 ? size : 8) : 1;
}

void mg_set_font_size(int size)  { mg_text_scale = mg_font_size_clamp(size); }
int  mg_get_font_size(void)      { return mg_text_scale; }
void mg_set_font_color(MgColor c){ mg_c_text = c; }
MgColor mg_get_font_color(void)  { return mg_c_text; }

void mg_set_font(int size, MgColor color)
{
    mg_set_font_size(size);
    mg_set_font_color(color);
}

/* Old names kept for compatibility. */
void mg_set_text_scale(int scale) { mg_set_font_size(scale); }
int  mg_get_text_scale(void)      { return mg_get_font_size(); }

/* scale = 0 means "use the global scale". */
void mg_text(MgWindow *w, int x, int y, const char *text, MgColor c, int scale)
{
    int cx = x;
    const char *p;
    int s = scale > 0 ? scale : mg_text_scale;
    if (!text) return;
    p = text;
    while (*p) {
        unsigned int cp = mg_utf8_next(&p);
        const unsigned char *glyph = mg_glyph(cp);
        int col, row;
        for (row = 0; row < MG_FONT_H; ++row) {
            unsigned int bits = glyph[row];
            for (col = 0; col < MG_FONT_W; ++col) {
                if (bits & (1u << col)) {
                    mg_rect(w, cx + col * s, y + row * s, s, s, c);
                }
            }
        }
        cx += MG_FONT_ADV * s;
    }
}

int mg_text_width(const char *text, int scale)
{
    int s = scale > 0 ? scale : mg_text_scale;
    return mg_utf8_count(text) * MG_FONT_ADV * s;
}

/* printf-style formatting: mg_textf(w,10,10,color,0,"count=%d",n). */
static void mg_vtextf(MgWindow *w, int x, int y, MgColor c, int size,
                      const char *fmt, va_list ap)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    mg_text(w, x, y, buf, c, size);
}

void mg_textf(MgWindow *w, int x, int y, MgColor c, int size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    mg_vtextf(w, x, y, c, size, fmt, ap);
    va_end(ap);
}

void mg_image(MgWindow *w, int x, int y, int iw, int ih, const MgColor *pixels)
{
    int row;
    for (row = 0; row < ih; ++row) {
        int xx;
        int yy = y + row;
        if (yy < 0 || yy >= w->height) continue;
        for (xx = 0; xx < iw; ++xx) {
            int tx = x + xx;
            if (tx >= 0 && tx < w->width) {
                ((unsigned int *)w->pixels)[(size_t)yy * (size_t)w->width + (size_t)tx] =
                    pixels[(size_t)row * (size_t)iw + (size_t)xx];
            }
        }
    }
}

/* ========================== INPUT ========================= */

int mg_mouse_x(MgWindow *w) { return w->mouse_x; }
int mg_mouse_y(MgWindow *w) { return w->mouse_y; }
int mg_mouse_down(MgWindow *w) { return w->mouse_down; }
int mg_mouse_pressed(MgWindow *w) { return w->mouse_down && !w->mouse_prev_down; }
int mg_key_down(MgWindow *w, int code)
{
    return code >= 0 && code < 512 ? (int)w->keys[code] : 0;
}
int mg_key_pressed(MgWindow *w, int code)
{
    return code >= 0 && code < 512 ? (int)(w->keys[code] && !w->prev_keys[code]) : 0;
}
const char *mg_typed(MgWindow *w) { return (const char *)w->typed; }

/* ========================== WIDGETS ========================== */

void mg_panel(MgWindow *w, int x, int y, int rw, int rh)
{
    mg_rect(w, x, y, rw, rh, mg_c_panel);
    mg_rect_border(w, x, y, rw, rh, mg_c_border, 1);
}

void mg_label(MgWindow *w, int x, int y, const char *text)
{
    mg_text(w, x, y, text, mg_c_text, 0);
}

/* Custom color/size for a single label. */
void mg_label_ex(MgWindow *w, int x, int y, const char *text, MgColor c, int size)
{
    mg_text(w, x, y, text, c, size);
}

void mg_labelf(MgWindow *w, int x, int y, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    mg_vtextf(w, x, y, mg_c_text, 0, fmt, ap);
    va_end(ap);
}

int mg_button(MgWindow *w, int x, int y, int rw, int rh, const char *label)
{
    int hover = mg_hit(w, x, y, rw, rh);
    MgColor c = hover ? mg_c_panel2 : mg_c_panel;
    int tw;
    int clicked = hover && mg_mouse_pressed(w);
    int fh = MG_FONT_H * mg_text_scale;
    if (hover && w->mouse_down) c = mg_c_accent;
    mg_rect(w, x, y, rw, rh, c);
    mg_rect_border(w, x, y, rw, rh, mg_c_border, 1);
    tw = mg_text_width(label, 0);
    mg_text(w, x + (rw - tw) / 2, y + (rh - fh) / 2, label, mg_c_text, 0);
    if (clicked) w->focus_id = 0;
    return clicked;
}

int mg_checkbox(MgWindow *w, int x, int y, const char *label, int *checked)
{
    int side = 14 * mg_text_scale;
    int tw = mg_text_width(label, 0);
    int fh = MG_FONT_H * mg_text_scale;
    int inside = mg_hit(w, x, y, side + tw + 8, side);
    int clicked = inside && mg_mouse_pressed(w);
    mg_rect(w, x, y, side, side, mg_c_panel);
    mg_rect_border(w, x, y, side, side, mg_c_border, 1);
    if (*checked) mg_rect(w, x + 3, y + 3, side - 6, side - 6, mg_c_accent);
    mg_text(w, x + side + 6, y + (side - fh) / 2, label, mg_c_text, 0);
    if (clicked) { *checked = !*checked; w->focus_id = 0; }
    return clicked;
}

int mg_textfield(MgWindow *w, int x, int y, int rw, int rh, char *buffer, int capacity)
{
    unsigned int id = mg_id(NULL, x, y, (const void *)buffer);
    int inside = mg_hit(w, x, y, rw, rh);
    int focused = (w->focus_id == id);
    int changed = 0;
    int len, chars, start, i, adv = MG_FONT_ADV * mg_text_scale;
    int fh = MG_FONT_H * mg_text_scale;

    if (mg_mouse_pressed(w) && inside) {
        int click = (w->mouse_x - x - 6) / (adv > 0 ? adv : 1);
        w->focus_id = id;
        w->focus_cursor = click < 0 ? 0 : click;
        if (w->focus_cursor > mg_utf8_count(buffer)) w->focus_cursor = mg_utf8_count(buffer);
        focused = 1;
    }

    if (focused) {
        chars = mg_utf8_count(buffer);
        if (w->focus_cursor < 0) w->focus_cursor = 0;
        if (w->focus_cursor > chars) w->focus_cursor = chars;

        /* Backspace/Delete remove a whole UTF-8 character, not a single byte. */
        if (mg_key_pressed(w, MG_KEY_BACK) && w->focus_cursor > 0) {
            int from = mg_utf8_offset(buffer, w->focus_cursor - 1);
            int to = mg_utf8_offset(buffer, w->focus_cursor);
            len = (int)strlen(buffer);
            memmove(buffer + from, buffer + to, (size_t)(len - to + 1));
            --w->focus_cursor;
            changed = 1;
        }
        if (mg_key_pressed(w, MG_KEY_DELETE) && w->focus_cursor < chars) {
            int from = mg_utf8_offset(buffer, w->focus_cursor);
            int to = mg_utf8_offset(buffer, w->focus_cursor + 1);
            len = (int)strlen(buffer);
            memmove(buffer + from, buffer + to, (size_t)(len - to + 1));
            changed = 1;
        }
        if (mg_key_pressed(w, MG_KEY_LEFT) && w->focus_cursor > 0) --w->focus_cursor;
        if (mg_key_pressed(w, MG_KEY_RIGHT) && w->focus_cursor < mg_utf8_count(buffer)) ++w->focus_cursor;

        /* Insert typed text a whole UTF-8 character at a time. */
        i = 0;
        while (i < w->typed_len && w->typed[i]) {
            const char *q = (const char *)(w->typed + i);
            unsigned int cp = mg_utf8_next(&q);
            int clen = (int)(q - (const char *)(w->typed + i));
            len = (int)strlen(buffer);
            if (cp >= 32 && cp != 127 && len + clen < capacity) {
                int at = mg_utf8_offset(buffer, w->focus_cursor);
                memmove(buffer + at + clen, buffer + at, (size_t)(len - at + 1));
                memcpy(buffer + at, w->typed + i, (size_t)clen);
                ++w->focus_cursor;
                changed = 1;
            }
            i += clen > 0 ? clen : 1;
        }
    }

    mg_rect(w, x, y, rw, rh, mg_c_panel);
    mg_rect_border(w, x, y, rw, rh, focused ? mg_c_accent : mg_c_border, focused ? 2 : 1);

    chars = mg_utf8_count(buffer);
    start = w->focus_cursor - (rw - 12) / (adv > 0 ? adv : 1);
    if (start < 0) start = 0;
    if (start > chars) start = chars;
    mg_text(w, x + 6, y + (rh - fh) / 2, buffer + mg_utf8_offset(buffer, start), mg_c_text, 0);

    if (focused && ((mg_time_ms() / 500u) & 1u) == 0u) {
        int cx = x + 6 + (w->focus_cursor - start) * adv;
        mg_rect(w, cx, y + 3, 2 * mg_text_scale, rh - 6, mg_c_accent);
    }
    return changed;
}

int mg_slider(MgWindow *w, int x, int y, int rw, int rh, int *value, int min_value, int max_value)
{
    unsigned int id = mg_id(NULL, x, y, (const void *)value);
    int inside = mg_hit(w, x, y, rw, rh);
    int changed = 0;
    int track_x = x + 6;
    int track_w = rw - 12;
    int knob_x, val, old;

    if (mg_mouse_pressed(w) && inside) w->drag_id = id;
    if (w->drag_id == id && w->mouse_down && track_w > 0) {
        val = min_value + (int)(((long)(w->mouse_x - track_x) * (long)(max_value - min_value)) / (long)track_w);
        val = mg_clamp_int(val, min_value, max_value);
        if (val != *value) { *value = val; changed = 1; }
    }
    if (mg_mouse_pressed(w) && inside) w->focus_id = 0;

    old = *value;
    (void)old;

    mg_rect(w, x, y, rw, rh, mg_c_panel);
    mg_rect_border(w, x, y, rw, rh, mg_c_border, 1);
    mg_rect(w, track_x, y + rh / 2 - 2, track_w, 4, mg_c_border);
    if (max_value != min_value) {
        int filled = (int)(((long)(*value - min_value) * (long)track_w) / (long)(max_value - min_value));
        filled = mg_clamp_int(filled, 0, track_w);
        mg_rect(w, track_x, y + rh / 2 - 2, filled, 4, mg_c_accent);
    }
    knob_x = track_x + (int)(((long)(*value - min_value) * (long)track_w) /
                            (max_value != min_value ? (long)(max_value - min_value) : 1L));
    knob_x = mg_clamp_int(knob_x - 5, x + 2, x + rw - 12);
    mg_rect(w, knob_x, y + 3, 10, rh - 6, mg_c_accent);
    mg_rect_border(w, knob_x, y + 3, 10, rh - 6, mg_c_text, 1);
    return changed;
}

/* =========================== PLATFORM ========================== */

#ifdef _WIN32

static int mg_vk_to_code(WPARAM k)
{
    switch (k) {
        case VK_BACK: return MG_KEY_BACK;
        case VK_RETURN: return MG_KEY_ENTER;
        case VK_ESCAPE: return MG_KEY_ESC;
        case VK_LEFT: return MG_KEY_LEFT;
        case VK_RIGHT: return MG_KEY_RIGHT;
        case VK_UP: return MG_KEY_UP;
        case VK_DOWN: return MG_KEY_DOWN;
        case VK_DELETE: return MG_KEY_DELETE;
        case VK_SHIFT: return MG_KEY_SHIFT;
        case VK_CONTROL: return MG_KEY_CTRL;
        default: break;
    }
    if (k >= 'A' && k <= 'Z') return (int)k + 32;
    if (k >= 32 && k < 127) return (int)k;
    return 0;
}

static void mg_char_to_typed(MgWindow *w, unsigned int cp)
{
    char tmp[4];
    int nb;
    if (cp > 0x10FFFFu) return;
    nb = mg_utf8_encode(cp, tmp);
    if (w->typed_len + nb <= 60) {
        memcpy(w->typed + w->typed_len, tmp, (size_t)nb);
        w->typed_len += nb;
    }
}

static void mg_utf8_to_wide(const char *s, wchar_t *out, int count)
{
    if (!s) { out[0] = 0; return; }
    if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, out, count)) out[0] = 0;
}

static LRESULT CALLBACK mg_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    MgWindow *w = (MgWindow *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (msg == WM_CLOSE) {
        if (w) w->quit = 1;
        return 0;
    }
    if (msg == WM_SIZE) {
        if (w) mg_resize_internal(w, LOWORD(lp), HIWORD(lp));
        return 0;
    }
    if (msg == WM_LBUTTONDOWN) {
        if (w) { w->mouse_x = GET_X_LPARAM(lp); w->mouse_y = GET_Y_LPARAM(lp); w->mouse_down = 1; SetCapture(hwnd); }
        return 0;
    }
    if (msg == WM_LBUTTONUP) {
        if (w) { w->mouse_x = GET_X_LPARAM(lp); w->mouse_y = GET_Y_LPARAM(lp); w->mouse_down = 0; ReleaseCapture(); }
        return 0;
    }
    if (msg == WM_RBUTTONDOWN) {
        if (w) { w->mouse_rdown = 1; }
        return 0;
    }
    if (msg == WM_RBUTTONUP) {
        if (w) { w->mouse_rdown = 0; }
        return 0;
    }
    if (msg == WM_MOUSEMOVE) {
        if (w) { w->mouse_x = GET_X_LPARAM(lp); w->mouse_y = GET_Y_LPARAM(lp); }
        return 0;
    }
    if (msg == WM_KEYDOWN || msg == WM_KEYUP) {
        int code = mg_vk_to_code(wp);
        if (w && code > 0 && code < 512)
            w->keys[code] = (unsigned char)(msg == WM_KEYDOWN ? 1 : 0);
        return 0;
    }
    if (msg == WM_CHAR) {
        if (w) mg_char_to_typed(w, (unsigned int)wp);
        return 0;
    }
    if (msg == WM_DESTROY) return 0;
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void mg_pump(MgWindow *w)
{
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (!w->mouse_down) w->drag_id = 0;
}

MgWindow *mg_create(const char *title, int width, int height)
{
    static const wchar_t *cls = L"MiniGuiWindowClass";
    static int registered = 0;
    MgWindow *w = (MgWindow *)calloc(1, sizeof(MgWindow));
    wchar_t wtitle[512];
    if (!w) return NULL;
    setlocale(LC_ALL, "");
    w->width = width;
    w->height = height;
    w->hinst = GetModuleHandleW(NULL);
    mg_utf8_to_wide(title, wtitle, 512);

    if (!registered) {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = mg_wndproc;
        wc.hInstance = w->hinst;
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = cls;
        if (!RegisterClassExW(&wc)) { free(w); return NULL; }
        registered = 1;
    }

    w->hwnd = CreateWindowExW(0, cls, wtitle, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                              NULL, NULL, w->hinst, NULL);
    if (!w->hwnd) { free(w); return NULL; }
    SetWindowLongPtrW(w->hwnd, GWLP_USERDATA, (LONG_PTR)w);

    memset(&w->bmi, 0, sizeof(w->bmi));
    w->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    w->bmi.bmiHeader.biWidth = (LONG)width;
    w->bmi.bmiHeader.biHeight = -(LONG)height;
    w->bmi.bmiHeader.biPlanes = 1;
    w->bmi.bmiHeader.biBitCount = 32;
    w->bmi.bmiHeader.biCompression = BI_RGB;
    w->pixels = (unsigned char *)malloc((size_t)width * (size_t)height * 4u);
    if (!w->pixels) { DestroyWindow(w->hwnd); free(w); return NULL; }

    ShowWindow(w->hwnd, SW_SHOW);
    UpdateWindow(w->hwnd);
    return w;
}

void mg_destroy(MgWindow *w)
{
    if (!w) return;
    DestroyWindow(w->hwnd);
    free(w->pixels);
    free(w);
}

void mg_set_title(MgWindow *w, const char *title)
{
    wchar_t buf[512];
    if (!w || !title) return;
    mg_utf8_to_wide(title, buf, 512);
    SetWindowTextW(w->hwnd, buf);
}

void mg_set_pos(MgWindow *w, int x, int y)
{
    if (w) SetWindowPos(w->hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void mg_set_size(MgWindow *w, int width, int height)
{
    if (!w || width <= 0 || height <= 0) return;
    SetWindowPos(w->hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    mg_resize_internal(w, width, height);
}

int mg_width(MgWindow *w) { return w ? w->width : 0; }
int mg_height(MgWindow *w) { return w ? w->height : 0; }
int mg_is_fullscreen(MgWindow *w) { return w ? w->fullscreen : 0; }

void mg_fullscreen(MgWindow *w, int on)
{
    on = on ? 1 : 0;
    if (!w || w->fullscreen == on) return;
    if (on) {
        GetWindowRect(w->hwnd, &w->saved_rect);
        {
            MONITORINFO mi;
            memset(&mi, 0, sizeof(mi));
            mi.cbSize = sizeof(mi);
            GetMonitorInfoW(MonitorFromWindow(w->hwnd, MONITOR_DEFAULTTONEAREST), &mi);
            SetWindowLongPtrW(w->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(w->hwnd, HWND_TOPMOST,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    } else {
        RECT r = w->saved_rect;
        SetWindowLongPtrW(w->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(w->hwnd, HWND_NOTOPMOST, r.left, r.top,
                     r.right - r.left, r.bottom - r.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    w->fullscreen = on;
}

void mg_end(MgWindow *w)
{
    if (!w) return;
    {
        HDC hdc = GetDC(w->hwnd);
        if (hdc) {
            StretchDIBits(hdc, 0, 0, w->width, w->height,
                          0, 0, w->width, w->height,
                          w->pixels, &w->bmi, DIB_RGB_COLORS, SRCCOPY);
            ReleaseDC(w->hwnd, hdc);
        }
    }
    w->mouse_prev_down = w->mouse_down;
    w->mouse_rprev_down = w->mouse_rdown;
    memcpy(w->prev_keys, w->keys, sizeof(w->keys));
}

void mg_sleep(int ms)
{
    if (ms > 0) Sleep((DWORD)ms);
}

unsigned int mg_time_ms(void)
{
    return (unsigned int)GetTickCount64();
}

#else /* --- X11 / Linux --- */

static int mg_keysym_to_code(KeySym k)
{
    switch (k) {
        case XK_BackSpace: return MG_KEY_BACK;
        case XK_Return:
        case XK_KP_Enter: return MG_KEY_ENTER;
        case XK_Escape: return MG_KEY_ESC;
        case XK_Left: return MG_KEY_LEFT;
        case XK_Right: return MG_KEY_RIGHT;
        case XK_Up: return MG_KEY_UP;
        case XK_Down: return MG_KEY_DOWN;
        case XK_Delete: return MG_KEY_DELETE;
        case XK_Shift_L:
        case XK_Shift_R: return MG_KEY_SHIFT;
        case XK_Control_L:
        case XK_Control_R: return MG_KEY_CTRL;
        default: return 0;
    }
}

/* X11 keys for Cyrillic: their codes do NOT match Unicode. */
static unsigned int mg_cyr_keysym_to_unicode(KeySym ks)
{
    static const struct { unsigned int ks, cp; } t[66] = {
    {0x06A3u, 0x0451u},
    {0x06B3u, 0x0401u},
    {0x06C0u, 0x044Eu},
    {0x06C1u, 0x0430u},
    {0x06C2u, 0x0431u},
    {0x06C3u, 0x0446u},
    {0x06C4u, 0x0434u},
    {0x06C5u, 0x0435u},
    {0x06C6u, 0x0444u},
    {0x06C7u, 0x0433u},
    {0x06C8u, 0x0445u},
    {0x06C9u, 0x0438u},
    {0x06CAu, 0x0439u},
    {0x06CBu, 0x043Au},
    {0x06CCu, 0x043Bu},
    {0x06CDu, 0x043Cu},
    {0x06CEu, 0x043Du},
    {0x06CFu, 0x043Eu},
    {0x06D0u, 0x043Fu},
    {0x06D1u, 0x044Fu},
    {0x06D2u, 0x0440u},
    {0x06D3u, 0x0441u},
    {0x06D4u, 0x0442u},
    {0x06D5u, 0x0443u},
    {0x06D6u, 0x0436u},
    {0x06D7u, 0x0432u},
    {0x06D8u, 0x044Cu},
    {0x06D9u, 0x044Bu},
    {0x06DAu, 0x0437u},
    {0x06DBu, 0x0448u},
    {0x06DCu, 0x044Du},
    {0x06DDu, 0x0449u},
    {0x06DEu, 0x0447u},
    {0x06DFu, 0x044Au},
    {0x06E0u, 0x042Eu},
    {0x06E1u, 0x0410u},
    {0x06E2u, 0x0411u},
    {0x06E3u, 0x0426u},
    {0x06E4u, 0x0414u},
    {0x06E5u, 0x0415u},
    {0x06E6u, 0x0424u},
    {0x06E7u, 0x0413u},
    {0x06E8u, 0x0425u},
    {0x06E9u, 0x0418u},
    {0x06EAu, 0x0419u},
    {0x06EBu, 0x041Au},
    {0x06ECu, 0x041Bu},
    {0x06EDu, 0x041Cu},
    {0x06EEu, 0x041Du},
    {0x06EFu, 0x041Eu},
    {0x06F0u, 0x041Fu},
    {0x06F1u, 0x042Fu},
    {0x06F2u, 0x0420u},
    {0x06F3u, 0x0421u},
    {0x06F4u, 0x0422u},
    {0x06F5u, 0x0423u},
    {0x06F6u, 0x0416u},
    {0x06F7u, 0x0412u},
    {0x06F8u, 0x042Cu},
    {0x06F9u, 0x042Bu},
    {0x06FAu, 0x0417u},
    {0x06FBu, 0x0428u},
    {0x06FCu, 0x042Du},
    {0x06FDu, 0x0429u},
    {0x06FEu, 0x0427u},
    {0x06FFu, 0x042Au}
    };
    int i;
    for (i = 0; i < 66; ++i) if (t[i].ks == (unsigned int)ks) return t[i].cp;
    return 0;
}

static void mg_pump(MgWindow *w)
{
    XEvent ev;
    while (XPending(w->dpy)) {
        XNextEvent(w->dpy, &ev);
        switch (ev.type) {
            case ClientMessage:
                if ((Atom)ev.xclient.data.l[0] == w->wm_delete) w->quit = 1;
                break;
            case ConfigureNotify:
                mg_resize_internal(w, ev.xconfigure.width, ev.xconfigure.height);
                break;
            case ButtonPress:
                w->mouse_x = ev.xbutton.x;
                w->mouse_y = ev.xbutton.y;
                if (ev.xbutton.button == Button1) w->mouse_down = 1;
                if (ev.xbutton.button == Button3) w->mouse_rdown = 1;
                break;
            case ButtonRelease:
                w->mouse_x = ev.xbutton.x;
                w->mouse_y = ev.xbutton.y;
                if (ev.xbutton.button == Button1) w->mouse_down = 0;
                if (ev.xbutton.button == Button3) w->mouse_rdown = 0;
                break;
            case MotionNotify:
                w->mouse_x = ev.xmotion.x;
                w->mouse_y = ev.xmotion.y;
                break;
            case KeyPress:
            case KeyRelease: {
                KeySym ks = 0, ks2;
                char buf[16];
                int n = XLookupString(&ev.xkey, buf, (int)sizeof(buf), &ks, NULL);
                int is_down = (ev.type == KeyPress);
                int code;
                unsigned int cp = 0;
                unsigned int group = (unsigned int)XkbGroupForCoreState(ev.xkey.state);
                int level = (ev.xkey.state & ShiftMask) ? 1 : 0;

                /* Take the character code directly from XKB: Cyrillic keysyms
                 * match Unicode (0x410..0x45F), so no locale is needed. */
                ks2 = XkbKeycodeToKeysym(w->dpy, (unsigned int)ev.xkey.keycode,
                                         (int)group, level);
                if (!ks2) ks2 = ks;
                code = mg_keysym_to_code(ks2);
                if (!code) code = mg_keysym_to_code(ks);
                if (!code && n == 1 && (unsigned char)buf[0] < 32)
                    code = (unsigned char)buf[0];
                if (code > 0 && code < 512) w->keys[code] = (unsigned char)(is_down ? 1 : 0);

                {
                    unsigned int cyr = mg_cyr_keysym_to_unicode(ks2);
                    if (cyr) cp = cyr;
                    else if (ks2 >= 0x0401u && ks2 <= 0x044Fu) cp = (unsigned int)ks2;
                    else if (ks2 >= 0x20u && ks2 <= 0x7Eu) cp = (unsigned int)ks2;
                }
                if (!cp && n > 0) {
                    const char *p = buf;
                    unsigned int tmp = mg_utf8_next(&p);
                    if (tmp >= 32) cp = tmp;
                }
                if (cp > 0 && cp < 512)
                    w->keys[cp] = (unsigned char)(is_down ? 1 : 0);
                if (is_down && cp >= 32 && w->typed_len < 60) {
                    char tmp[4];
                    int nb = mg_utf8_encode(cp, tmp);
                    if (w->typed_len + nb <= 60) {
                        memcpy(w->typed + w->typed_len, tmp, (size_t)nb);
                        w->typed_len += nb;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    if (!w->mouse_down) w->drag_id = 0;
}

MgWindow *mg_create(const char *title, int width, int height)
{
    MgWindow *w = (MgWindow *)calloc(1, sizeof(MgWindow));
    if (!w) return NULL;

    /* Enable the system locale (usually C.UTF-8): XLookupString needs it. */
    setlocale(LC_ALL, "");

    w->dpy = XOpenDisplay(NULL);
    if (!w->dpy) { free(w); return NULL; }
    w->screen = DefaultScreen(w->dpy);

    w->xwin = XCreateSimpleWindow(w->dpy, RootWindow(w->dpy, w->screen),
                                  0, 0, (unsigned int)width, (unsigned int)height, 0,
                                  BlackPixel(w->dpy, w->screen), WhitePixel(w->dpy, w->screen));
    XStoreName(w->dpy, w->xwin, title);
    w->wm_delete = XInternAtom(w->dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(w->dpy, w->xwin, &w->wm_delete, 1);
    XSelectInput(w->dpy, w->xwin,
                 ExposureMask | StructureNotifyMask | ButtonPressMask |
                 ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask);
    w->gc = XCreateGC(w->dpy, w->xwin, 0, NULL);
    w->img = XCreateImage(w->dpy, DefaultVisual(w->dpy, w->screen),
                          24, ZPixmap, 0, NULL, (unsigned int)width, (unsigned int)height, 32, 0);
    if (!w->img) {
        XFreeGC(w->dpy, w->gc);
        XDestroyWindow(w->dpy, w->xwin);
        XCloseDisplay(w->dpy);
        free(w);
        return NULL;
    }
    if (!w->img->data) {
        w->img->data = (char *)malloc((size_t)w->img->bytes_per_line * (size_t)w->img->height);
        if (!w->img->data) {
            XDestroyImage(w->img);
            XFreeGC(w->dpy, w->gc);
            XDestroyWindow(w->dpy, w->xwin);
            XCloseDisplay(w->dpy);
            free(w);
            return NULL;
        }
    }
    w->pixels = (unsigned char *)w->img->data;
    w->width = width;
    w->height = height;

    XMapWindow(w->dpy, w->xwin);
    XFlush(w->dpy);
    return w;
}

void mg_destroy(MgWindow *w)
{
    if (!w) return;
    if (w->img) XDestroyImage(w->img);   /* also frees pixels */
    if (w->gc) XFreeGC(w->dpy, w->gc);
    if (w->xwin) XDestroyWindow(w->dpy, w->xwin);
    if (w->dpy) XCloseDisplay(w->dpy);
    free(w);
}

static int mg_wm_supports(MgWindow *w, const char *name)
{
    Atom prop = XInternAtom(w->dpy, "_NET_SUPPORTED", True);
    Atom need = XInternAtom(w->dpy, name, True);
    Atom type;
    int fmt, found = 0;
    unsigned long n = 0, after = 0;
    unsigned char *data = NULL;
    if (!prop || !need) return 0;
    if (XGetWindowProperty(w->dpy, RootWindow(w->dpy, w->screen), prop,
                           0, 4096, False, XA_ATOM, &type, &fmt, &n, &after,
                           &data) == Success && data) {
        Atom *list = (Atom *)(void *)data;
        unsigned long i;
        for (i = 0; i < n; ++i) if (list[i] == need) { found = 1; break; }
        XFree(data);
    }
    return found;
}

void mg_set_title(MgWindow *w, const char *title)
{
    if (w && title) { XStoreName(w->dpy, w->xwin, title); XFlush(w->dpy); }
}

void mg_set_pos(MgWindow *w, int x, int y)
{
    if (w) { XMoveWindow(w->dpy, w->xwin, x, y); XFlush(w->dpy); }
}

void mg_set_size(MgWindow *w, int width, int height)
{
    if (w && width > 0 && height > 0) {
        XResizeWindow(w->dpy, w->xwin, (unsigned int)width, (unsigned int)height);
        XFlush(w->dpy);
    }
}

int mg_width(MgWindow *w) { return w ? w->width : 0; }
int mg_height(MgWindow *w) { return w ? w->height : 0; }
int mg_is_fullscreen(MgWindow *w) { return w ? w->fullscreen : 0; }

void mg_fullscreen(MgWindow *w, int on)
{
    on = on ? 1 : 0;
    if (!w || w->fullscreen == on) return;
    if (on) {
        w->saved_width = w->width;
        w->saved_height = w->height;
    } else if (w->saved_width > 0 && w->saved_height > 0) {
        XResizeWindow(w->dpy, w->xwin, (unsigned int)w->saved_width,
                      (unsigned int)w->saved_height);
    }
    if (mg_wm_supports(w, "_NET_WM_STATE_FULLSCREEN")) {
        XEvent ev;
        Atom state = XInternAtom(w->dpy, "_NET_WM_STATE", False);
        Atom full = XInternAtom(w->dpy, "_NET_WM_STATE_FULLSCREEN", False);
        memset(&ev, 0, sizeof(ev));
        ev.xclient.type = ClientMessage;
        ev.xclient.window = w->xwin;
        ev.xclient.message_type = state;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = on ? 1 : 0;   /* 1 = ADD, 0 = REMOVE */
        ev.xclient.data.l[1] = (long)full;
        XSendEvent(w->dpy, RootWindow(w->dpy, w->screen), False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &ev);
    } else if (on) {
        Screen *scr = ScreenOfDisplay(w->dpy, w->screen);
        XMoveResizeWindow(w->dpy, w->xwin, 0, 0,
                          (unsigned int)scr->width, (unsigned int)scr->height);
    }
    w->fullscreen = on;
    XFlush(w->dpy);
}

void mg_end(MgWindow *w)
{
    XPutImage(w->dpy, w->xwin, w->gc, w->img, 0, 0, 0, 0,
              (unsigned int)w->width, (unsigned int)w->height);
    XFlush(w->dpy);
    w->mouse_prev_down = w->mouse_down;
    w->mouse_rprev_down = w->mouse_rdown;
    memcpy(w->prev_keys, w->keys, sizeof(w->keys));
}

void mg_sleep(int ms)
{
    struct timespec ts;
    if (ms <= 0) return;
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

unsigned int mg_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned int)((unsigned long long)ts.tv_sec * 1000ull +
                          (unsigned long long)ts.tv_nsec / 1000000ull);
}

#endif /* _WIN32 */

/* ========================== FRAME ========================= */

int mg_open(MgWindow *w)
{
    return w && !w->quit;
}

void mg_begin(MgWindow *w)
{
    w->typed_len = 0;
    mg_pump(w);
    if (mg_mouse_pressed(w)) w->focus_id = 0;
}

void mg_quit(MgWindow *w) { if (w) w->quit = 1; }

/* Window + loop + clearing + shutdown in a single function. */
int mg_run(const char *title, int width, int height,
           void (*drawing)(MgWindow *w, void *user), void *user)
{
    MgWindow *w = mg_create(title, width, height);
    if (!w) return 1;
    while (mg_open(w)) {
        mg_begin(w);
        mg_clear(w, mg_c_bg);
        if (drawing) drawing(w, user);
        mg_end(w);
        mg_sleep(16);
    }
    mg_destroy(w);
    return 0;
}

#endif /* MINIGUI_IMPLEMENTATION */
#endif /* MINIGUI_H */

#ifndef WS_GUI_H
#define WS_GUI_H

/*
 * WornewScript GUI runtime - a thin wrapper over minigui.h.
 * Windows are exposed as integer ids (1..16) so the language never deals with pointers.
 * Included by both the VM (C++) and the generated C code, so everything is static.
 */


#define WSG_MAX_WIN   16
#define WSG_MAX_FIELD 32

typedef struct {
    int used;
    int win;
    int x, y, rw, rh;
    char text[512];
} WsgField;

static MgWindow *wsg_windows_[WSG_MAX_WIN];
static WsgField   wsg_fields_[WSG_MAX_FIELD];

static MgWindow *wsg_win_(double id) {
    int i = (int)id - 1;
    return (i >= 0 && i < WSG_MAX_WIN) ? wsg_windows_[i] : 0;
}

/* --- window --- */
static double wsg_window(const char *title, double w, double h) {
    int i;
    for (i = 0; i < WSG_MAX_WIN; ++i) {
        if (!wsg_windows_[i]) {
            wsg_windows_[i] = mg_create(title, (int)w, (int)h);
            return wsg_windows_[i] ? (double)(i + 1) : 0;
        }
    }
    return 0;
}
static double wsg_open(double id)     { MgWindow *w = wsg_win_(id); return w ? mg_open(w) : 0; }
static void   wsg_begin(double id)    { MgWindow *w = wsg_win_(id); if (w) mg_begin(w); }
static void   wsg_end(double id)      { MgWindow *w = wsg_win_(id); if (w) mg_end(w); }
static void   wsg_clear(double id, double c) { MgWindow *w = wsg_win_(id); if (w) mg_clear(w, (MgColor)(unsigned int)c); }
static void   wsg_quit(double id)     { MgWindow *w = wsg_win_(id); if (w) mg_quit(w); }
static double wsg_width(double id)    { MgWindow *w = wsg_win_(id); return w ? mg_width(w) : 0; }
static double wsg_height(double id)   { MgWindow *w = wsg_win_(id); return w ? mg_height(w) : 0; }
static void   wsg_fullscreen(double id, double on) { MgWindow *w = wsg_win_(id); if (w) mg_fullscreen(w, on != 0); }
static double wsg_is_fullscreen(double id) { MgWindow *w = wsg_win_(id); return w ? mg_is_fullscreen(w) : 0; }

/* --- drawing --- */
static double wsg_rgb(double r, double g, double b) {
    return (double)MG_RGB((int)r, (int)g, (int)b);
}
static void wsg_rect(double id, double x, double y, double rw, double rh, double c) {
    MgWindow *w = wsg_win_(id);
    if (w) mg_rect(w, (int)x, (int)y, (int)rw, (int)rh, (MgColor)(unsigned int)c);
}
static void wsg_panel(double id, double x, double y, double rw, double rh) {
    MgWindow *w = wsg_win_(id);
    if (w) mg_panel(w, (int)x, (int)y, (int)rw, (int)rh);
}
static void wsg_label(double id, double x, double y, const char *text) {
    MgWindow *w = wsg_win_(id);
    if (w) mg_label(w, (int)x, (int)y, text);
}
static void wsg_label_ex(double id, double x, double y, const char *text, double c, double size) {
    MgWindow *w = wsg_win_(id);
    if (w) mg_label_ex(w, (int)x, (int)y, text, (MgColor)(unsigned int)c, (int)size);
}
static double wsg_button(double id, double x, double y, double rw, double rh, const char *text) {
    MgWindow *w = wsg_win_(id);
    return w ? mg_button(w, (int)x, (int)y, (int)rw, (int)rh, text) : 0;
}
static double wsg_checkbox(double id, double x, double y, const char *text, double checked) {
    MgWindow *w = wsg_win_(id);
    int v = (int)checked;
    if (!w) return v;
    mg_checkbox(w, (int)x, (int)y, text, &v);
    return v;
}

/* --- text field --- */
static const char *wsg_field(double id, double x, double y, double rw, double rh, const char *initial) {
    MgWindow *w = wsg_win_(id);
    int i, free_slot = -1;
    if (!w) return "";
    for (i = 0; i < WSG_MAX_FIELD; ++i) {
        WsgField *f = &wsg_fields_[i];
        if (!f->used) { if (free_slot < 0) free_slot = i; continue; }
        if (f->win == (int)id && f->x == (int)x && f->y == (int)y) {
            mg_textfield(w, f->x, f->y, f->rw, f->rh, f->text, (int)sizeof(f->text) - 1);
            return f->text;
        }
    }
    if (free_slot < 0) return "";
    {
        WsgField *f = &wsg_fields_[free_slot];
        f->used = 1;
        f->win = (int)id;
        f->x = (int)x; f->y = (int)y; f->rw = (int)rw; f->rh = (int)rh;
        snprintf(f->text, sizeof(f->text), "%s", initial ? initial : "");
        mg_textfield(w, f->x, f->y, f->rw, f->rh, f->text, (int)sizeof(f->text) - 1);
        return f->text;
    }
}

/* --- input --- */
static double wsg_mouse_x(double id)    { MgWindow *w = wsg_win_(id); return w ? mg_mouse_x(w) : 0; }
static double wsg_mouse_y(double id)    { MgWindow *w = wsg_win_(id); return w ? mg_mouse_y(w) : 0; }
static double wsg_mouse_down(double id) { MgWindow *w = wsg_win_(id); return w ? mg_mouse_down(w) : 0; }
static double wsg_mouse_pressed(double id) { MgWindow *w = wsg_win_(id); return w ? mg_mouse_pressed(w) : 0; }
static double wsg_key_down(double id, double code) { MgWindow *w = wsg_win_(id); return w ? mg_key_down(w, (int)code) : 0; }
static double wsg_key_pressed(double id, double code) { MgWindow *w = wsg_win_(id); return w ? mg_key_pressed(w, (int)code) : 0; }

/* --- font --- */
static void   wsg_font(double size, double c) { mg_set_font((int)size, (MgColor)(unsigned int)c); }
static double wsg_font_size(void) { return mg_get_font_size(); }

#endif /* WS_GUI_H */
)WS_GUI_RUNTIME";
