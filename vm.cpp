#include "vm.h"
#include "compiler.h"
#include "gui_bindings.h"
#include "input_bindings.h"
#include "native_calls.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

static std::string fmt_num(double n) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.15g", n);
    return std::string(buf);
}

std::string Value::as_text() const {
    return str ? std::string(str) : fmt_num(num);
}

double Value::as_num() const {
    if (!str) return num;
    try {
        return std::stod(str);
    } catch (...) {
        return 0.0;
    }
}

Vm::Vm(const BytecodeProgram& prog)
    : prog_(prog),
      globals_(prog.names.size()),
      stack_(8192),
      rng_seed_(12345u),
      start_(std::chrono::steady_clock::now()) {}

Value Vm::concat(Value a, Value b) {
    std::string sa = a.as_text(), sb = b.as_text();
    heap_.push_back(sa + sb);
    return Value::text(heap_.back().c_str());
}

Value Vm::call_builtin(int id, const Value* a, int argc) {
    auto N = [&](int i) -> double { return i < argc ? a[i].as_num() : 0.0; };
    const std::string& name = Compiler::BUILTINS[id];
    if (name == "sqrt") return Value::number(std::sqrt(N(0)));
    if (name == "abs") return Value::number(std::fabs(N(0)));
    if (name == "pow") return Value::number(std::pow(N(0), N(1)));
    if (name == "min") return Value::number(std::min(N(0), N(1)));
    if (name == "max") return Value::number(std::max(N(0), N(1)));
    if (name == "int") return Value::number(std::trunc(N(0)));
    if (name == "len") {
        if (argc > 0 && a[0].is_str()) return Value::number(static_cast<double>(std::strlen(a[0].str)));
        return Value::number(static_cast<double>(fmt_num(N(0)).size()));
    }
    if (name == "str") {
        heap_.push_back(argc > 0 ? a[0].as_text() : std::string(""));
        return Value::text(heap_.back().c_str());
    }
    if (name == "clock") {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_).count();
        return Value::number(static_cast<double>(ms));
    }
    if (name == "rand") {
        rng_seed_ = rng_seed_ * 1103515245u + 12345u;
        return Value::number(static_cast<double>(rng_seed_ & 0x7fffffffu) / 2147483648.0);
    }
    if (name == "sin") return Value::number(std::sin(N(0)));
    if (name == "cos") return Value::number(std::cos(N(0)));
    if (name == "tan") return Value::number(std::tan(N(0)));
    if (name == "floor") return Value::number(std::floor(N(0)));
    if (name == "ceil") return Value::number(std::ceil(N(0)));
    if (name == "round") return Value::number(std::round(N(0)));
    if (name == "log") return Value::number(std::log(N(0)));
    if (name == "log10") return Value::number(std::log10(N(0)));
    if (name == "exp") return Value::number(std::exp(N(0)));
    if (name == "sign") return Value::number(N(0) > 0 ? 1 : (N(0) < 0 ? -1 : 0));
    if (name == "clamp") return Value::number(std::clamp(N(0), N(1), N(2)));

    /* C libraries (id 48+) and GUI are moved to separate TUs — they don't interfere
     * with optimization of the VM's hot loop. */
    if (id >= 48) return call_native_builtin(name, a, argc);
    if (id >= 21) return gui_builtin(name, a, argc, heap_);
    return Value::number(0);
}

void Vm::run() {
    frames_.clear();
    frames_.push_back(Frame{&prog_.main_code, 0, {}});
    size_t fi = 0;
    size_t sp = 0;

    Value* st = stack_.data();

#define PUSH(v) do { if (sp >= stack_.size()) throw std::runtime_error("VM stack overflow"); \
                     st[sp++] = (v); } while (0)
#define POP()  (sp == 0 ? (throw std::runtime_error("VM stack underflow"), Value()) : st[--sp])

    for (;;) {
        const std::vector<int>& code = *frames_[fi].code;
        size_t& ip = frames_[fi].ip;
        Op op = static_cast<Op>(code[ip++]);

        switch (op) {
            case Op::Nop: break;

            case Op::PushNum: PUSH(Value::number(prog_.numbers[code[ip++]])); break;
            case Op::PushStr: PUSH(Value::text(prog_.strings[code[ip++]].c_str())); break;

            case Op::LoadG: PUSH(globals_[code[ip++]]); break;
            case Op::StoreG: globals_[code[ip++]] = POP(); break;
            case Op::LoadL: PUSH(frames_[fi].locals[code[ip++]]); break;
            case Op::StoreL: frames_[fi].locals[code[ip++]] = POP(); break;

            case Op::Add: {
                Value b = POP(), a = POP();
                PUSH((a.is_str() || b.is_str()) ? concat(a, b) : Value::number(a.num + b.num));
                break;
            }
            case Op::Sub: { double b = POP().as_num(), a = POP().as_num(); PUSH(Value::number(a - b)); break; }
            case Op::Mul: { double b = POP().as_num(), a = POP().as_num(); PUSH(Value::number(a * b)); break; }
            case Op::Div: { double b = POP().as_num(), a = POP().as_num(); PUSH(Value::number(b == 0 ? 0 : a / b)); break; }
            case Op::Mod: { double b = POP().as_num(), a = POP().as_num(); PUSH(Value::number(b == 0 ? 0 : std::fmod(a, b))); break; }
            case Op::Neg: st[sp - 1] = Value::number(-st[sp - 1].as_num()); break;
            case Op::Not: st[sp - 1] = Value::number(st[sp - 1].truthy() ? 0 : 1); break;

            case Op::Eq:
            case Op::Ne: {
                Value b = POP(), a = POP();
                bool eq = (a.is_str() || b.is_str())
                    ? std::strcmp(a.str ? a.str : "", b.str ? b.str : "") == 0
                    : a.num == b.num;
                PUSH(Value::number((op == Op::Eq) == eq ? 1 : 0));
                break;
            }
            case Op::Lt:
            case Op::Le:
            case Op::Gt:
            case Op::Ge: {
                Value b = POP(), a = POP();
                int cmp;
                if (a.is_str() || b.is_str()) {
                    cmp = std::strcmp(a.str ? a.str : "", b.str ? b.str : "");
                } else {
                    cmp = a.num < b.num ? -1 : (a.num > b.num ? 1 : 0);
                }
                bool r = false;
                switch (op) {
                    case Op::Lt: r = cmp < 0; break;
                    case Op::Le: r = cmp <= 0; break;
                    case Op::Gt: r = cmp > 0; break;
                    case Op::Ge: r = cmp >= 0; break;
                    default: break;
                }
                PUSH(Value::number(r ? 1 : 0));
                break;
            }
            case Op::And: { Value b = POP(), a = POP(); PUSH(Value::number(a.truthy() && b.truthy() ? 1 : 0)); break; }
            case Op::Or:  { Value b = POP(), a = POP(); PUSH(Value::number(a.truthy() || b.truthy() ? 1 : 0)); break; }

            case Op::Jmp: ip = static_cast<size_t>(code[ip]); break;
            case Op::JmpF: { int t = code[ip++]; if (!POP().truthy()) ip = static_cast<size_t>(t); break; }
            case Op::JmpT: { int t = code[ip++]; if (POP().truthy()) ip = static_cast<size_t>(t); break; }

            case Op::Print: std::cout << POP().as_text() << '\n'; break;
            case Op::Pop: --sp; break;

            case Op::Read: {
                int tid = code[ip++], has_prompt = code[ip++];
                Value pr = has_prompt ? POP() : Value::number(0);
                PUSH(read_value(tid, has_prompt != 0, pr, heap_));
                break;
            }

            case Op::Call: {
                int fid = code[ip++], argc = code[ip++];
                const Function& fn = prog_.functions[fid];
                int slots = std::min(argc, fn.local_count);
                if (frames_.size() >= 1024)
                    throw std::runtime_error("recursion too deep");
                frames_.push_back(Frame{&fn.code, 0, std::vector<Value>(fn.local_count)});
                fi = frames_.size() - 1;
                for (int i = argc - 1; i >= 0; --i) {
                    Value v = POP();
                    if (i < slots) frames_[fi].locals[i] = v;
                }
                break;
            }
            case Op::Ret: {
                if (fi == 0) return;
                frames_.pop_back();
                --fi;
                break;
            }
            case Op::Builtin: {
                int bid = code[ip++], argc = code[ip++];
                Value args[8];
                int n = std::min(argc, 8);
                for (int i = argc - 1; i >= 0; --i) {
                    Value v = POP();
                    if (i < n) args[i] = v;
                }
                PUSH(call_builtin(bid, args, n));
                break;
            }
            case Op::Halt: return;
        }
    }
#undef PUSH
#undef POP
}
