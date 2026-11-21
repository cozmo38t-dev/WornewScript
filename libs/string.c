#ifndef WS_LIB_STRING_C
#define WS_LIB_STRING_C
/* string.c — strings */
#include <string.h>
static double ws_c_strlen(const char *s) { return (double)(s ? strlen(s) : 0); }
static double ws_c_padlen(const char *s, double width) {
    double n = width - ws_c_strlen(s);
    return n > 0 ? n : 0;
}
#endif
