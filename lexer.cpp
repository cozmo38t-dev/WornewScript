#include "lexer.h"
#include <cctype>
#include <sstream>
#include <stdexcept>

std::string token_type_name(TokenType t) {
    switch (t) {
        case TokenType::Say: return "Say";
        case TokenType::Var: return "Var";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::Elif: return "Elif";
        case TokenType::While: return "While";
        case TokenType::For: return "For";
        case TokenType::Func: return "Func";
        case TokenType::Give: return "Give";
        case TokenType::Use: return "Use";
        case TokenType::Input: return "Input";
        case TokenType::Import: return "Import";
        case TokenType::Break: return "Break";
        case TokenType::Continue: return "Continue";
        case TokenType::Repeat: return "Repeat";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::And: return "And";
        case TokenType::Or: return "Or";
        case TokenType::Not: return "Not";
        case TokenType::String: return "String";
        case TokenType::Number: return "Number";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::Equals: return "Equals";
        case TokenType::EqualsEquals: return "EqualsEquals";
        case TokenType::NotEquals: return "NotEquals";
        case TokenType::Greater: return "Greater";
        case TokenType::Less: return "Less";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::AndAnd: return "AndAnd";
        case TokenType::OrOr: return "OrOr";
        case TokenType::Bang: return "Bang";
        case TokenType::PlusEq: return "PlusEq";
        case TokenType::MinusEq: return "MinusEq";
        case TokenType::StarEq: return "StarEq";
        case TokenType::SlashEq: return "SlashEq";
        case TokenType::PercentEq: return "PercentEq";
        case TokenType::Semi: return "Semi";
        case TokenType::Comma: return "Comma";
        case TokenType::Colon: return "Colon";
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::Eof: return "EOF";
    }
    return "?";
}

std::string token_to_string(const Token& t) {
    return token_type_name(t.type) + "(" + t.value + ")";
}

bool Lexer::is_id_start(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    return std::isalpha(u) || c == '_' || u >= 0x80;
}

bool Lexer::is_id_part(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    return std::isalnum(u) || c == '_' || u >= 0x80;
}

TokenType Lexer::keyword(const std::string& w) {
    if (w == "say") return TokenType::Say;
    if (w == "var") return TokenType::Var;
    if (w == "if") return TokenType::If;
    if (w == "else") return TokenType::Else;
    if (w == "elif") return TokenType::Elif;
    if (w == "while") return TokenType::While;
    if (w == "for") return TokenType::For;
    if (w == "repeat") return TokenType::Repeat;
    if (w == "func") return TokenType::Func;
    if (w == "give") return TokenType::Give;
    if (w == "use") return TokenType::Use;
    if (w == "input") return TokenType::Input;
    if (w == "break") return TokenType::Break;
    if (w == "continue") return TokenType::Continue;
    if (w == "true") return TokenType::True;
    if (w == "false") return TokenType::False;
    if (w == "and") return TokenType::And;
    if (w == "or") return TokenType::Or;
    if (w == "not") return TokenType::Not;
    return TokenType::Identifier;
}

void Lexer::push(std::vector<Token>& out, TokenType type, std::string text, int line, size_t len) {
    out.emplace_back(type, std::move(text), line);
    pos_ += len;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(256);
    const size_t n = src_.size();

    // UTF-8 BOM (Windows) must not end up in the first name.
    if (n >= 3 && static_cast<unsigned char>(src_[0]) == 0xEF &&
        static_cast<unsigned char>(src_[1]) == 0xBB &&
        static_cast<unsigned char>(src_[2]) == 0xBF) pos_ = 3;

    while (pos_ < n) {
        char c = src_[pos_];

        if (c == '\n') { ++line_; ++pos_; continue; }
        if (c == ' ' || c == '\t' || c == '\r') { ++pos_; continue; }

        // comments
        if (c == '/' && pos_ + 1 < n && src_[pos_ + 1] == '/') {
            while (pos_ < n && src_[pos_] != '\n') ++pos_;
            continue;
        }
        if (c == '#') {
            while (pos_ < n && src_[pos_] != '\n') ++pos_;
            continue;
        }

        // $APT "libs/x.wor" — library import (idea from CurrtTalk)
        if (c == '$' && pos_ + 3 < n && src_.compare(pos_ + 1, 3, "APT") == 0) {
            push(tokens, TokenType::Import, "$APT", line_, 4);
            continue;
        }

        // number
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '.' && pos_ + 1 < n && std::isdigit(static_cast<unsigned char>(src_[pos_ + 1])))) {
            size_t start = pos_;
            while (pos_ < n && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
            if (pos_ < n && src_[pos_] == '.') {
                ++pos_;
                while (pos_ < n && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
            }
            if (pos_ < n && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
                size_t save = pos_;
                ++pos_;
                if (pos_ < n && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
                if (pos_ < n && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                    while (pos_ < n && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
                } else {
                    pos_ = save;
                }
            }
            tokens.emplace_back(TokenType::Number, src_.substr(start, pos_ - start), line_);
            continue;
        }

        // string
        if (c == '"') {
            int str_line = line_;
            ++pos_;
            std::string s;
            bool closed = false;
            while (pos_ < n) {
                char ch = src_[pos_++];
                if (ch == '"') { closed = true; break; }
                if (ch == '\\' && pos_ < n) {
                    char e = src_[pos_++];
                    switch (e) {
                        case 'n': s += '\n'; break;
                        case 't': s += '\t'; break;
                        case 'r': s += '\r'; break;
                        case '0': s += '\0'; break;
                        case '"': s += '"'; break;
                        case '\\': s += '\\'; break;
                        default: s += e; break;
                    }
                } else {
                    if (ch == '\n') ++line_;
                    s += ch;
                }
            }
            if (!closed)
                throw std::runtime_error("unterminated string on line " + std::to_string(str_line));
            tokens.emplace_back(TokenType::String, std::move(s), str_line);
            continue;
        }

        // identifier / keyword
        if (is_id_start(c)) {
            size_t start = pos_;
            while (pos_ < n && is_id_part(src_[pos_])) ++pos_;
            std::string word = src_.substr(start, pos_ - start);
            tokens.emplace_back(keyword(word), word, line_);
            continue;
        }

        int ln = line_;
        char next = pos_ + 1 < n ? src_[pos_ + 1] : '\0';

        switch (c) {
            case '+':
                if (next == '=') push(tokens, TokenType::PlusEq, "+=", ln, 2);
                else push(tokens, TokenType::Plus, "+", ln, 1);
                continue;
            case '-':
                if (next == '=') push(tokens, TokenType::MinusEq, "-=", ln, 2);
                else push(tokens, TokenType::Minus, "-", ln, 1);
                continue;
            case '*':
                if (next == '=') push(tokens, TokenType::StarEq, "*=", ln, 2);
                else push(tokens, TokenType::Star, "*", ln, 1);
                continue;
            case '/':
                if (next == '=') push(tokens, TokenType::SlashEq, "/=", ln, 2);
                else push(tokens, TokenType::Slash, "/", ln, 1);
                continue;
            case '%':
                if (next == '=') push(tokens, TokenType::PercentEq, "%=", ln, 2);
                else push(tokens, TokenType::Percent, "%", ln, 1);
                continue;
            case '=':
                if (next == '=') push(tokens, TokenType::EqualsEquals, "==", ln, 2);
                else push(tokens, TokenType::Equals, "=", ln, 1);
                continue;
            case '!':
                if (next == '=') push(tokens, TokenType::NotEquals, "!=", ln, 2);
                else push(tokens, TokenType::Bang, "!", ln, 1);
                continue;
            case '>':
                if (next == '=') push(tokens, TokenType::GreaterEqual, ">=", ln, 2);
                else push(tokens, TokenType::Greater, ">", ln, 1);
                continue;
            case '<':
                if (next == '=') push(tokens, TokenType::LessEqual, "<=", ln, 2);
                else push(tokens, TokenType::Less, "<", ln, 1);
                continue;
            case '&':
                if (next == '&') { push(tokens, TokenType::AndAnd, "&&", ln, 2); continue; }
                throw std::runtime_error("single '&' on line " + std::to_string(ln) + " ('&&' required)");
            case '|':
                if (next == '|') { push(tokens, TokenType::OrOr, "||", ln, 2); continue; }
                throw std::runtime_error("single '|' on line " + std::to_string(ln) + " ('||' required)");
            case ';': push(tokens, TokenType::Semi, ";", ln, 1); continue;
            case ',': push(tokens, TokenType::Comma, ",", ln, 1); continue;
            case ':': push(tokens, TokenType::Colon, ":", ln, 1); continue;
            case '(': push(tokens, TokenType::LParen, "(", ln, 1); continue;
            case ')': push(tokens, TokenType::RParen, ")", ln, 1); continue;
            case '{': push(tokens, TokenType::LBrace, "{", ln, 1); continue;
            case '}': push(tokens, TokenType::RBrace, "}", ln, 1); continue;
        }

        throw std::runtime_error("unknown character '" + std::string(1, c) +
                                 "' on line " + std::to_string(ln));
    }

    tokens.emplace_back(TokenType::Eof, "", line_);
    return tokens;
}
