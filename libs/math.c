#ifndef WS_LIB_MATH_C
#define WS_LIB_MATH_C
/* math.c — math */
#include <math.h>

static double ws_c_square(double x) { return x * x; }
static double ws_c_fact(double n) {
    double r = 1;
    while (n > 1) { r *= n; n -= 1; }
    return r;
}
static double ws_c_gcd(double a, double b) {
    /* fmod instead of %: works with fractions too and won't fail when b == 0. */
    a = fabs(a);
    b = fabs(b);
    while (b > 1e-9) {
        double t = fmod(a, b);
        a = b;
        b = t;
    }
    return a;
}
#endif
