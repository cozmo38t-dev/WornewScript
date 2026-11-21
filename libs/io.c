#ifndef WS_LIB_IO_C
#define WS_LIB_IO_C
/* io.c — small numeric output helpers */
static double ws_c_sum2(double a, double b) { return a + b; }
static double ws_c_sign(double x) { return x > 0 ? 1 : (x < 0 ? -1 : 0); }
#endif
