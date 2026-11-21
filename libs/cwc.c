#ifndef WS_LIB_CWC_C
#define WS_LIB_CWC_C
/* cwc.c — temperature conversion */
static double ws_c_c2f(double c) { return c * 9.0 / 5.0 + 32.0; }
static double ws_c_f2c(double f) { return (f - 32.0) * 5.0 / 9.0; }
#endif
