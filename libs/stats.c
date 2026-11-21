#ifndef WS_LIB_STATS_C
#define WS_LIB_STATS_C
/* stats.c — statistics */
static double ws_c_mean3(double a, double b, double c) { return (a + b + c) / 3.0; }
static double ws_c_med3(double a, double b, double c) {
    double t;
    if (a > b) { t = a; a = b; b = t; }
    if (b > c) { t = b; b = c; c = t; }
    if (a > b) { t = a; a = b; b = t; }
    return b;
}
#endif
