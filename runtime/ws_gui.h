#ifndef WS_GUI_H
#define WS_GUI_H

/*
 * WornewScript GUI runtime - a thin wrapper over minigui.h.
 * Windows are exposed as integer ids (1..16) so the language never deals with pointers.
 * Included by both the VM (C++) and the generated C code, so everything is static.
 */

#include "minigui.h"

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
