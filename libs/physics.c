#ifndef WS_LIB_PHYSICS_C
#define WS_LIB_PHYSICS_C
/* physics.c — mechanics */
static double ws_c_force(double m, double a) { return m * a; }
static double ws_c_fall(double t) { return 0.5 * 9.81 * t * t; }
#endif
