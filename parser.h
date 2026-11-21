#pragma once
#include "common.h"
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

/* ---------- AST: expressions ---------- */
struct Expr { virtual ~Expr() = default; };
using ExprPtr = std::unique_ptr<Expr>;

struct NumExpr : Expr {
    double value;
    explicit NumExpr(double v) : value(v) {}
};
struct StrExpr : Expr {
    std::string value;
    explicit StrExpr(std::string v) : value(std::move(v)) {}
};
struct VarExpr : Expr {
    std::string name;
    explicit VarExpr(std::string n) : name(std::move(n)) {}
};
struct UnaryExpr : Expr {
    TokenType op;
    ExprPtr operand;
    UnaryExpr(TokenType o, ExprPtr a) : op(o), operand(std::move(a)) {}
};
struct BinaryExpr : Expr {
    TokenType op;
    ExprPtr left, right;
    BinaryExpr(TokenType o, ExprPtr a, ExprPtr b)
        : op(o), left(std::move(a)), right(std::move(b)) {}
};
struct CallExpr : Expr {
    std::string name;
    std::vector<ExprPtr> args;
    CallExpr(std::string n, std::vector<ExprPtr> a)
        : name(std::move(n)), args(std::move(a)) {}
};
struct InputExpr : Expr {
    std::string type;      // int | num | str | bool
    ExprPtr prompt;        // optional prompt text
    InputExpr(std::string t, ExprPtr p)
        : type(std::move(t)), prompt(std::move(p)) {}
};

/* ---------- AST: statements ---------- */
struct Stmt { virtual ~Stmt() = default; };
using StmtPtr = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

struct SayStmt : Stmt {
    ExprPtr value;
    explicit SayStmt(ExprPtr e) : value(std::move(e)) {}
};
struct VarStmt : Stmt {
    std::string name;
    ExprPtr value;
    VarStmt(std::string n, ExprPtr e) : name(std::move(n)), value(std::move(e)) {}
};
struct AssignStmt : Stmt {
    std::string name;
    ExprPtr value;
    AssignStmt(std::string n, ExprPtr e) : name(std::move(n)), value(std::move(e)) {}
};
struct CompoundStmt : Stmt {
    std::string name;
    TokenType op;
    ExprPtr value;
    CompoundStmt(std::string n, TokenType o, ExprPtr e)
        : name(std::move(n)), op(o), value(std::move(e)) {}
};
struct IfStmt : Stmt {
    ExprPtr cond;
    StmtList then_body, else_body;
    IfStmt(ExprPtr c, StmtList t, StmtList e)
        : cond(std::move(c)), then_body(std::move(t)), else_body(std::move(e)) {}
};
struct WhileStmt : Stmt {
    ExprPtr cond;
    StmtList body;
    WhileStmt(ExprPtr c, StmtList b) : cond(std::move(c)), body(std::move(b)) {}
};
struct RepeatStmt : Stmt {
    ExprPtr count;
    StmtList body;
    RepeatStmt(ExprPtr c, StmtList b) : count(std::move(c)), body(std::move(b)) {}
};
struct ForStmt : Stmt {
    StmtPtr init;
    ExprPtr cond;
    StmtPtr step;
    StmtList body;
    ForStmt(StmtPtr i, ExprPtr c, StmtPtr s, StmtList b)
        : init(std::move(i)), cond(std::move(c)), step(std::move(s)), body(std::move(b)) {}
};
struct BreakStmt : Stmt {};
struct ContinueStmt : Stmt {};
struct FuncStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    StmtList body;
    FuncStmt(std::string n, std::vector<std::string> p, StmtList b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};
struct GiveStmt : Stmt {
    ExprPtr value;
    explicit GiveStmt(ExprPtr e) : value(std::move(e)) {}
};
struct ExprStmt : Stmt {
    ExprPtr value;
    explicit ExprStmt(ExprPtr e) : value(std::move(e)) {}
};
struct BlockStmt : Stmt {
    StmtList statements;
    explicit BlockStmt(StmtList s) : statements(std::move(s)) {}
};
struct ImportStmt : Stmt {
    std::string path;
    explicit ImportStmt(std::string p) : path(std::move(p)) {}
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens, std::string base_dir = "",
                    std::shared_ptr<std::set<std::string>> imported = nullptr)
        : tokens_(tokens), pos_(0), base_dir_(std::move(base_dir)),
          imported_(imported ? imported : std::make_shared<std::set<std::string>>()) {}
    StmtList parse_program();
    const std::vector<std::string>& native_includes() const { return native_includes_; }

private:
    const std::vector<Token>& tokens_;
    size_t pos_;
    std::string base_dir_;
    std::shared_ptr<std::set<std::string>> imported_;
    std::vector<std::string> native_includes_;
    StmtList import_file(const std::string& path);

    const Token& cur() const { return tokens_[pos_]; }
    TokenType at() const { return tokens_[pos_].type; }
    Token advance() { return tokens_[pos_++]; }
    bool check(TokenType t) const { return at() == t; }
    bool match(TokenType t) { if (at() == t) { ++pos_; return true; } return false; }
    Token expect(TokenType t, const std::string& what);
    void eat_semi();

    StmtPtr statement();
    StmtPtr for_statement();
    StmtList block();

    ExprPtr expression();
    ExprPtr or_expr();
    ExprPtr and_expr();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr add();
    ExprPtr mul();
    ExprPtr unary();
    ExprPtr primary();
};
