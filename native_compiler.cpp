#include "native_compiler.h"
#include "runtime/gui_embedded.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

static const char* RUNTIME = R"WS(
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <stddef.h>

#define MAX_STACK 8192
#define MAX_FRAMES 128
#define MAX_LOCALS 256

typedef struct { double num; const char *str; int is_str; } Value;

static Value mk_num(double n) { Value v; v.num = n; v.str = NULL; v.is_str = 0; return v; }
static Value mk_str(const char *s) { Value v; v.num = 0; v.str = s; v.is_str = 1; return v; }
static double as_num(Value v) { return v.is_str ? (v.str ? atof(v.str) : 0.0) : v.num; }
static int truthy(Value v) {
    if (v.is_str) return v.str != NULL && v.str[0] != '\0';
    return v.num != 0.0;
}
static const char *dup_str(const char *s) {
    size_t n = strlen(s); char *p = (char *)malloc(n + 1);
    if (p) memcpy(p, s, n + 1);
    return p;
}
static void fmt_val(char *out, size_t cap, Value v) {
    if (v.is_str) snprintf(out, cap, "%s", v.str ? v.str : "");
    else snprintf(out, cap, "%.15g", v.num);
}
static void print_val(Value v) {
    char buf[1024]; fmt_val(buf, sizeof(buf), v); puts(buf);
}
static Value concat_vals(Value a, Value b) {
    char sa[512], sb[512], out[1100];
    fmt_val(sa, sizeof(sa), a); fmt_val(sb, sizeof(sb), b);
    snprintf(out, sizeof(out), "%s%s", sa, sb);
    return mk_str(dup_str(out));
}
static const char *ws_read_str(void) {
    char b[1024];
    if (!fgets(b, sizeof b, stdin)) b[0] = '\0';
    size_t n = strlen(b);
    while (n && (b[n-1] == '\n' || b[n-1] == '\r')) b[--n] = '\0';
    return dup_str(b);
}
static double ws_read_num(void) {
    char b[256];
    if (!fgets(b, sizeof b, stdin)) return 0;
    return atof(b);
}
static double ws_read_int(void) {
    char b[256];
    if (!fgets(b, sizeof b, stdin)) return 0;
    return (double)strtoll(b, NULL, 10);
}
static double ws_read_bool(void) {
    char b[256];
    if (!fgets(b, sizeof b, stdin)) return 0;
    size_t n = strlen(b);
    while (n && (b[n-1] == '\n' || b[n-1] == '\r')) b[--n] = '\0';
    for (size_t i = 0; i < n; ++i) b[i] = (char)tolower((unsigned char)b[i]);
    return (b[0] == '\0' || strcmp(b, "0") == 0 || strcmp(b, "false") == 0 ||
            strcmp(b, "no") == 0 || strcmp(b, "нет") == 0) ? 0 : 1;
}

typedef struct {
    const int *code;
    const int *base;
    int ip;
    Value locals[MAX_LOCALS];
} Frame;

typedef struct {
    Value globals[NGLOBALS];
    Value stack[MAX_STACK];
    Frame frames[MAX_FRAMES];
    int sp;
    int ftop;
    unsigned int seed;
    const int *fns[NFUNCS];
} VM;

static const char *str_arg(int argc, Value *a, int i) {
    return (argc > i && a[i].is_str && a[i].str) ? a[i].str : "";
}

static Value builtin(VM *vm, int id, int argc, Value *a) {
#define NA(i) (argc > (i) ? as_num(a[i]) : 0.0)
#define NS(i) str_arg(argc, a, i)
    double x = NA(0), y = NA(1);
    switch (id) {
        case 0: return mk_num(sqrt(x));
        case 1: return mk_num(fabs(x));
        case 2: return mk_num(pow(x, y));
        case 3: return mk_num(x < y ? x : y);
        case 4: return mk_num(x > y ? x : y);
        case 5: {
            char buf[128];
            if (argc > 0 && a[0].is_str) return mk_num((double)(a[0].str ? strlen(a[0].str) : 0));
            snprintf(buf, sizeof(buf), "%.15g", x);
            return mk_num((double)strlen(buf));
        }
        case 6: return mk_num(trunc(x));
        case 7: {
            char buf[512];
            fmt_val(buf, sizeof(buf), argc > 0 ? a[0] : mk_num(0));
            return mk_str(dup_str(buf));
        }
        case 8: return mk_num((double)clock() * 1000.0 / CLOCKS_PER_SEC);
        case 9: {
            vm->seed = vm->seed * 1103515245u + 12345u;
            return mk_num((double)(vm->seed & 0x7fffffffu) / 2147483648.0);
        }
        case 10: return mk_num(sin(x));
        case 11: return mk_num(cos(x));
        case 12: return mk_num(tan(x));
        case 13: return mk_num(floor(x));
        case 14: return mk_num(ceil(x));
        case 15: return mk_num(round(x));
        case 16: return mk_num(log(x));
        case 17: return mk_num(log10(x));
        case 18: return mk_num(exp(x));
        case 19: return mk_num(x > 0 ? 1 : (x < 0 ? -1 : 0));
        case 20: return mk_num(x < NA(1) ? NA(1) : (x > NA(2) ? NA(2) : x));
        /* --- GUI 21..47 --- */
        case 21: return mk_num(wsg_window(NS(0), NA(1), NA(2)));
        case 22: return mk_num(wsg_open(x));
        case 23: wsg_begin(x); return mk_num(0);
        case 24: wsg_end(x); return mk_num(0);
        case 25: wsg_clear(x, y); return mk_num(0);
        case 26: wsg_quit(x); return mk_num(0);
        case 27: return mk_num(wsg_width(x));
        case 28: return mk_num(wsg_height(x));
        case 29: wsg_fullscreen(x, y); return mk_num(0);
        case 30: return mk_num(wsg_is_fullscreen(x));
        case 31: return mk_num(wsg_rgb(x, y, NA(2)));
        case 32: wsg_rect(x, y, NA(2), NA(3), NA(4), NA(5)); return mk_num(0);
        case 33: wsg_panel(x, y, NA(2), NA(3), NA(4)); return mk_num(0);
        case 34: wsg_label(x, y, NA(2), NS(3)); return mk_num(0);
        case 35: wsg_label_ex(x, y, NA(2), NS(3), NA(4), NA(5)); return mk_num(0);
        case 36: return mk_num(wsg_button(x, y, NA(2), NA(3), NA(4), NS(5)));
        case 37: return mk_num(wsg_checkbox(x, y, NA(2), NS(3), NA(4)));
        case 38: return mk_str(dup_str(wsg_field(x, y, NA(2), NA(3), NA(4), NS(5))));
        case 39: return mk_num(wsg_mouse_x(x));
        case 40: return mk_num(wsg_mouse_y(x));
        case 41: return mk_num(wsg_mouse_down(x));
        case 42: return mk_num(wsg_mouse_pressed(x));
        case 43: return mk_num(wsg_key_down(x, y));
        case 44: return mk_num(wsg_key_pressed(x, y));
        case 45: wsg_font(x, y); return mk_num(0);
        case 46: return mk_num(wsg_font_size());
        case 47: mg_sleep((int)x); return mk_num(0);
        /* --- C libraries 48..80 (sources are injected via $APT) --- */
        case 48: return mk_num(ws_c_rgb(x, y, NA(2)));
        case 49: return mk_num(ws_c_gray(x));
        case 50: return mk_num(ws_c_clamp(x, NA(1), NA(2)));
        case 51: return mk_num(ws_c_between(x, NA(1), NA(2)));
        case 52: return mk_num(ws_c_c2f(x));
        case 53: return mk_num(ws_c_f2c(x));
        case 54: return mk_num(ws_c_sleep(x));
        case 55: return mk_num(ws_c_ticks());
        case 56: return mk_num(ws_c_sum2(x, y));
        case 57: return mk_num(ws_c_sign(x));
        case 58: return mk_num(ws_c_key_esc());
        case 59: return mk_num(ws_c_key_enter());
        case 60: return mk_num(ws_c_square(x));
        case 61: return mk_num(ws_c_gcd(x, y));
        case 62: return mk_num(ws_c_fact(x));
        case 63: return mk_num(ws_c_randint(x, y));
        case 64: return mk_num(ws_c_randrange(x, y));
        case 65: return mk_num(ws_c_exists(NS(0)));
        case 66: return mk_num(ws_c_is_abs(NS(0)));
        case 67: return mk_num(ws_c_now());
        case 68: return mk_num(ws_c_elapsed(x));
        case 69: return mk_num(ws_c_cls());
        case 70: return mk_num(ws_c_title(NS(0)));
        case 71: return mk_num(ws_c_strlen(NS(0)));
        case 72: return mk_num(ws_c_padlen(NS(0), NA(1)));
        case 73: return mk_num(ws_c_mean3(x, y, NA(2)));
        case 74: return mk_num(ws_c_med3(x, y, NA(2)));
        case 75: return mk_num(ws_c_discount(x, y));
        case 76: return mk_num(ws_c_vat(x, y));
        case 77: return mk_num(ws_c_force(x, y));
        case 78: return mk_num(ws_c_fall(x));
        case 79: return mk_num(ws_c_circle_area(x));
        case 80: return mk_num(ws_c_dist(x, y, NA(2), NA(3)));
        default: return mk_num(0);
    }
#undef NA
#undef NS
}

static void vm_run(VM *vm, const int *main_code) {
    const int *code = main_code;
    const int *base = main_code;
    Value *locals = vm->frames[0].locals;
    vm->sp = 0;
    vm->ftop = 0;
    vm->seed = 12345u;   /* deterministic RNG, as in the VM and direct C */

    for (;;) {
        int op = *code++;
        switch (op) {
            case 0: break;
            case 1: vm->stack[vm->sp++] = mk_num(NUMS[*code++]); break;
            case 2: vm->stack[vm->sp++] = mk_str(STRS[*code++]); break;
            case 3: vm->stack[vm->sp++] = vm->globals[*code++]; break;
            case 4: vm->globals[*code++] = vm->stack[--vm->sp]; break;
            case 5: vm->stack[vm->sp++] = locals[*code++]; break;
            case 6: locals[*code++] = vm->stack[--vm->sp]; break;
            case 7: { Value b = vm->stack[--vm->sp], a = vm->stack[--vm->sp];
                      vm->stack[vm->sp++] = (a.is_str || b.is_str) ? concat_vals(a,b) : mk_num(a.num + b.num); break; }
            case 8: { double b = as_num(vm->stack[--vm->sp]), a = as_num(vm->stack[--vm->sp]);
                      vm->stack[vm->sp++] = mk_num(a - b); break; }
            case 9: { double b = as_num(vm->stack[--vm->sp]), a = as_num(vm->stack[--vm->sp]);
                      vm->stack[vm->sp++] = mk_num(a * b); break; }
            case 10: { double b = as_num(vm->stack[--vm->sp]), a = as_num(vm->stack[--vm->sp]);
                       vm->stack[vm->sp++] = mk_num(b == 0 ? 0 : a / b); break; }
            case 11: { double b = as_num(vm->stack[--vm->sp]), a = as_num(vm->stack[--vm->sp]);
                       vm->stack[vm->sp++] = mk_num(b == 0 ? 0 : fmod(a, b)); break; }
            case 12: vm->stack[vm->sp-1] = mk_num(-as_num(vm->stack[vm->sp-1])); break;
            case 13: vm->stack[vm->sp-1] = mk_num(truthy(vm->stack[vm->sp-1]) ? 0 : 1); break;
            case 14: case 15: { Value b = vm->stack[--vm->sp], a = vm->stack[--vm->sp];
                       int eq = (a.is_str || b.is_str) ? strcmp(a.str?a.str:"", b.str?b.str:"") == 0
                                                       : a.num == b.num;
                       vm->stack[vm->sp++] = mk_num(((op == 14) == (eq != 0)) ? 1 : 0); break; }
            case 16: case 17: case 18: case 19: {
                       Value b = vm->stack[--vm->sp], a = vm->stack[--vm->sp];
                       int strcmp_mode = (a.is_str || b.is_str);
                       int cmp = strcmp_mode ? strcmp(a.str?a.str:"", b.str?b.str:"")
                                             : (a.num < b.num ? -1 : (a.num > b.num ? 1 : 0));
                       int r = (op == 16) ? cmp < 0 : (op == 17) ? cmp <= 0
                             : (op == 18) ? cmp > 0 : cmp >= 0;
                       vm->stack[vm->sp++] = mk_num(r ? 1 : 0); break; }
            case 20: { Value b = vm->stack[--vm->sp], a = vm->stack[--vm->sp];
                       vm->stack[vm->sp++] = mk_num(truthy(a) && truthy(b) ? 1 : 0); break; }
            case 21: { Value b = vm->stack[--vm->sp], a = vm->stack[--vm->sp];
                       vm->stack[vm->sp++] = mk_num(truthy(a) || truthy(b) ? 1 : 0); break; }
            case 22: code = base + *code; break;
            case 23: { int t = *code++; if (!truthy(vm->stack[--vm->sp])) code = base + t; break; }
            case 24: { int t = *code++; if ( truthy(vm->stack[--vm->sp])) code = base + t; break; }
            case 25: print_val(vm->stack[--vm->sp]); break;
            case 26: {
                       int fid = *code++, argc = *code++;
                       if (vm->ftop >= MAX_FRAMES) { fprintf(stderr, "recursion too deep\n"); return; }
                       Frame *fr = &vm->frames[vm->ftop++];
                       fr->code = code; fr->base = base; fr->ip = 0;
                       memset(fr->locals, 0, sizeof(fr->locals));
                       for (int i = argc - 1; i >= 0; --i) {
                           Value v = vm->stack[--vm->sp];
                           if (i < MAX_LOCALS) fr->locals[i] = v;
                       }
                       locals = fr->locals;
                       code = vm->fns[fid]; base = code;
                       break; }
            case 27: {
                       if (vm->ftop == 0) return;
                       Frame *fr = &vm->frames[--vm->ftop];
                       code = fr->code; base = fr->base;
                       locals = (vm->ftop > 0) ? vm->frames[vm->ftop-1].locals : vm->frames[0].locals;
                       break; }
            case 28: return;
            case 29: {
                       int bid = *code++, argc = *code++;
                       Value a[8];
                       int n = argc < 8 ? argc : 8;
                       for (int i = argc - 1; i >= 0; --i) {
                           Value v = vm->stack[--vm->sp];
                           if (i < n) a[i] = v;
                       }
                       vm->stack[vm->sp++] = builtin(vm, bid, n, a);
                       break; }
            case 30: --vm->sp; break;
            case 31: {
                        int tid = *code++, hp = *code++;
                        if (hp) { Value p = vm->stack[--vm->sp];
                                  char buf[1024]; fmt_val(buf, sizeof(buf), p);
                                  printf("%s", buf); fflush(stdout); }
                        if (tid == 2) vm->stack[vm->sp++] = mk_str(ws_read_str());
                        else if (tid == 3) vm->stack[vm->sp++] = mk_num(ws_read_bool());
                        else if (tid == 0) vm->stack[vm->sp++] = mk_num(ws_read_int());
                        else vm->stack[vm->sp++] = mk_num(ws_read_num());
                        break; }
        }
    }
}

int main(void) {
    static VM vm;
    for (int i = 0; i < NFUNCS; ++i) vm.fns[i] = FNS[i];
    vm_run(&vm, MAIN_CODE);
    return 0;
}
)WS";

static std::string c_num(double v) {
    std::ostringstream oss;
    oss << std::setprecision(17) << v;
    std::string s = oss.str();
    if (s.find('.') == std::string::npos &&
        s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos) {
        s += ".0";
    }
    return s;
}

static std::string c_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '\0': out += "\\0"; break;
            default: out += c; break;
        }
    }
    return out;
}

static void append_code(std::ostringstream& oss, const std::vector<int>& code) {
    for (size_t i = 0; i < code.size(); ++i) {
        if (i % 20 == 0) oss << "\n    ";
        oss << code[i] << ',';
    }
}

std::string NativeCompiler::generate(const BytecodeProgram& prog) {
    std::ostringstream oss;
    oss << "#include <stddef.h>\n";
    oss << "#define NGLOBALS " << std::max(1, static_cast<int>(prog.names.size())) << "\n";
    oss << "#define NFUNCS " << std::max(1, static_cast<int>(prog.functions.size())) << "\n\n";

    oss << "static const double NUMS[] = {";
    for (size_t i = 0; i < prog.numbers.size(); ++i) {
        if (i % 12 == 0) oss << "\n    ";
        oss << c_num(prog.numbers[i]) << ',';
    }
    oss << "\n};\n";

    oss << "static const char *STRS[] = {";
    for (size_t i = 0; i < prog.strings.size(); ++i) {
        if (i % 8 == 0) oss << "\n    ";
        oss << '"' << c_escape(prog.strings[i]) << "\",";
    }
    oss << "\n};\n";

    oss << "static const int MAIN_CODE[] = {";
    append_code(oss, prog.main_code);
    oss << "\n};\n";

    for (size_t i = 0; i < prog.functions.size(); ++i) {
        oss << "static const int F" << i << "_CODE[] = {";
        append_code(oss, prog.functions[i].code);
        oss << "\n};\n";
    }

    oss << "static const int *FNS[] = {";
    if (prog.functions.empty()) oss << "NULL";
    else for (size_t i = 0; i < prog.functions.size(); ++i) oss << 'F' << i << "_CODE,";
    oss << "};\n";

    oss << RUNTIME;
    return oss.str();
}

bool NativeCompiler::build(const BytecodeProgram& prog, const std::string& output, std::string& message) {
    return build_source(generate(prog), output, message);
}

bool NativeCompiler::build_source(const std::string& source, const std::string& output, std::string& message) {
    std::filesystem::path cpath_p(output);
    cpath_p.replace_extension(".c");
    std::string cpath = cpath_p.string();
    {
        std::ofstream f(cpath);
        // The GUI runtime is embedded in every binary: the generated C
        // needs no external headers, only -lX11.
        f << WS_GUI_SOURCE << "\n" << source;
    }

    std::string err_path = output + ".gccerr";
#ifdef _WIN32
    const char* gui_libs = " -D_WIN32_WINNT=0x0601 -lgdi32 -luser32";
#else
    const char* gui_libs = " -lX11";
#endif
    std::string cmd = "gcc -O3 -std=gnu11 \"" + cpath + "\" -o \"" + output +
                      "\" -lm" + gui_libs + " 2> \"" + err_path + "\"";
    int rc = std::system(cmd.c_str());

    {
        std::ifstream f(err_path);
        std::ostringstream oss;
        oss << f.rdbuf();
        message = oss.str();
    }
    std::remove(err_path.c_str());

    return rc == 0;
}
