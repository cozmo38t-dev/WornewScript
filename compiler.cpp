#include "compiler.h"
#include <cmath>
#include <sstream>

static int input_type_id(const std::string& t) {
    if (t == "int") return 0;
    if (t == "num") return 1;
    if (t == "str") return 2;
    if (t == "bool") return 3;
    return 1;
}

const std::vector<std::string> Compiler::BUILTINS = {
    "sqrt", "abs", "pow", "min", "max", "len", "int", "str", "clock", "rand",
    "sin", "cos", "tan", "floor", "ceil", "round", "log", "log10", "exp",
    "sign", "clamp",
    /* 21+ — GUI (windows), implemented on top of minigui.h */
    "window", "win_open", "win_begin", "win_end", "win_clear", "win_quit",
    "win_width", "win_height", "win_fullscreen", "win_is_fullscreen", "rgb",
    "rect", "panel", "label", "label_ex", "button", "checkbox", "field",
    "mouse_x", "mouse_y", "mouse_down", "mouse_pressed", "key_down",
    "key_pressed", "font", "font_size", "sleep",
    /* 48+ — functions from C libraries in libs/  (included via $APT) */
    "c_rgb", "c_gray", "c_clamp", "c_between", "c_c2f", "c_f2c",
    "c_sleep", "c_ticks", "c_sum2", "c_sign", "c_key_esc", "c_key_enter",
    "c_square", "c_gcd", "c_fact", "c_randint", "c_randrange",
    "c_exists", "c_is_abs", "c_now", "c_elapsed", "c_cls", "c_title",
    "c_strlen", "c_padlen", "c_mean3", "c_med3", "c_discount", "c_vat",
    "c_force", "c_fall", "c_circle_area", "c_dist"
};

int Compiler::num_index(double v) {
    std::ostringstream oss;
    oss.precision(17);
    oss << v;
    std::string key = oss.str();
    auto it = num_pool_.find(key);
    if (it != num_pool_.end()) return it->second;
    int i = static_cast<int>(prog_.numbers.size());
    prog_.numbers.push_back(v);
    num_pool_[key] = i;
    return i;
}

int Compiler::str_index(const std::string& s) {
    auto it = str_pool_.find(s);
    if (it != str_pool_.end()) return it->second;
    int i = static_cast<int>(prog_.strings.size());
    prog_.strings.push_back(s);
    str_pool_[s] = i;
    return i;
}

int Compiler::global_index(const std::string& name) {
    auto it = globals_.find(name);
    if (it != globals_.end()) return it->second;
    int i = static_cast<int>(prog_.names.size());
    prog_.names.push_back(name);
    globals_[name] = i;
    return i;
}

int Compiler::declare_local(const std::string& name) {
    int slot = local_count_++;
    if (!scopes_.empty()) scopes_.back()[name] = slot;
    return slot;
}

bool Compiler::try_local(const std::string& name, int& slot) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) { slot = f->second; return true; }
    }
    return false;
}

void Compiler::load(const std::string& name) {
    int slot;
    if (try_local(name, slot)) { emit1(Op::LoadL, slot); return; }
    for (auto it = aliases_.rbegin(); it != aliases_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) {
            int v = f->second;
            if (v < 0) emit1(Op::LoadL, -v - 1);   // local loop alias
            else emit1(Op::LoadG, v);
            return;
        }
    }
    emit1(Op::LoadG, global_index(name));
}

void Compiler::store(const std::string& name) {
    int slot;
    if (try_local(name, slot)) { emit1(Op::StoreL, slot); return; }
    for (auto it = aliases_.rbegin(); it != aliases_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) {
            int v = f->second;
            if (v < 0) emit1(Op::StoreL, -v - 1);
            else emit1(Op::StoreG, v);
            return;
        }
    }
    emit1(Op::StoreG, global_index(name));
}

int Compiler::emit_jump(Op op) {
    emit(op);
    int pos = static_cast<int>(code_->size());
    code_->push_back(-1);
    return pos;
}

void Compiler::patch(int pos) {
    (*code_)[pos] = static_cast<int>(code_->size());
}

BytecodeProgram Compiler::compile(const StmtList& statements) {
    for (int i = 0; i < static_cast<int>(BUILTINS.size()); ++i) {
        builtins_[BUILTINS[i]] = i;
    }

    // register all functions first — they can be called before declaration
    for (const auto& s : statements) {
        if (auto f = dynamic_cast<const FuncStmt*>(s.get())) {
            if (funcs_.count(f->name))
                throw ParseError("duplicate declaration of function '" + f->name + "'");
            {
                int idx = static_cast<int>(prog_.functions.size());
                funcs_[f->name] = idx;
                Function fn;
                fn.name = f->name;
                fn.arity = static_cast<int>(f->params.size());
                prog_.functions.push_back(std::move(fn));
            }
        }
    }

    code_ = &prog_.main_code;
    local_count_ = 0;
    in_function_ = false;
    compile_block(statements);
    emit(Op::Halt);

    for (const auto& s : statements) {
        auto fn_stmt = dynamic_cast<const FuncStmt*>(s.get());
        if (!fn_stmt) continue;
        int idx = funcs_[fn_stmt->name];
        Function& fn = prog_.functions[idx];
        code_ = &fn.code;
        local_count_ = 0;
        in_function_ = true;
        scopes_.clear();
        aliases_.clear();
        push_scope();
        for (const auto& p : fn_stmt->params) declare_local(p);
        compile_block(fn_stmt->body);
        emit_num(0);
        emit(Op::Ret);
        fn.local_count = local_count_;
        pop_scope();
    }

    return prog_;
}

void Compiler::compile_block(const StmtList& statements) {
    for (const auto& s : statements) compile_stmt(*s);
}

void Compiler::compile_stmt(const Stmt& s) {
    if (auto x = dynamic_cast<const SayStmt*>(&s)) {
        compile_expr(*x->value);
        emit(Op::Print);

    } else if (auto x = dynamic_cast<const VarStmt*>(&s)) {
        compile_expr(*x->value);
        if (!scopes_.empty() && in_function_) {
            int slot = declare_local(x->name);
            emit1(Op::StoreL, slot);
        } else {
            store(x->name);
        }

    } else if (auto x = dynamic_cast<const AssignStmt*>(&s)) {
        compile_expr(*x->value);
        store(x->name);

    } else if (auto x = dynamic_cast<const CompoundStmt*>(&s)) {
        load(x->name);
        compile_expr(*x->value);
        Op op = Op::Add;
        switch (x->op) {
            case TokenType::PlusEq: op = Op::Add; break;
            case TokenType::MinusEq: op = Op::Sub; break;
            case TokenType::StarEq: op = Op::Mul; break;
            case TokenType::SlashEq: op = Op::Div; break;
            case TokenType::PercentEq: op = Op::Mod; break;
            default: break;
        }
        emit(op);
        store(x->name);

    } else if (auto x = dynamic_cast<const IfStmt*>(&s)) {
        compile_expr(*x->cond);
        int to_else = emit_jump(Op::JmpF);
        push_scope(); compile_block(x->then_body); pop_scope();
        int to_end = emit_jump(Op::Jmp);
        patch(to_else);
        push_scope(); compile_block(x->else_body); pop_scope();
        patch(to_end);

    } else if (auto x = dynamic_cast<const WhileStmt*>(&s)) {
        int start = static_cast<int>(code_->size());
        compile_expr(*x->cond);
        int exit = emit_jump(Op::JmpF);
        LoopCtx loop;
        loops_.push_back(loop);
        push_scope(); compile_block(x->body); pop_scope();
        for (int p : loops_.back().continues) (*code_)[p] = start;
        emit1(Op::Jmp, start);
        patch(exit);
        for (int p : loops_.back().breaks) (*code_)[p] = static_cast<int>(code_->size());
        loops_.pop_back();

    } else if (auto x = dynamic_cast<const RepeatStmt*>(&s)) {
        std::string counter = "$rep" + std::to_string(rep_counter_++);
        int cidx;
        bool local = false;
        if (in_function_) { cidx = declare_local(counter); local = true; }
        else cidx = global_index(counter);
        Op load_c = local ? Op::LoadL : Op::LoadG;
        Op store_c = local ? Op::StoreL : Op::StoreG;
        emit_num(0);
        emit1(store_c, cidx);
        int start = static_cast<int>(code_->size());
        emit1(load_c, cidx);
        compile_expr(*x->count);
        emit(Op::Lt);
        int exit = emit_jump(Op::JmpF);
        loops_.emplace_back();
        push_scope(); compile_block(x->body); pop_scope();
        for (int p : loops_.back().continues) (*code_)[p] = static_cast<int>(code_->size());
        emit1(load_c, cidx);
        emit_num(1);
        emit(Op::Add);
        emit1(store_c, cidx);
        emit1(Op::Jmp, start);
        patch(exit);
        for (int p : loops_.back().breaks) (*code_)[p] = static_cast<int>(code_->size());
        loops_.pop_back();

    } else if (auto x = dynamic_cast<const ForStmt*>(&s)) {
        // The for counter lives in the loop scope: hide it under a unique name
        // so that two nested for loops with variable i don't overwrite each other.
        const VarStmt* vi = x->init ? dynamic_cast<const VarStmt*>(x->init.get()) : nullptr;
        push_scope();
        int alias_slot = -1;
        if (vi) {
            bool local = in_function_;
            if (local) alias_slot = declare_local("$for" + std::to_string(for_counter_++));
            else alias_slot = global_index("$for" + std::to_string(for_counter_++));
            compile_expr(*vi->value);
            emit1(local ? Op::StoreL : Op::StoreG, alias_slot);
            // Negative value = local slot (see load/store).
            aliases_.back()[vi->name] = local ? -(alias_slot + 1) : alias_slot;
        } else if (x->init) {
            compile_stmt(*x->init);
        }

        int start = static_cast<int>(code_->size());
        compile_expr(*x->cond);
        int exit = emit_jump(Op::JmpF);
        loops_.emplace_back();
        compile_block(x->body);
        for (int p : loops_.back().continues) (*code_)[p] = static_cast<int>(code_->size());
        if (x->step) compile_stmt(*x->step);
        pop_scope();
        emit1(Op::Jmp, start);
        patch(exit);
        for (int p : loops_.back().breaks) (*code_)[p] = static_cast<int>(code_->size());
        loops_.pop_back();

    } else if (dynamic_cast<const BreakStmt*>(&s)) {
        if (loops_.empty()) throw ParseError("break outside of a loop");
        loops_.back().breaks.push_back(emit_jump(Op::Jmp));

    } else if (dynamic_cast<const ContinueStmt*>(&s)) {
        if (loops_.empty()) throw ParseError("continue outside of a loop");
        loops_.back().continues.push_back(emit_jump(Op::Jmp));

    } else if (auto x = dynamic_cast<const GiveStmt*>(&s)) {
        compile_expr(*x->value);
        emit(Op::Ret);

    } else if (dynamic_cast<const FuncStmt*>(&s)) {
        // functions are compiled separately

    } else if (auto x = dynamic_cast<const ExprStmt*>(&s)) {
        compile_expr(*x->value);
        emit(Op::Pop);

    } else if (auto x = dynamic_cast<const BlockStmt*>(&s)) {
        push_scope(); compile_block(x->statements); pop_scope();
    }
}

void Compiler::compile_expr(const Expr& e) {
    if (auto x = dynamic_cast<const NumExpr*>(&e)) {
        emit_num(x->value);

    } else if (auto x = dynamic_cast<const StrExpr*>(&e)) {
        emit_str(x->value);

    } else if (auto x = dynamic_cast<const VarExpr*>(&e)) {
        load(x->name);

    } else if (auto x = dynamic_cast<const UnaryExpr*>(&e)) {
        compile_expr(*x->operand);
        emit(x->op == TokenType::Minus ? Op::Neg : Op::Not);

    } else if (auto x = dynamic_cast<const BinaryExpr*>(&e)) {
        // && and || evaluate the right-hand side only when necessary —
        // as in C. This matters for side effects inside expressions.
        if (x->op == TokenType::And || x->op == TokenType::AndAnd) {
            compile_expr(*x->left);
            int skip_right = emit_jump(Op::JmpF);   // consumes the left value
            compile_expr(*x->right);
            emit(Op::Not); emit(Op::Not);           // normalize to 0/1
            int done = emit_jump(Op::Jmp);
            patch(skip_right);
            emit_num(0);
            patch(done);
        } else if (x->op == TokenType::Or || x->op == TokenType::OrOr) {
            compile_expr(*x->left);
            int need_right = emit_jump(Op::JmpF);   // consumes the left value
            emit_num(1);
            int done = emit_jump(Op::Jmp);
            patch(need_right);
            compile_expr(*x->right);
            emit(Op::Not); emit(Op::Not);
            patch(done);
        } else {
            compile_expr(*x->left);
            compile_expr(*x->right);
            Op op = Op::Nop;
            switch (x->op) {
                case TokenType::Plus: op = Op::Add; break;
                case TokenType::Minus: op = Op::Sub; break;
                case TokenType::Star: op = Op::Mul; break;
                case TokenType::Slash: op = Op::Div; break;
                case TokenType::Percent: op = Op::Mod; break;
                case TokenType::EqualsEquals: op = Op::Eq; break;
                case TokenType::NotEquals: op = Op::Ne; break;
                case TokenType::Less: op = Op::Lt; break;
                case TokenType::LessEqual: op = Op::Le; break;
                case TokenType::Greater: op = Op::Gt; break;
                case TokenType::GreaterEqual: op = Op::Ge; break;
                default: break;
            }
            emit(op);
        }

    } else if (auto x = dynamic_cast<const InputExpr*>(&e)) {
        if (x->prompt) compile_expr(*x->prompt);
        emit2(Op::Read, input_type_id(x->type), x->prompt ? 1 : 0);

    } else if (auto x = dynamic_cast<const CallExpr*>(&e)) {
        for (const auto& a : x->args) compile_expr(*a);
        auto b = builtins_.find(x->name);
        if (b != builtins_.end()) {
            emit2(Op::Builtin, b->second, static_cast<int>(x->args.size()));
        } else {
            auto f = funcs_.find(x->name);
            if (f == funcs_.end()) throw ParseError("unknown function '" + x->name + "'");
            emit2(Op::Call, f->second, static_cast<int>(x->args.size()));
        }
    }
}
