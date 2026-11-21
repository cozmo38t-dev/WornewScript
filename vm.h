#pragma once
#include "common.h"
#include <chrono>
#include <deque>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

/* Compact value: 16 bytes instead of ~40.
 * s == nullptr -> number in n, otherwise a string. */
struct Value {
    double num;
    const char* str;

    Value() : num(0), str(nullptr) {}
    static Value number(double n) { Value v; v.num = n; v.str = nullptr; return v; }
    static Value text(const char* s) { Value v; v.num = 0; v.str = s; return v; }

    bool is_str() const { return str != nullptr; }
    bool truthy() const { return str ? str[0] != '\0' : num != 0.0; }
    std::string as_text() const;
    double as_num() const;
};

class Vm {
public:
    explicit Vm(const BytecodeProgram& prog);
    void run();

private:
    const BytecodeProgram& prog_;
    std::vector<Value> globals_;
    std::vector<Value> stack_;
    std::deque<std::string> heap_;   // owns concatenation strings
    uint32_t rng_seed_;
    std::chrono::steady_clock::time_point start_;

    struct Frame {
        const std::vector<int>* code;
        size_t ip;
        std::vector<Value> locals;
    };
    std::vector<Frame> frames_;

    Value concat(Value a, Value b);
    Value call_builtin(int id, const Value* args, int argc);
};
