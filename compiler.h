#pragma once
#include "common.h"
#include "parser.h"
#include <unordered_map>
#include <vector>

class Compiler {
public:
    static const std::vector<std::string> BUILTINS;

    BytecodeProgram compile(const StmtList& statements);

private:
    BytecodeProgram prog_;
    std::unordered_map<std::string, int> globals_, funcs_, builtins_, num_pool_, str_pool_;
    std::vector<std::unordered_map<std::string, int>> scopes_;
    std::vector<std::unordered_map<std::string, int>> aliases_;

    struct LoopCtx {
        std::vector<int> breaks;
        std::vector<int> continues;
    };
    std::vector<LoopCtx> loops_;

    std::vector<int>* code_ = nullptr;
    int local_count_ = 0;
    int rep_counter_ = 0;
    int for_counter_ = 0;
    bool in_function_ = false;

    int num_index(double v);
    int str_index(const std::string& s);
    int global_index(const std::string& name);

    void push_scope() { scopes_.emplace_back(); aliases_.emplace_back(); }
    void pop_scope() { scopes_.pop_back(); aliases_.pop_back(); }
    int declare_local(const std::string& name);
    bool try_local(const std::string& name, int& slot) const;
    void load(const std::string& name);
    void store(const std::string& name);

    void emit(Op op) { code_->push_back(static_cast<int>(op)); }
    void emit1(Op op, int a) { emit(op); code_->push_back(a); }
    void emit2(Op op, int a, int b) { emit(op); code_->push_back(a); code_->push_back(b); }
    void emit_num(double v) { emit1(Op::PushNum, num_index(v)); }
    void emit_str(const std::string& s) { emit1(Op::PushStr, str_index(s)); }
    int emit_jump(Op op);
    void patch(int pos);

    void compile_block(const StmtList& statements);
    void compile_stmt(const Stmt& s);
    void compile_expr(const Expr& e);
};
