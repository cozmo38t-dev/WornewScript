#ifndef WS_LIB_RANDNUM_C
#define WS_LIB_RANDNUM_C
/* randnum.c — random numbers.
 * A deterministic LCG with a fixed seed: identical results
 * in the VM, direct C, and the bytecode native. */
static unsigned int ws_rng_state = 12345u;

static double ws_rng_next(void) {
    ws_rng_state = ws_rng_state * 1103515245u + 12345u;
    return (double)(ws_rng_state & 0x7fffffffu) / 2147483648.0;
}

static double ws_c_randrange(double lo, double hi) {
    return lo + ws_rng_next() * (hi - lo);
}
static double ws_c_randint(double lo, double hi) {
    return (double)(int)ws_c_randrange(lo, hi + 1);
}
#endif
