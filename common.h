#pragma once
#include <string>
#include <vector>

enum class TokenType {
    Say, Var, If, Else, Elif, While, For, Func, Give, Use, Input, Import,
    Break, Continue, Repeat, True, False, And, Or, Not,
    String, Number, Identifier,
    Plus, Minus, Star, Slash, Percent,
    Equals, EqualsEquals, NotEquals,
    Greater, Less, GreaterEqual, LessEqual,
    AndAnd, OrOr, Bang,
    PlusEq, MinusEq, StarEq, SlashEq, PercentEq,
    Semi, Comma, Colon, LParen, RParen, LBrace, RBrace,
    Eof
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    Token(TokenType t, std::string v, int l)
        : type(t), value(std::move(v)), line(l) {}
};

/* Opcode values — must match the C runtime in native_compiler.cpp */
enum class Op : int {
    Nop = 0,
    PushNum = 1, PushStr = 2,
    LoadG = 3, StoreG = 4,
    LoadL = 5, StoreL = 6,
    Add = 7, Sub = 8, Mul = 9, Div = 10, Mod = 11,
    Neg = 12, Not = 13,
    Eq = 14, Ne = 15, Lt = 16, Le = 17, Gt = 18, Ge = 19,
    And = 20, Or = 21,
    Jmp = 22, JmpF = 23, JmpT = 24,
    Print = 25, Call = 26, Ret = 27, Halt = 28,
    Builtin = 29, Pop = 30, Read = 31
};

struct Function {
    std::string name;
    int arity = 0;
    int local_count = 0;
    std::vector<int> code;
};

struct BytecodeProgram {
    std::vector<int> main_code;
    std::vector<Function> functions;
    std::vector<double> numbers;
    std::vector<std::string> strings;
    std::vector<std::string> names;
};
