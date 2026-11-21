#pragma once
#include "common.h"
#include "parser.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Value type in the generated C: Num = a plain double, Dyn = struct V (string/number). */
enum class CType { Num, Dyn };

/* Generates C source directly from the AST, without a bytecode VM.
 * Numeric expressions become ordinary double expressions, so gcc
 * optimizes loops as a whole (unlike bytecode interpretation). */
class DirectCompiler {
public:
    std::string generate(const StmtList& statements);

private:
    using TypeEnv = std::unordered_map<std::string, CType>;

    TypeEnv global_types_;
    std::unordered_map<std::string, TypeEnv> func_envs_;
    std::unordered_map<std::string, CType> func_returns_;
    std::unordered_map<std::string, std::vector<CType>> func_params_;
    std::unordered_map<std::string, std::string> func_cnames_;
    std::unordered_set<std::string> global_vars_;

    CType expr_type(const Expr& e, const TypeEnv& env) const;
    void collect_types(const StmtList& stmts, TypeEnv& env, bool skip_funcs);
    void collect_calls(const Expr& e, const TypeEnv& env);
    void collect_calls_stmt(const Stmt& s, const TypeEnv& env);
    void analyze(const StmtList& prog);

    std::string gen_expr(const Expr& e, CType want, const TypeEnv& env) const;
    std::string gen_cond(const Expr& e, const TypeEnv& env) const;
    void gen_stmt(const Stmt& s, const TypeEnv& env, bool top_level, int indent, std::string& out) const;
    void gen_block(const StmtList& stmts, const TypeEnv& env, bool top_level, int indent, std::string& out) const;

    static CType join(CType a, CType b) { return (a == CType::Dyn || b == CType::Dyn) ? CType::Dyn : CType::Num; }
    static std::string c_escape(const std::string& s);
    static std::string c_num(double v);
    static std::string ind(int n) { return std::string(n * 4, ' '); }
};
