#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "native_compiler.h"
#include "direct_compiler.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

static std::string read_native_includes(const std::vector<std::string>& paths,
                                        const std::string& base_dir) {
    std::string out;
    for (const auto& p : paths) {
        std::filesystem::path fp(p);
        std::filesystem::path full = fp.is_absolute() ? fp : std::filesystem::path(base_dir) / fp;
        std::ifstream f(full, std::ios::binary);
        if (!f) throw std::runtime_error("C library not found: " + full.string());
        std::ostringstream oss;
        oss << f.rdbuf();
        out += "/* === " + p + " === */\n" + oss.str() + "\n";
    }
    return out;
}

/* The bytecode C runtime contains calls to all c_* (id 48..80), so
 * in this mode we include all C libraries from the import directory, not only
 * those encountered in the program. In direct C, only actually imported ones suffice. */
#ifndef WS_LIBS_DIR
#define WS_LIBS_DIR "libs"
#endif

/* The bytecode C runtime contains calls to all c_* (id 48..80), so
 * in this mode we always include all standard C libraries of the project. */
static std::vector<std::string> bytecode_native_paths(
        const std::vector<std::string>&, const std::string&) {
    std::vector<std::string> out;
    std::error_code ec;
    std::filesystem::path dir(WS_LIBS_DIR);
    for (std::filesystem::directory_iterator it(dir, ec), end; it != end; it.increment(ec)) {
        if (ec) break;
        if (it->path().extension() == ".c") out.push_back(it->path().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

static void usage() {
    std::cout << "WornewScript v0.6 (C++)\n\n"
              << "Usage:\n"
              << "  ./wornew <file.wor>                    execute (fast VM)\n"
              << "  ./wornew run <file.wor>                same thing\n"
              << "  ./wornew compile <file.wor> [-o name]  direct C (maximum speed)\n"
              << "  ./wornew compile <file.wor> -b -o name bytecode native\n"
              << "  ./wornew tokens <file.wor>             show tokens\n\n"
              << "New: input: int/num/str/bool, $APT imports (.wor and .c), windows (minigui),\n"
              << "     16 libraries in libs/wor/ and 16 C libraries in libs/,\n"
              << "     demo_big/main.wor example, logo.svg logo\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 0;
    }

    std::string cmd = argv[1];
    std::string path;
    if (cmd == "run" || cmd == "compile" || cmd == "tokens") {
        path.clear();
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-o") { ++i; continue; }
            if (!a.empty() && a[0] == '-') continue;
            if (path.empty()) path = a;
        }
        if (path.empty()) {
            std::cerr << "Error: '" << cmd << "' requires a .wor file\n";
            return 1;
        }
    } else {
        path = cmd;
        cmd = "run";
    }

    std::string source = read_file(path);
    if (source.empty() && !std::ifstream(path).good()) {
        std::cerr << "Error: file not found: " << path << "\n";
        return 1;
    }

    try {
        auto tokens = Lexer(source).tokenize();

        if (cmd == "tokens") {
            std::cout << "TOKENS:\n";
            for (const auto& t : tokens) std::cout << "  " << token_to_string(t) << '\n';
            return 0;
        }

        std::string base_dir = std::filesystem::path(path).parent_path().string();
        Parser parser(tokens, base_dir);
        auto ast = parser.parse_program();
        Compiler compiler;
        auto program = compiler.compile(ast);

        if (cmd == "compile") {
            std::filesystem::path out_path(path);
            if (out_path.has_extension()) out_path.replace_extension();
            std::string output = out_path.string();
            for (int i = 3; i < argc - 1; ++i) {
                if (std::string(argv[i]) == "-o") output = argv[i + 1];
            }
            bool bytecode_mode = false;
            for (int i = 3; i < argc; ++i) if (std::string(argv[i]) == "-b") bytecode_mode = true;
            std::string native = read_native_includes(
                bytecode_mode ? bytecode_native_paths(parser.native_includes(), base_dir)
                              : parser.native_includes(),
                base_dir);
            auto start = std::chrono::steady_clock::now();
            std::string message;
            bool ok;
            if (bytecode_mode)
                ok = NativeCompiler::build_source(native + NativeCompiler::generate(program),
                                                  output, message);
            else
                ok = NativeCompiler::build_source(native + DirectCompiler().generate(ast),
                                                  output, message);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (!ok) {
                std::cerr << "C compilation error: " << message << '\n';
                return 1;
            }
            std::cout << "Done: " << output << " (" << elapsed << " ms)\n";
            return 0;
        }

        auto start = std::chrono::steady_clock::now();
        Vm vm(program);
        vm.run();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start).count();
        std::cout << "\nTime: " << (elapsed / 1000.0) << " ms\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
