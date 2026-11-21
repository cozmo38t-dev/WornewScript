#include "parser.h"
#include "lexer.h"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

Token Parser::expect(TokenType t, const std::string& what) {
    if (at() != t) {
        std::string msg = "line " + std::to_string(cur().line) +
                          ": expected " + what + ", found " + token_to_string(cur());
        throw ParseError(msg);
    }
    return advance();
}

void Parser::eat_semi() {
    if (check(TokenType::Semi)) ++pos_;
}

StmtList Parser::import_file(const std::string& path) {
    std::filesystem::path full = std::filesystem::path(base_dir_) / path;
    std::error_code ec;
    std::string key = std::filesystem::weakly_canonical(full, ec).string();
    if (ec) key = full.string();
    if (imported_->count(key)) return {};   // guard against circular imports
    imported_->insert(key);

    std::ifstream f(full, std::ios::binary);
    if (!f) {
        throw ParseError("library not found: " + full.string());
    }
    std::ostringstream oss;
    oss << f.rdbuf();
    auto tokens = Lexer(oss.str()).tokenize();
    Parser sub(tokens, full.parent_path().string(), imported_);
    auto result = sub.parse_program();
    for (const auto& n : sub.native_includes_) native_includes_.push_back(n);
    return result;
}

static bool is_native_source(const std::string& path) {
    static const char* exts[] = { ".c", ".h", ".cc", ".cpp", ".hpp", ".cxx" };
    for (const char* e : exts) {
        if (path.size() >= std::strlen(e) &&
            path.compare(path.size() - std::strlen(e), std::strlen(e), e) == 0)
            return true;
    }
    return false;
}

StmtList Parser::parse_program() {
    StmtList list;
    while (at() != TokenType::Eof) {
        if (at() == TokenType::Import) {
            advance();
            std::string path = expect(TokenType::String, "library path").value;
            eat_semi();
            if (is_native_source(path)) {
                // C/C++ library: remember the resolved path so that
                // nested $APT imports from other directories also work.
                std::filesystem::path full = std::filesystem::path(base_dir_) / path;
                std::error_code ec;
                std::string resolved = std::filesystem::weakly_canonical(full, ec).string();
                native_includes_.push_back(ec ? full.string() : resolved);
            } else {
                auto added = import_file(path);
                for (auto& s : added) list.push_back(std::move(s));
            }
        } else {
            list.push_back(statement());
        }
    }
    return list;
}

StmtList Parser::block() {
    expect(TokenType::LBrace, "'{'");
    StmtList list;
    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        list.push_back(statement());
    }
    expect(TokenType::RBrace, "'}'");
    return list;
}

StmtPtr Parser::statement() {
    switch (at()) {
        case TokenType::Say: {
            advance();
            auto e = expression();
            eat_semi();
            return std::make_unique<SayStmt>(std::move(e));
        }
        case TokenType::Var: {
            advance();
            std::string name = expect(TokenType::Identifier, "variable name").value;
            ExprPtr value = std::make_unique<NumExpr>(0);
            if (match(TokenType::Equals)) value = expression();
            eat_semi();
            return std::make_unique<VarStmt>(std::move(name), std::move(value));
        }
        case TokenType::If:
        case TokenType::Elif: {
            advance();
            auto cond = expression();
            auto then_body = block();
            StmtList else_body;
            if (match(TokenType::Else)) {
                if (check(TokenType::If) || check(TokenType::Elif)) {
                    else_body.push_back(statement());
                } else {
                    else_body = block();
                }
            }
            return std::make_unique<IfStmt>(std::move(cond), std::move(then_body), std::move(else_body));
        }
        case TokenType::While: {
            advance();
            auto cond = expression();
            return std::make_unique<WhileStmt>(std::move(cond), block());
        }
        case TokenType::For:
            return for_statement();
        case TokenType::Import:
            throw ParseError("$APT can only be used at the top level of the program");
        case TokenType::Repeat: {
            advance();
            auto count = expression();
            return std::make_unique<RepeatStmt>(std::move(count), block());
        }
        case TokenType::Break:
            advance(); eat_semi();
            return std::make_unique<BreakStmt>();
        case TokenType::Continue:
            advance(); eat_semi();
            return std::make_unique<ContinueStmt>();
        case TokenType::Func: {
            advance();
            std::string name = expect(TokenType::Identifier, "function name").value;
            expect(TokenType::LParen, "'('");
            std::vector<std::string> params;
            if (!check(TokenType::RParen)) {
                do {
                    params.push_back(expect(TokenType::Identifier, "parameter name").value);
                } while (match(TokenType::Comma));
            }
            expect(TokenType::RParen, "')'");
            return std::make_unique<FuncStmt>(std::move(name), std::move(params), block());
        }
        case TokenType::Give: {
            advance();
            auto e = expression();
            eat_semi();
            return std::make_unique<GiveStmt>(std::move(e));
        }
        case TokenType::Use: {
            advance();
            std::string name = expect(TokenType::Identifier, "function name").value;
            std::vector<ExprPtr> args;
            if (match(TokenType::LParen)) {
                if (!check(TokenType::RParen)) {
                    do { args.push_back(expression()); } while (match(TokenType::Comma));
                }
                expect(TokenType::RParen, "')'");
            }
            eat_semi();
            return std::make_unique<ExprStmt>(std::make_unique<CallExpr>(std::move(name), std::move(args)));
        }
        case TokenType::LBrace:
            return std::make_unique<BlockStmt>(block());
        case TokenType::Identifier: {
            std::string name = cur().value;
            if (pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].type == TokenType::Equals) {
                advance(); advance();
                auto e = expression();
                eat_semi();
                return std::make_unique<AssignStmt>(std::move(name), std::move(e));
            }
            if (pos_ + 1 < tokens_.size() &&
                (tokens_[pos_ + 1].type == TokenType::PlusEq ||
                 tokens_[pos_ + 1].type == TokenType::MinusEq ||
                 tokens_[pos_ + 1].type == TokenType::StarEq ||
                 tokens_[pos_ + 1].type == TokenType::SlashEq ||
                 tokens_[pos_ + 1].type == TokenType::PercentEq)) {
                TokenType op = tokens_[pos_ + 1].type;
                advance(); advance();
                auto e = expression();
                eat_semi();
                return std::make_unique<CompoundStmt>(std::move(name), op, std::move(e));
            }
            auto ex = expression();
            eat_semi();
            return std::make_unique<ExprStmt>(std::move(ex));
        }
        default: {
            auto e = expression();
            eat_semi();
            return std::make_unique<ExprStmt>(std::move(e));
        }
    }
}

StmtPtr Parser::for_statement() {
    advance(); // 'for'
    expect(TokenType::LParen, "'('");

    StmtPtr init;
    if (!check(TokenType::Semi)) init = statement(); // statement() consumes ';'
    else advance();

    ExprPtr cond = std::make_unique<NumExpr>(1);
    if (!check(TokenType::Semi)) cond = expression();
    expect(TokenType::Semi, "';'");

    StmtPtr step;
    if (!check(TokenType::RParen)) step = statement();
    expect(TokenType::RParen, "')'");

    return std::make_unique<ForStmt>(std::move(init), std::move(cond),
                                     std::move(step), block());
}

ExprPtr Parser::expression() { return or_expr(); }

ExprPtr Parser::or_expr() {
    auto left = and_expr();
    while (at() == TokenType::Or || at() == TokenType::OrOr) {
        advance();
        left = std::make_unique<BinaryExpr>(TokenType::Or, std::move(left), and_expr());
    }
    return left;
}

ExprPtr Parser::and_expr() {
    auto left = equality();
    while (at() == TokenType::And || at() == TokenType::AndAnd) {
        advance();
        left = std::make_unique<BinaryExpr>(TokenType::And, std::move(left), equality());
    }
    return left;
}

ExprPtr Parser::equality() {
    auto left = comparison();
    while (at() == TokenType::EqualsEquals || at() == TokenType::NotEquals) {
        TokenType op = advance().type;
        left = std::make_unique<BinaryExpr>(op, std::move(left), comparison());
    }
    return left;
}

ExprPtr Parser::comparison() {
    auto left = add();
    while (at() == TokenType::Greater || at() == TokenType::GreaterEqual ||
           at() == TokenType::Less || at() == TokenType::LessEqual) {
        TokenType op = advance().type;
        left = std::make_unique<BinaryExpr>(op, std::move(left), add());
    }
    return left;
}

ExprPtr Parser::add() {
    auto left = mul();
    while (at() == TokenType::Plus || at() == TokenType::Minus) {
        TokenType op = advance().type;
        left = std::make_unique<BinaryExpr>(op, std::move(left), mul());
    }
    return left;
}

ExprPtr Parser::mul() {
    auto left = unary();
    while (at() == TokenType::Star || at() == TokenType::Slash || at() == TokenType::Percent) {
        TokenType op = advance().type;
        left = std::make_unique<BinaryExpr>(op, std::move(left), unary());
    }
    return left;
}

ExprPtr Parser::unary() {
    if (at() == TokenType::Minus || at() == TokenType::Bang || at() == TokenType::Not) {
        TokenType op = advance().type;
        return std::make_unique<UnaryExpr>(op, unary());
    }
    return primary();
}

ExprPtr Parser::primary() {
    switch (at()) {
        case TokenType::Number: {
            std::string s = advance().value;
            return std::make_unique<NumExpr>(std::stod(s));
        }
        case TokenType::String:
            return std::make_unique<StrExpr>(advance().value);
        case TokenType::Input: {
            advance();
            ExprPtr prompt;
            if (match(TokenType::LParen)) {
                prompt = expression();
                expect(TokenType::RParen, "')'");
            }
            std::string type = "num";
            if (match(TokenType::Colon)) {
                if (at() != TokenType::Identifier)
                    throw ParseError("line " + std::to_string(cur().line) +
                                     ": expected a type after ':'");
                type = advance().value;
            }
            if (type == "float" || type == "double") type = "num";
            if (type == "string") type = "str";
            if (type == "boolean") type = "bool";
            return std::make_unique<InputExpr>(type, std::move(prompt));
        }
        case TokenType::True:
            advance(); return std::make_unique<NumExpr>(1);
        case TokenType::False:
            advance(); return std::make_unique<NumExpr>(0);
        case TokenType::LParen: {
            advance();
            auto e = expression();
            expect(TokenType::RParen, "')'");
            return e;
        }
        case TokenType::Identifier: {
            std::string name = advance().value;
            if (match(TokenType::LParen)) {
                std::vector<ExprPtr> args;
                if (!check(TokenType::RParen)) {
                    do { args.push_back(expression()); } while (match(TokenType::Comma));
                }
                expect(TokenType::RParen, "')'");
                return std::make_unique<CallExpr>(std::move(name), std::move(args));
            }
            return std::make_unique<VarExpr>(std::move(name));
        }
        default:
            throw ParseError("line " + std::to_string(cur().line) +
                             ": unexpected token " + token_to_string(cur()));
    }
}
