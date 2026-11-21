#ifndef WS_LIB_GEOMETRY_C
#define WS_LIB_GEOMETRY_C
/* geometry.c — geometry */
#include <math.h>
static double ws_c_circle_area(double r) { return 3.141592653589793 * r * r; }
static double ws_c_dist(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1, dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}
#endif
