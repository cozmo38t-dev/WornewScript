#ifndef WS_LIB_TIME_C
#define WS_LIB_TIME_C
/* time.c — time */
#include <time.h>
static double ws_c_now(void) { return (double)time(NULL) * 1000.0; }
static double ws_c_elapsed(double start) { return ws_c_now() - start; }
#endif
