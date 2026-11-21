using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;

/* Generates C source with bytecode and a compact VM, then invokes gcc.
 * The result is a native binary without the .NET runtime. */
public static class NativeCompiler {
    private const string Header = """
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
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
    int seed;
    const int *fns[NFUNCS];
} VM;

""";

    private const string Runtime = """

static Value builtin(VM *vm, int id, int argc, Value *a) {
    double x = argc > 0 ? as_num(a[0]) : 0.0;
    double y = argc > 1 ? as_num(a[1]) : 0.0;
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
            char buf[512]; fmt_val(buf, sizeof(buf), a[0]);
            return mk_str(dup_str(buf));
        }
        case 8: return mk_num((double)clock());
        case 9: {
            vm->seed = vm->seed * 1103515245 + 12345;
            return mk_num((double)((unsigned int)vm->seed & 0x7fffffff) / 2147483648.0);
        }
        default: return mk_num(0);
    }
}

static void vm_run(VM *vm, const int *main_code) {
    const int *code = main_code;
    const int *base = main_code;
    Value *locals = vm->frames[0].locals;
    vm->sp = 0;
    vm->ftop = 0;
    vm->seed = (int)time(NULL);

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
                       vm->stack[vm->sp++] = mk_num((op == 14) == (eq != 0) ? 1 : 0); break; }
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
                       Frame *fr = &vm->frames[vm->ftop++];
                       fr->code = code; fr->base = base; fr->ip = 0;
                       memset(fr->locals, 0, sizeof(fr->locals));
                       for (int i = 0; i < argc; ++i) fr->locals[i] = vm->stack[--vm->sp];
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
                       for (int i = 0; i < argc && i < 8; ++i) a[i] = vm->stack[--vm->sp];
                       vm->stack[vm->sp++] = builtin(vm, bid, argc, a);
                       break; }
            case 30: --vm->sp; break;
        }
    }
}

int main(void) {
    static VM vm;
    for (int i = 0; i < NFUNCS; ++i) vm.fns[i] = FNS[i];
    vm_run(&vm, MAIN_CODE);
    return 0;
}
""";

    public static string Generate(BytecodeProgram p) {
        var sb = new StringBuilder(Runtime.Length + Header.Length + 4096);
        sb.Append("#include <stddef.h>\n");
        sb.Append("#define NGLOBALS ").Append(Math.Max(1, p.Names.Count)).Append('\n');
        sb.Append("#define NFUNCS ").Append(Math.Max(1, p.Functions.Count)).Append('\n');
        sb.Append('\n');

        sb.Append("static const double NUMS[] = {");
        for (int i = 0; i < p.Numbers.Count; i++) {
            if (i % 12 == 0) sb.Append("\n    ");
            sb.Append(CNum(p.Numbers[i])).Append(',');
        }
        sb.Append("\n};\n");

        sb.Append("static const char *STRS[] = {");
        for (int i = 0; i < p.Strings.Count; i++) {
            if (i % 8 == 0) sb.Append("\n    ");
            sb.Append('"').Append(CEscape(p.Strings[i])).Append("\",");
        }
        sb.Append("\n};\n");

        sb.Append("static const int MAIN_CODE[] = {");
        AppendCode(sb, p.MainCode);
        sb.Append("\n};\n");

        for (int i = 0; i < p.Functions.Count; i++) {
            sb.Append("static const int F").Append(i).Append("_CODE[] = {");
            AppendCode(sb, p.Functions[i].Code);
            sb.Append("\n};\n");
        }

        sb.Append("static const int *FNS[] = {");
        if (p.Functions.Count == 0) sb.Append("NULL");
        else for (int i = 0; i < p.Functions.Count; i++) sb.Append("F").Append(i).Append("_CODE,");
        sb.Append("};\n\n");

        sb.Append(Header);
        sb.Append(Runtime);
        return sb.ToString();
    }

    private static void AppendCode(StringBuilder sb, List<int> code) {
        for (int i = 0; i < code.Count; i++) {
            if (i % 20 == 0) sb.Append("\n    ");
            sb.Append(code[i]).Append(',');
        }
    }

    private static string CNum(double v) {
        string s = v.ToString("R", CultureInfo.InvariantCulture);
        if (s.IndexOf('.') < 0 && s.IndexOf('e') < 0 && s.IndexOf('E') < 0) s += ".0";
        return s;
    }

    private static string CEscape(string s) {
        var sb = new StringBuilder(s.Length + 8);
        foreach (char c in s) {
            switch (c) {
                case '\\': sb.Append("\\\\"); break;
                case '"': sb.Append("\\\""); break;
                case '\n': sb.Append("\\n"); break;
                case '\t': sb.Append("\\t"); break;
                case '\r': sb.Append("\\r"); break;
                case '\0': sb.Append("\\0"); break;
                default: sb.Append(c); break;
            }
        }
        return sb.ToString();
    }

    public static bool Build(BytecodeProgram p, string output, out string message) {
        string cpath = Path.ChangeExtension(output, ".c");
        File.WriteAllText(cpath, Generate(p), new UTF8Encoding(false));
        try {
            var psi = new ProcessStartInfo("gcc", $"-O2 -std=gnu11 \"{cpath}\" -o \"{output}\" -lm") {
                RedirectStandardError = true,
                RedirectStandardOutput = true,
                UseShellExecute = false
            };
            using var proc = Process.Start(psi)!;
            string err = proc.StandardError.ReadToEnd();
            proc.WaitForExit();
            message = err.Trim();
            return proc.ExitCode == 0;
        } catch (Exception e) {
            message = "gcc not found: " + e.Message;
            return false;
        }
    }
}
