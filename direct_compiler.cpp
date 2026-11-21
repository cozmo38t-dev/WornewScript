#include "direct_compiler.h"
#include "compiler.h"
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <unordered_map>

/* Signatures of functions from C libraries: 'n' = number, 's' = string.
 * Names match the builtin ID table in compiler.cpp. */
/* Whether an expression contains a call/input: in that case, evaluation order in C matters. */
static bool has_side_effect(const Expr& e) {
    if (dynamic_cast<const CallExpr*>(&e) || dynamic_cast<const InputExpr*>(&e)) return true;
    if (auto u = dynamic_cast<const UnaryExpr*>(&e)) return has_side_effect(*u->operand);
    if (auto b = dynamic_cast<const BinaryExpr*>(&e))
        return has_side_effect(*b->left) || has_side_effect(*b->right);
    return false;
}

static const std::unordered_map<std::string, std::string>& native_c_signatures() {
    static const std::unordered_map<std::string, std::string> sigs = {
        {"c_rgb", "nnn"}, {"c_gray", "n"}, {"c_clamp", "nnn"}, {"c_between", "nnn"},
        {"c_c2f", "n"}, {"c_f2c", "n"}, {"c_sleep", "n"}, {"c_ticks", ""},
        {"c_sum2", "nn"}, {"c_sign", "n"}, {"c_key_esc", ""}, {"c_key_enter", ""},
        {"c_square", "n"}, {"c_gcd", "nn"}, {"c_fact", "n"},
        {"c_randint", "nn"}, {"c_randrange", "nn"},
        {"c_exists", "s"}, {"c_is_abs", "s"}, {"c_now", ""}, {"c_elapsed", "n"},
        {"c_cls", ""}, {"c_title", "s"}, {"c_strlen", "s"}, {"c_padlen", "sn"},
        {"c_mean3", "nnn"}, {"c_med3", "nnn"}, {"c_discount", "nn"}, {"c_vat", "nn"},
        {"c_force", "nn"}, {"c_fall", "n"}, {"c_circle_area", "n"}, {"c_dist", "nnnn"}
    };
    return sigs;
}

static const char* DIRECT_RUNTIME = R"WS(
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <stddef.h>

typedef struct { double n; const char *s; } V;   /* s != NULL -> string */

static V vn(double n) { V v; v.n = n; v.s = NULL; return v; }
static V vs(const char *s) { V v; v.n = 0; v.s = s; return v; }
static double vd(V v) { return v.s ? atof(v.s) : v.n; }
static int vt(V v) { return v.s ? (v.s[0] != '\0') : (v.n != 0.0); }

static char *ws_dup(const char *s) {
    size_t n = strlen(s); char *p = (char *)malloc(n + 1);
    if (p) memcpy(p, s, n + 1);
    return p;
}
static void ws_fmt(char *out, size_t cap, V v) {
    if (v.s) snprintf(out, cap, "%s", v.s);
    else snprintf(out, cap, "%.15g", v.n);
}
static V vadd(V a, V b) {
    if (!a.s && !b.s) return vn(a.n + b.n);
    char sa[512], sb[512], out[1100];
    ws_fmt(sa, sizeof(sa), a); ws_fmt(sb, sizeof(sb), b);
    snprintf(out, sizeof(out), "%s%s", sa, sb);
    return vs(ws_dup(out));
}
static double vcmp(V a, V b) {
    if (a.s || b.s) return (double)strcmp(a.s ? a.s : "", b.s ? b.s : "");
    return a.n < b.n ? -1.0 : (a.n > b.n ? 1.0 : 0.0);
}
static int veq(V a, V b) {
    if (a.s || b.s) return strcmp(a.s ? a.s : "", b.s ? b.s : "") == 0;
    return a.n == b.n;
}
static double vlen(V v) {
    if (v.s) return (double)strlen(v.s);
    char buf[128]; snprintf(buf, sizeof(buf), "%.15g", v.n);
    return (double)strlen(buf);
}
static const char *vstr(V v) {
    /* Several buffers: two vstr() calls in the same expression must not
     * overwrite each other's results. */
    static char bufs[8][512];
    static int next = 0;
    if (v.s) return v.s;
    char *buf = bufs[next++ & 7];
    snprintf(buf, 512, "%.15g", v.n);
    return buf;
}
static void nprint(double n) { printf("%.15g\n", n); }
static void vprint(V v) { if (v.s) puts(v.s); else nprint(v.n); }
static double ws_clock(void) { return (double)clock() * 1000.0 / CLOCKS_PER_SEC; }
static unsigned int ws_builtin_rng = 12345u;
static double ws_rand(void) {
    ws_builtin_rng = ws_builtin_rng * 1103515245u + 12345u;
    return (double)(ws_builtin_rng & 0x7fffffffu) / 2147483648.0;
}
static void ws_prompt(V v) {
    if (v.s) printf("%s", v.s); else printf("%.15g", v.n);
    fflush(stdout);
}
static V ws_read_str(void) {
    char b[1024];
    if (!fgets(b, sizeof b, stdin)) b[0] = '\0';
    size_t n = strlen(b);
    while (n && (b[n-1] == '\n' || b[n-1] == '\r')) b[--n] = '\0';
    return vs(ws_dup(b));
}
static double ws_read_num(void) {
    char b[256]; if (!fgets(b, sizeof b, stdin)) return 0; return atof(b);
}
static double ws_read_int(void) {
    char b[256]; if (!fgets(b, sizeof b, stdin)) return 0;
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
)WS";

std::string DirectCompiler::c_escape(const std::string& s) {
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

std::string DirectCompiler::c_num(double v) {
    std::ostringstream oss;
    oss << std::setprecision(17) << v;
    std::string s = oss.str();
    if (s.find('.') == std::string::npos &&
        s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos) s += ".0";
    return s;
}

CType DirectCompiler::expr_type(const Expr& e, const TypeEnv& env) const {
    if (dynamic_cast<const NumExpr*>(&e)) return CType::Num;
    if (dynamic_cast<const StrExpr*>(&e)) return CType::Dyn;
    if (auto x = dynamic_cast<const InputExpr*>(&e))
        return x->type == "str" ? CType::Dyn : CType::Num;
    if (auto x = dynamic_cast<const VarExpr*>(&e)) {
        auto it = env.find(x->name);
        return it == env.end() ? CType::Num : it->second;
    }
    if (auto x = dynamic_cast<const UnaryExpr*>(&e)) {
        (void)x;
        return CType::Num;
    }
    if (auto x = dynamic_cast<const BinaryExpr*>(&e)) {
        CType l = expr_type(*x->left, env), r = expr_type(*x->right, env);
        if (x->op == TokenType::Plus) return join(l, r);
        return CType::Num;
    }
    if (auto x = dynamic_cast<const CallExpr*>(&e)) {
        if (x->name == "str") return CType::Dyn;
        auto f = func_returns_.find(x->name);
        return f == func_returns_.end() ? CType::Num : f->second;
    }
    return CType::Num;
}

void DirectCompiler::collect_types(const StmtList& stmts, TypeEnv& env, bool skip_funcs) {
    for (const auto& sp : stmts) {
        const Stmt& s = *sp;
        if (auto x = dynamic_cast<const VarStmt*>(&s)) {
            env[x->name] = join(env.count(x->name) ? env[x->name] : CType::Num, expr_type(*x->value, env));
        } else if (auto x = dynamic_cast<const AssignStmt*>(&s)) {
            if (!env.count(x->name)) global_vars_.insert(x->name);
            env[x->name] = join(env.count(x->name) ? env[x->name] : CType::Num, expr_type(*x->value, env));
        } else if (auto x = dynamic_cast<const CompoundStmt*>(&s)) {
            if (!env.count(x->name)) global_vars_.insert(x->name);
            (void)x;
        } else if (auto x = dynamic_cast<const IfStmt*>(&s)) {
            collect_types(x->then_body, env, skip_funcs);
            collect_types(x->else_body, env, skip_funcs);
        } else if (auto x = dynamic_cast<const WhileStmt*>(&s)) {
            collect_types(x->body, env, skip_funcs);
        } else if (auto x = dynamic_cast<const RepeatStmt*>(&s)) {
            collect_types(x->body, env, skip_funcs);
        } else if (auto x = dynamic_cast<const ForStmt*>(&s)) {
            auto note = [&](const Stmt* q) {
                if (auto v = dynamic_cast<const VarStmt*>(q))
                    env[v->name] = join(env.count(v->name) ? env[v->name] : CType::Num,
                                        expr_type(*v->value, env));
                else if (auto a = dynamic_cast<const AssignStmt*>(q))
                    env[a->name] = join(env.count(a->name) ? env[a->name] : CType::Num,
                                        expr_type(*a->value, env));
            };
            if (x->init) note(x->init.get());
            if (x->step) note(x->step.get());
            collect_types(x->body, env, skip_funcs);
        } else if (auto x = dynamic_cast<const FuncStmt*>(&s)) {
            if (!skip_funcs) collect_types(x->body, env, skip_funcs);
        } else if (auto x = dynamic_cast<const BlockStmt*>(&s)) {
            collect_types(x->statements, env, skip_funcs);
        }
    }
}

static void collect_gives(const StmtList& stmts,
                          const std::function<void(const Expr&)>& visit) {
    for (const auto& sp : stmts) {
        const Stmt& s = *sp;
        if (auto x = dynamic_cast<const GiveStmt*>(&s)) visit(*x->value);
        else if (auto x = dynamic_cast<const IfStmt*>(&s)) {
            collect_gives(x->then_body, visit);
            collect_gives(x->else_body, visit);
        }
        else if (auto x = dynamic_cast<const WhileStmt*>(&s)) collect_gives(x->body, visit);
        else if (auto x = dynamic_cast<const RepeatStmt*>(&s)) collect_gives(x->body, visit);
        else if (auto x = dynamic_cast<const ForStmt*>(&s)) collect_gives(x->body, visit);
        else if (auto x = dynamic_cast<const BlockStmt*>(&s)) collect_gives(x->statements, visit);
    }
}

void DirectCompiler::collect_calls(const Expr& e, const TypeEnv& env) {
    if (auto x = dynamic_cast<const CallExpr*>(&e)) {
        auto p = func_params_.find(x->name);
        if (p != func_params_.end()) {
            if (p->second.size() < x->args.size()) p->second.resize(x->args.size(), CType::Num);
            for (size_t i = 0; i < x->args.size(); ++i) {
                p->second[i] = join(p->second[i], expr_type(*x->args[i], env));
            }
        }
        for (const auto& a : x->args) collect_calls(*a, env);
    } else if (auto x = dynamic_cast<const InputExpr*>(&e)) {
        if (x->prompt) collect_calls(*x->prompt, env);
    } else if (auto x = dynamic_cast<const UnaryExpr*>(&e)) {
        collect_calls(*x->operand, env);
    } else if (auto x = dynamic_cast<const BinaryExpr*>(&e)) {
        collect_calls(*x->left, env);
        collect_calls(*x->right, env);
    }
}

void DirectCompiler::collect_calls_stmt(const Stmt& s, const TypeEnv& env) {
    if (auto x = dynamic_cast<const SayStmt*>(&s)) collect_calls(*x->value, env);
    else if (auto x = dynamic_cast<const VarStmt*>(&s)) collect_calls(*x->value, env);
    else if (auto x = dynamic_cast<const AssignStmt*>(&s)) collect_calls(*x->value, env);
    else if (auto x = dynamic_cast<const CompoundStmt*>(&s)) collect_calls(*x->value, env);
    else if (auto x = dynamic_cast<const IfStmt*>(&s)) {
        collect_calls(*x->cond, env);
        for (const auto& q : x->then_body) collect_calls_stmt(*q, env);
        for (const auto& q : x->else_body) collect_calls_stmt(*q, env);
    }
    else if (auto x = dynamic_cast<const WhileStmt*>(&s)) {
        collect_calls(*x->cond, env);
        for (const auto& q : x->body) collect_calls_stmt(*q, env);
    }
    else if (auto x = dynamic_cast<const RepeatStmt*>(&s)) {
        collect_calls(*x->count, env);
        for (const auto& q : x->body) collect_calls_stmt(*q, env);
    }
    else if (auto x = dynamic_cast<const ForStmt*>(&s)) {
        if (x->init) collect_calls_stmt(*x->init, env);
        collect_calls(*x->cond, env);
        if (x->step) collect_calls_stmt(*x->step, env);
        for (const auto& q : x->body) collect_calls_stmt(*q, env);
    }
    else if (auto x = dynamic_cast<const GiveStmt*>(&s)) collect_calls(*x->value, env);
    else if (auto x = dynamic_cast<const ExprStmt*>(&s)) collect_calls(*x->value, env);
    else if (auto x = dynamic_cast<const FuncStmt*>(&s)) {
        for (const auto& q : x->body) collect_calls_stmt(*q, env);
    }
    else if (auto x = dynamic_cast<const BlockStmt*>(&s)) {
        for (const auto& q : x->statements) collect_calls_stmt(*q, env);
    }
}

void DirectCompiler::analyze(const StmtList& prog) {
    int fc = 0;
    for (const auto& sp : prog) {
        if (auto f = dynamic_cast<FuncStmt*>(sp.get())) {
            if (func_cnames_.count(f->name))
                throw ParseError("duplicate declaration of function '" + f->name + "'");
            func_cnames_[f->name] = "ws_f" + std::to_string(fc++);
            func_params_[f->name] = std::vector<CType>(f->params.size(), CType::Num);
            func_returns_[f->name] = CType::Num;
        }
        if (auto v = dynamic_cast<VarStmt*>(sp.get())) global_vars_.insert(v->name);
    }

    for (int iter = 0; iter < 4; ++iter) {
        TypeEnv genv;
        collect_types(prog, genv, true);
        global_types_ = genv;

        for (const auto& sp : prog) collect_calls_stmt(*sp, genv);

        for (const auto& sp : prog) {
            auto f = dynamic_cast<FuncStmt*>(sp.get());
            if (!f) continue;
            TypeEnv env = genv;
            for (size_t i = 0; i < f->params.size(); ++i) {
                env[f->params[i]] = i < func_params_[f->name].size()
                    ? func_params_[f->name][i] : CType::Num;
            }
            collect_types(f->body, env, true);
            CType ret = CType::Num;
            collect_gives(f->body, [&](const Expr& ge) { ret = join(ret, expr_type(ge, env)); });
            func_returns_[f->name] = ret;
            func_envs_[f->name] = env;
        }
    }
}


std::string DirectCompiler::gen_expr(const Expr& e, CType want, const TypeEnv& env) const {
    CType have = expr_type(e, env);
    auto as_want = [&](const std::string& s, CType h) -> std::string {
        if (h == want) return s;
        if (want == CType::Num) return "vd(" + s + ")";
        return "vn(" + s + ")";
    };

    if (auto x = dynamic_cast<const NumExpr*>(&e)) return as_want(c_num(x->value), CType::Num);
    if (auto x = dynamic_cast<const StrExpr*>(&e)) return as_want("vs(\"" + c_escape(x->value) + "\")", CType::Dyn);
    if (auto x = dynamic_cast<const VarExpr*>(&e)) return as_want(x->name, have);

    if (auto x = dynamic_cast<const UnaryExpr*>(&e)) {
        std::string a = gen_expr(*x->operand, CType::Dyn, env);
        if (x->op == TokenType::Minus) return as_want("(-vd(" + a + "))", CType::Num);
        return as_want("(!vt(" + a + "))", CType::Num);
    }

    if (auto x = dynamic_cast<const BinaryExpr*>(&e)) {
        CType lt = expr_type(*x->left, env), rt = expr_type(*x->right, env);
        bool dyn = (lt == CType::Dyn || rt == CType::Dyn);
        bool logic = (x->op == TokenType::And || x->op == TokenType::AndAnd ||
                      x->op == TokenType::Or || x->op == TokenType::OrOr);

        /* In C, operand evaluation order is unspecified. If there are calls,
         * compute both into sequential temporaries, as in the VM. */
        static int seq_id = 0;
        std::string pre;
        std::function<std::string(CType)> L = [&](CType t) { return gen_expr(*x->left, t, env); };
        std::function<std::string(CType)> R = [&](CType t) { return gen_expr(*x->right, t, env); };
        if (!logic && (has_side_effect(*x->left) || has_side_effect(*x->right))) {
            int id = seq_id++;
            std::string lname = "ws_seq_l" + std::to_string(id);
            std::string rname = "ws_seq_r" + std::to_string(id);
            pre = "V " + lname + " = " + gen_expr(*x->left, CType::Dyn, env) + "; " +
                  "V " + rname + " = " + gen_expr(*x->right, CType::Dyn, env) + "; ";
            L = [lname](CType t) { return t == CType::Dyn ? lname : "vd(" + lname + ")"; };
            R = [rname](CType t) { return t == CType::Dyn ? rname : "vd(" + rname + ")"; };
        }
        auto finish = [&](const std::string& core) {
            return pre.empty() ? core : "({ " + pre + " " + core + "; })";
        };

        const char* op = nullptr;
        switch (x->op) {
            case TokenType::Minus: op = "-"; break;
            case TokenType::Star: op = "*"; break;
            case TokenType::Slash: op = "/"; break;
            case TokenType::Percent: op = "%"; break;
            case TokenType::EqualsEquals: op = "=="; break;
            case TokenType::NotEquals: op = "!="; break;
            case TokenType::Less: op = "<"; break;
            case TokenType::LessEqual: op = "<="; break;
            case TokenType::Greater: op = ">"; break;
            case TokenType::GreaterEqual: op = ">="; break;
            case TokenType::And: op = "&&"; break;
            case TokenType::AndAnd: op = "&&"; break;
            case TokenType::Or: op = "||"; break;
            case TokenType::OrOr: op = "||"; break;
            default: op = "+"; break;
        }

        if (x->op == TokenType::Plus) {
            if (dyn) return as_want(finish("vadd(" + L(CType::Dyn) + ", " + R(CType::Dyn) + ")"), CType::Dyn);
            return as_want(finish("(" + L(CType::Num) + " + " + R(CType::Num) + ")"), CType::Num);
        }
        if (x->op == TokenType::Percent) {
            std::string b = R(CType::Num);
            return as_want(finish("((" + b + ") == 0 ? 0 : fmod(" + L(CType::Num) + ", " + b + "))"), CType::Num);
        }
        bool cmp = (x->op == TokenType::EqualsEquals || x->op == TokenType::NotEquals ||
                    x->op == TokenType::Less || x->op == TokenType::LessEqual ||
                    x->op == TokenType::Greater || x->op == TokenType::GreaterEqual);
        if (cmp && dyn) {
            std::string core;
            if (x->op == TokenType::EqualsEquals) core = "veq(" + L(CType::Dyn) + ", " + R(CType::Dyn) + ")";
            else if (x->op == TokenType::NotEquals) core = "(!veq(" + L(CType::Dyn) + ", " + R(CType::Dyn) + "))";
            else core = "(vcmp(" + L(CType::Dyn) + ", " + R(CType::Dyn) + ") " + op + " 0)";
            return as_want(finish(core), CType::Num);
        }
        if (logic)
            return as_want("(" + L(CType::Num) + " " + op + " " + R(CType::Num) + ")", CType::Num);

        std::string b = R(CType::Num);
        std::string core = "(" + L(CType::Num) + " " + op + " " + b + ")";
        if (x->op == TokenType::Slash || x->op == TokenType::Percent)
            core = "((" + b + ") == 0 ? 0 : " + core + ")";
        return as_want(finish(core), CType::Num);
    }

    if (auto x = dynamic_cast<const InputExpr*>(&e)) {
        std::string read;
        if (x->type == "str") read = "ws_read_str()";
        else if (x->type == "bool") read = "ws_read_bool()";
        else if (x->type == "int") read = "ws_read_int()";
        else read = "ws_read_num()";

        CType natural = (x->type == "str") ? CType::Dyn : CType::Num;
        std::string core;
        if (x->prompt) {
            // Wrap in parentheses: otherwise `f = ws_prompt(...), ws_read_num();`
            // is parsed as `(f = ws_prompt(...)), ws_read_num()`.
            core = "(ws_prompt(" + gen_expr(*x->prompt, CType::Dyn, env) + "), " + read + ")";
        } else {
            core = read;
        }
        return as_want(core, natural);
    }

    if (auto x = dynamic_cast<const CallExpr*>(&e)) {
        const std::string& name = x->name;
        auto arg = [&](size_t i, CType t) -> std::string {
            if (i >= x->args.size())
                return t == CType::Dyn ? std::string("vs(\"\")") : std::string("0");
            return gen_expr(*x->args[i], t, env);
        };
        auto txt = [&](size_t i) { return "vstr(" + arg(i, CType::Dyn) + ")"; };
        std::string core;
        CType natural = CType::Num;

        if (name == "sqrt") core = "sqrt(" + arg(0, CType::Num) + ")";
        else if (name == "abs") core = "fabs(" + arg(0, CType::Num) + ")";
        else if (name == "pow") core = "pow(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ")";
        else if (name == "min") core = "fmin(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ")";
        else if (name == "max") core = "fmax(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ")";
        else if (name == "int") core = "trunc(" + arg(0, CType::Num) + ")";
        else if (name == "len") {
            CType t = x->args.empty() ? CType::Num : expr_type(*x->args[0], env);
            if (t == CType::Dyn && !x->args.empty()) core = "vlen(" + arg(0, CType::Dyn) + ")";
            else core = "vlen(vn(" + (x->args.empty() ? std::string("0") : arg(0, CType::Num)) + "))";
        }
        else if (name == "str") {
            core = arg(0, CType::Dyn);
            natural = CType::Dyn;
        }
        else if (name == "clock") core = "ws_clock()";
        else if (name == "rand") core = "ws_rand()";
        else if (name == "sin") core = "sin(" + arg(0, CType::Num) + ")";
        else if (name == "cos") core = "cos(" + arg(0, CType::Num) + ")";
        else if (name == "tan") core = "tan(" + arg(0, CType::Num) + ")";
        else if (name == "floor") core = "floor(" + arg(0, CType::Num) + ")";
        else if (name == "ceil") core = "ceil(" + arg(0, CType::Num) + ")";
        else if (name == "round") core = "round(" + arg(0, CType::Num) + ")";
        else if (name == "log") core = "log(" + arg(0, CType::Num) + ")";
        else if (name == "log10") core = "log10(" + arg(0, CType::Num) + ")";
        else if (name == "exp") core = "exp(" + arg(0, CType::Num) + ")";
        else if (name == "sign") {
            std::string a = arg(0, CType::Num);
            core = "((" + a + ") > 0 ? 1 : ((" + a + ") < 0 ? -1 : 0))";
        }
        else if (name == "clamp") {
            std::string a = arg(0, CType::Num), lo = arg(1, CType::Num), hi = arg(2, CType::Num);
            core = "fmin(fmax(" + a + ", " + lo + "), " + hi + ")";
        }

        /* --- GUI --- */
        else if (name == "window") core = "wsg_window(" + txt(0) + ", " + arg(1, CType::Num) + ", " + arg(2, CType::Num) + ")";
        else if (name == "win_open") core = "wsg_open(" + arg(0, CType::Num) + ")";
        else if (name == "win_begin") core = "(wsg_begin(" + arg(0, CType::Num) + "), 0)";
        else if (name == "win_end") core = "(wsg_end(" + arg(0, CType::Num) + "), 0)";
        else if (name == "win_clear") core = "(wsg_clear(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + "), 0)";
        else if (name == "win_quit") core = "(wsg_quit(" + arg(0, CType::Num) + "), 0)";
        else if (name == "win_width") core = "wsg_width(" + arg(0, CType::Num) + ")";
        else if (name == "win_height") core = "wsg_height(" + arg(0, CType::Num) + ")";
        else if (name == "win_fullscreen") core = "(wsg_fullscreen(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + "), 0)";
        else if (name == "win_is_fullscreen") core = "wsg_is_fullscreen(" + arg(0, CType::Num) + ")";
        else if (name == "rgb") core = "wsg_rgb(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " + arg(2, CType::Num) + ")";
        else if (name == "rect") core = "(wsg_rect(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                                        arg(2, CType::Num) + ", " + arg(3, CType::Num) + ", " +
                                        arg(4, CType::Num) + ", " + arg(5, CType::Num) + "), 0)";
        else if (name == "panel") core = "(wsg_panel(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                                          arg(2, CType::Num) + ", " + arg(3, CType::Num) + ", " +
                                          arg(4, CType::Num) + "), 0)";
        else if (name == "label") core = "(wsg_label(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                                          arg(2, CType::Num) + ", " + txt(3) + "), 0)";
        else if (name == "label_ex") core = "(wsg_label_ex(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                                             arg(2, CType::Num) + ", " + txt(3) + ", " + arg(4, CType::Num) + ", " +
                                             arg(5, CType::Num) + "), 0)";
        else if (name == "button") core = "wsg_button(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                                          arg(2, CType::Num) + ", " + arg(3, CType::Num) + ", " +
                                          arg(4, CType::Num) + ", " + txt(5) + ")";
        else if (name == "checkbox") core = "wsg_checkbox(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                                            arg(2, CType::Num) + ", " + txt(3) + ", " + arg(4, CType::Num) + ")";
        else if (name == "field") {
            core = "vs(wsg_field(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ", " +
                   arg(2, CType::Num) + ", " + arg(3, CType::Num) + ", " + arg(4, CType::Num) + ", " +
                   txt(5) + "))";
            natural = CType::Dyn;
        }
        else if (name == "mouse_x") core = "wsg_mouse_x(" + arg(0, CType::Num) + ")";
        else if (name == "mouse_y") core = "wsg_mouse_y(" + arg(0, CType::Num) + ")";
        else if (name == "mouse_down") core = "wsg_mouse_down(" + arg(0, CType::Num) + ")";
        else if (name == "mouse_pressed") core = "wsg_mouse_pressed(" + arg(0, CType::Num) + ")";
        else if (name == "key_down") core = "wsg_key_down(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ")";
        else if (name == "key_pressed") core = "wsg_key_pressed(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + ")";
        else if (name == "font") core = "(wsg_font(" + arg(0, CType::Num) + ", " + arg(1, CType::Num) + "), 0)";
        else if (name == "font_size") core = "wsg_font_size()";
        else if (name == "sleep") core = "(mg_sleep((int)(" + arg(0, CType::Num) + ")), 0)";

        /* --- C libraries from libs/  (linked via $APT) --- */
        else if (auto sit = native_c_signatures().find(name);
                 sit != native_c_signatures().end()) {
            std::string args;
            for (size_t i = 0; i < sit->second.size(); ++i) {
                if (i >= x->args.size()) break;
                if (i) args += ", ";
                args += (sit->second[i] == 's') ? txt(i) : arg(i, CType::Num);
            }
            core = "ws_" + name + "(" + args + ")";
        }
        else {
            std::string args;
            auto pit = func_params_.find(name);
            for (size_t i = 0; i < x->args.size(); ++i) {
                CType t = CType::Num;
                if (pit != func_params_.end() && i < pit->second.size()) t = pit->second[i];
                if (i) args += ", ";
                args += arg(i, t);
            }
            core = func_cnames_.at(name) + "(" + args + ")";
            auto rit = func_returns_.find(name);
            natural = rit == func_returns_.end() ? CType::Num : rit->second;
        }
        return as_want(core, natural);
    }
    return as_want("0", CType::Num);
}


std::string DirectCompiler::gen_cond(const Expr& e, const TypeEnv& env) const {
    CType t = expr_type(e, env);
    if (t == CType::Dyn) return "vt(" + gen_expr(e, CType::Dyn, env) + ")";
    return "(" + gen_expr(e, CType::Num, env) + " != 0)";
}

void DirectCompiler::gen_block(const StmtList& stmts, const TypeEnv& env, bool top_level,
                               int indent, std::string& out) const {
    for (const auto& sp : stmts) gen_stmt(*sp, env, top_level, indent, out);
}

void DirectCompiler::gen_stmt(const Stmt& s, const TypeEnv& env, bool top_level,
                              int indent, std::string& out) const {
    std::string p = ind(indent);

    if (auto x = dynamic_cast<const SayStmt*>(&s)) {
        CType t = expr_type(*x->value, env);
        if (t == CType::Dyn) out += p + "vprint(" + gen_expr(*x->value, CType::Dyn, env) + ");\n";
        else out += p + "nprint(" + gen_expr(*x->value, CType::Num, env) + ");\n";

    } else if (auto x = dynamic_cast<const VarStmt*>(&s)) {
        auto it = env.find(x->name);
        CType t = it == env.end() ? CType::Num : it->second;
        std::string init = gen_expr(*x->value, t, env);
        if (top_level) out += p + x->name + " = " + init + ";\n";
        else {
            std::string decl = (t == CType::Dyn) ? "V " : "double ";
            out += p + decl + x->name + " = " + init + ";\n";
        }

    } else if (auto x = dynamic_cast<const AssignStmt*>(&s)) {
        auto it = env.find(x->name);
        CType t = it == env.end() ? CType::Num : it->second;
        out += p + x->name + " = " + gen_expr(*x->value, t, env) + ";\n";

    } else if (auto x = dynamic_cast<const CompoundStmt*>(&s)) {
        auto it = env.find(x->name);
        CType t = it == env.end() ? CType::Num : it->second;
        const char* op = "+=";
        switch (x->op) {
            case TokenType::MinusEq: op = "-="; break;
            case TokenType::StarEq: op = "*="; break;
            case TokenType::SlashEq: op = "/="; break;
            default: break;
        }
        if (x->op == TokenType::PercentEq || x->op == TokenType::SlashEq) {
            static int guard_id = 0;
            std::string tmp = "ws_guard" + std::to_string(guard_id++);
            std::string rhs = gen_expr(*x->value, CType::Num, env);
            out += p + "double " + tmp + " = " + rhs + ";\n";
            if (x->op == TokenType::PercentEq)
                out += p + x->name + " = (" + tmp + " == 0 ? 0 : fmod(" + x->name + ", " + tmp + "));\n";
            else
                out += p + x->name + " = (" + tmp + " == 0 ? 0 : " + x->name + " / " + tmp + ");\n";
        } else if (t == CType::Dyn) {
            out += p + x->name + " = vadd(" + x->name + ", " + gen_expr(*x->value, CType::Dyn, env) + ");\n";
        } else {
            out += p + x->name + " " + op + " " + gen_expr(*x->value, CType::Num, env) + ";\n";
        }

    } else if (auto x = dynamic_cast<const IfStmt*>(&s)) {
        out += p + "if (" + gen_cond(*x->cond, env) + ") {\n";
        gen_block(x->then_body, env, false, indent + 1, out);
        if (!x->else_body.empty()) {
            out += p + "} else {\n";
            gen_block(x->else_body, env, false, indent + 1, out);
        }
        out += p + "}\n";

    } else if (auto x = dynamic_cast<const WhileStmt*>(&s)) {
        out += p + "while (" + gen_cond(*x->cond, env) + ") {\n";
        gen_block(x->body, env, false, indent + 1, out);
        out += p + "}\n";

    } else if (auto x = dynamic_cast<const RepeatStmt*>(&s)) {
        static int rc = 0;
        std::string var = "ws_rep" + std::to_string(rc++);
        out += p + "for (double " + var + " = 0; " + var + " < (" +
               gen_expr(*x->count, CType::Num, env) + "); ++" + var + ") {\n";
        gen_block(x->body, env, false, indent + 1, out);
        out += p + "}\n";

    } else if (auto x = dynamic_cast<const ForStmt*>(&s)) {
        // Initialization inside the for header => the counter is scoped to the loop.
        std::string init;
        if (x->init) {
            if (auto v = dynamic_cast<const VarStmt*>(x->init.get())) {
                auto it = env.find(v->name);
                CType t = it == env.end() ? CType::Num : it->second;
                init = ((t == CType::Dyn) ? "V " : "double ") + v->name + " = " +
                       gen_expr(*v->value, t, env);
            } else {
                std::string tmp;
                gen_stmt(*x->init, env, false, 0, tmp);
                while (!tmp.empty() && (tmp.back() == '\n' || tmp.back() == ';')) tmp.pop_back();
                init = tmp;
            }
        }

        std::string step;
        if (x->step) {
            if (auto v = dynamic_cast<const VarStmt*>(x->step.get())) {
                auto it = env.find(v->name);
                CType t = it == env.end() ? CType::Num : it->second;
                step = v->name + " = " + gen_expr(*v->value, t, env);
            } else {
                std::string tmp;
                gen_stmt(*x->step, env, false, 0, tmp);
                while (!tmp.empty() && (tmp.back() == '\n' || tmp.back() == ';')) tmp.pop_back();
                step = tmp;
            }
        }

        out += p + "for (" + init + "; " + gen_cond(*x->cond, env) + "; " + step + ") {\n";
        gen_block(x->body, env, false, indent + 1, out);
        out += p + "}\n";

    } else if (dynamic_cast<const BreakStmt*>(&s)) {
        out += p + "break;\n";
    } else if (dynamic_cast<const ContinueStmt*>(&s)) {
        out += p + "continue;\n";

    } else if (auto x = dynamic_cast<const GiveStmt*>(&s)) {
        // Take the return type from function analysis: it is in env under a service key
        auto rit = env.find("$return");
        CType rt = rit == env.end() ? CType::Num : rit->second;
        out += p + "return " + gen_expr(*x->value, rt, env) + ";\n";

    } else if (dynamic_cast<const FuncStmt*>(&s)) {
        // functions are emitted separately in generate()

    } else if (auto x = dynamic_cast<const ExprStmt*>(&s)) {
        out += p + gen_expr(*x->value, expr_type(*x->value, env), env) + ";\n";

    } else if (auto x = dynamic_cast<const BlockStmt*>(&s)) {
        out += p + "{\n";
        gen_block(x->statements, env, false, indent + 1, out);
        out += p + "}\n";
    }
}

std::string DirectCompiler::generate(const StmtList& prog) {
    analyze(prog);

    std::ostringstream oss;
    oss << DIRECT_RUNTIME << "\n";

    // global declarations
    for (const auto& name : global_vars_) {
        auto it = global_types_.find(name);
        CType t = it == global_types_.end() ? CType::Num : it->second;
        oss << "static " << (t == CType::Dyn ? "V " : "double ") << name << ";\n";
    }

    // function prototypes
    for (const auto& sp : prog) {
        auto f = dynamic_cast<FuncStmt*>(sp.get());
        if (!f) continue;
        CType ret = func_returns_[f->name];
        oss << "static " << (ret == CType::Dyn ? "V " : "double ") << func_cnames_[f->name] << "(";
        for (size_t i = 0; i < f->params.size(); ++i) {
            if (i) oss << ", ";
            CType t = i < func_params_[f->name].size() ? func_params_[f->name][i] : CType::Num;
            oss << (t == CType::Dyn ? "V " : "double ") << f->params[i];
        }
        oss << ");\n";
    }
    oss << "\n";

    // function definitions
    for (const auto& sp : prog) {
        auto f = dynamic_cast<FuncStmt*>(sp.get());
        if (!f) continue;
        CType ret = func_returns_[f->name];
        TypeEnv env = func_envs_[f->name];
        env["$return"] = ret;
        oss << "static " << (ret == CType::Dyn ? "V " : "double ") << func_cnames_[f->name] << "(";
        for (size_t i = 0; i < f->params.size(); ++i) {
            if (i) oss << ", ";
            CType t = i < func_params_[f->name].size() ? func_params_[f->name][i] : CType::Num;
            oss << (t == CType::Dyn ? "V " : "double ") << f->params[i];
        }
        oss << ") {\n";
        std::string body;
        gen_block(f->body, env, false, 1, body);
        oss << body;
        if (ret == CType::Dyn) oss << "    return vn(0);\n";
        else oss << "    return 0;\n";
        oss << "}\n\n";
    }

    oss << "int main(void) {\n";
    std::string body;
    gen_block(prog, global_types_, true, 1, body);
    oss << body;
    oss << "    return 0;\n}\n";
    return oss.str();
}
