#ifndef WS_LIB_SYSFILE_C
#define WS_LIB_SYSFILE_C
/* sysfile.c — file handling */
#include <stdio.h>
static double ws_c_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}
static double ws_c_is_abs(const char *path) {
    return (path && (path[0] == '/' || path[0] == '\\')) ? 1 : 0;
}
#endif
