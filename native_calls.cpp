#include "native_calls.h"

/* C libraries are included directly here: this way the VM sees the same implementations
 * as the generated C, but without external dependencies. */
#include "libs/col.c"
#include "libs/control.c"
#include "libs/cwc.c"
#include "libs/dlc.c"
#include "libs/io.c"
#include "libs/keyboard.c"
#include "libs/math.c"
#include "libs/randnum.c"
#include "libs/sysfile.c"
#include "libs/time.c"
#include "libs/winapi.c"
#include "libs/string.c"
#include "libs/stats.c"
#include "libs/money.c"
#include "libs/physics.c"
#include "libs/geometry.c"

#include <cmath>
#include <string>

static std::string arg_text(const Value& v) {
    if (v.is_str()) return v.as_text();
    return v.as_text();
}

Value call_native_builtin(const std::string& name, const Value* args, int argc) {
    auto n = [&](int i) -> double {
        return (args && i < argc) ? args[i].as_num() : 0.0;
    };
    auto s = [&](int i) -> std::string {
        return (args && i < argc) ? arg_text(args[i]) : std::string();
    };

    if (name == "c_rgb")         return Value::number(ws_c_rgb(n(0), n(1), n(2)));
    if (name == "c_gray")        return Value::number(ws_c_gray(n(0)));
    if (name == "c_clamp")       return Value::number(ws_c_clamp(n(0), n(1), n(2)));
    if (name == "c_between")     return Value::number(ws_c_between(n(0), n(1), n(2)));
    if (name == "c_c2f")         return Value::number(ws_c_c2f(n(0)));
    if (name == "c_f2c")         return Value::number(ws_c_f2c(n(0)));
    if (name == "c_sleep")       return Value::number(ws_c_sleep(n(0)));
    if (name == "c_ticks")       return Value::number(ws_c_ticks());
    if (name == "c_sum2")        return Value::number(ws_c_sum2(n(0), n(1)));
    if (name == "c_sign")        return Value::number(ws_c_sign(n(0)));
    if (name == "c_key_esc")     return Value::number(ws_c_key_esc());
    if (name == "c_key_enter")   return Value::number(ws_c_key_enter());
    if (name == "c_square")      return Value::number(ws_c_square(n(0)));
    if (name == "c_gcd")         return Value::number(ws_c_gcd(n(0), n(1)));
    if (name == "c_fact")        return Value::number(ws_c_fact(n(0)));
    if (name == "c_randint")     return Value::number(ws_c_randint(n(0), n(1)));
    if (name == "c_randrange")   return Value::number(ws_c_randrange(n(0), n(1)));
    if (name == "c_exists")      return Value::number(ws_c_exists(s(0).c_str()));
    if (name == "c_is_abs")      return Value::number(ws_c_is_abs(s(0).c_str()));
    if (name == "c_now")         return Value::number(ws_c_now());
    if (name == "c_elapsed")     return Value::number(ws_c_elapsed(n(0)));
    if (name == "c_cls")         return Value::number(ws_c_cls());
    if (name == "c_title")       return Value::number(ws_c_title(s(0).c_str()));
    if (name == "c_strlen")      return Value::number(ws_c_strlen(s(0).c_str()));
    if (name == "c_padlen")      return Value::number(ws_c_padlen(s(0).c_str(), n(1)));
    if (name == "c_mean3")       return Value::number(ws_c_mean3(n(0), n(1), n(2)));
    if (name == "c_med3")        return Value::number(ws_c_med3(n(0), n(1), n(2)));
    if (name == "c_discount")    return Value::number(ws_c_discount(n(0), n(1)));
    if (name == "c_vat")         return Value::number(ws_c_vat(n(0), n(1)));
    if (name == "c_force")       return Value::number(ws_c_force(n(0), n(1)));
    if (name == "c_fall")        return Value::number(ws_c_fall(n(0)));
    if (name == "c_circle_area") return Value::number(ws_c_circle_area(n(0)));
    if (name == "c_dist")        return Value::number(ws_c_dist(n(0), n(1), n(2), n(3)));
    return Value();
}
