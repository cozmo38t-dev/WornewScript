#ifndef WS_LIB_DLC_C
#define WS_LIB_DLC_C
/* dlc.c — delays (real time, no busy-wait) */
#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include <time.h>
#else
#include <windows.h>
#endif

static double ws_c_sleep(double ms) {
    if (ms <= 0) return 0;
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    {
        struct timespec ts;
        ts.tv_sec = (time_t)(ms / 1000.0);
        ts.tv_nsec = (long)((ms - (double)ts.tv_sec * 1000.0) * 1000000.0);
        if (ts.tv_nsec >= 1000000000L) { ++ts.tv_sec; ts.tv_nsec -= 1000000000L; }
        while (nanosleep(&ts, &ts) == -1) { }
    }
#endif
    return ms;
}

static double ws_c_ticks(void) {
#ifdef _WIN32
    return (double)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
#endif
}
#endif
