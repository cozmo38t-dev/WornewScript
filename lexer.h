#pragma once
#include "common.h"
#include <string>
#include <vector>

std::string token_type_name(TokenType t);
std::string token_to_string(const Token& t);

class Lexer {
public:
    explicit Lexer(std::string source) : src_(std::move(source)) {}
    std::vector<Token> tokenize();

private:
    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;

    static bool is_id_start(char c);
    static bool is_id_part(char c);
    static TokenType keyword(const std::string& word);
    void push(std::vector<Token>& out, TokenType type, std::string text, int line, size_t len);
};
