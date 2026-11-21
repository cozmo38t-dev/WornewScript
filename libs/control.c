#ifndef WS_LIB_CONTROL_C
#define WS_LIB_CONTROL_C
/* control.c — bounds and checks */
static double ws_c_clamp(double x, double lo, double hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}
static double ws_c_between(double x, double lo, double hi) {
    return (x >= lo && x <= hi) ? 1 : 0;
}
#endif
