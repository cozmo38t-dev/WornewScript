#ifndef WS_LIB_WINAPI_C
#define WS_LIB_WINAPI_C
/* winapi.c — console "windows" (ANSI), no external libraries */
#include <stdio.h>
static double ws_c_cls(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
    return 0;
}
static double ws_c_title(const char *text) {
    printf("\033]0;%s\007", text ? text : "");
    fflush(stdout);
    return 0;
}
#endif
