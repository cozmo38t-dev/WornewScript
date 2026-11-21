#ifndef WS_LIB_COL_C
#define WS_LIB_COL_C
/* col.c — color packing (CurrtTalk style: included as a source file) */
#define WS_C_RGB(r,g,b) (0xFF000000u | ((unsigned int)(r) << 16) | ((unsigned int)(g) << 8) | (unsigned int)(b))
static double ws_c_rgb(double r, double g, double b) {
    return (double)WS_C_RGB((int)r, (int)g, (int)b);
}
static double ws_c_gray(double v) { return ws_c_rgb(v, v, v); }
#endif
