#pragma once
#include "common.h"
#include <string>

class NativeCompiler {
public:
    static std::string generate(const BytecodeProgram& prog);
    static bool build(const BytecodeProgram& prog, const std::string& output, std::string& message);
    static bool build_source(const std::string& source, const std::string& output, std::string& message);
};
