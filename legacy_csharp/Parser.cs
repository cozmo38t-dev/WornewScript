using System;
using System.Collections.Generic;

/* ---------- AST ---------- */

public abstract class Expr { }
public sealed class NumExpr : Expr { public readonly double Value; public NumExpr(double v) { Value = v; } }
public sealed class StrExpr : Expr { public readonly string Value; public StrExpr(string v) { Value = v; } }
public sealed class VarExpr : Expr { public readonly string Name; public VarExpr(string n) { Name = n; } }
public sealed class UnaryExpr : Expr {
    public readonly TokenType Op; public readonly Expr Operand;
    public UnaryExpr(TokenType op, Expr a) { Op = op; Operand = a; }
}
public sealed class BinaryExpr : Expr {
    public readonly TokenType Op; public readonly Expr Left, Right;
    public BinaryExpr(TokenType op, Expr a, Expr b) { Op = op; Left = a; Right = b; }
}
public sealed class CallExpr : Expr {
    public readonly string Name; public readonly List<Expr> Args;
    public CallExpr(string name, List<Expr> args) { Name = name; Args = args; }
}

public abstract class Stmt { }
public sealed class SayStmt : Stmt { public readonly Expr Value; public SayStmt(Expr e) { Value = e; } }
public sealed class VarStmt : Stmt { public readonly string Name; public readonly Expr Value; public VarStmt(string n, Expr e) { Name = n; Value = e; } }
public sealed class AssignStmt : Stmt { public readonly string Name; public readonly Expr Value; public AssignStmt(string n, Expr e) { Name = n; Value = e; } }
public sealed class CompoundStmt : Stmt {
    public readonly string Name; public readonly TokenType Op; public readonly Expr Value;
    public CompoundStmt(string n, TokenType op, Expr e) { Name = n; Op = op; Value = e; }
}
public sealed class IfStmt : Stmt {
    public readonly Expr Cond; public readonly List<Stmt> Then, Else;
    public IfStmt(Expr c, List<Stmt> t, List<Stmt>? e) { Cond = c; Then = t; Else = e ?? new List<Stmt>(); }
}
public sealed class WhileStmt : Stmt { public readonly Expr Cond; public readonly List<Stmt> Body; public WhileStmt(Expr c, List<Stmt> b) { Cond = c; Body = b; } }
public sealed class RepeatStmt : Stmt { public readonly Expr Count; public readonly List<Stmt> Body; public RepeatStmt(Expr c, List<Stmt> b) { Count = c; Body = b; } }
public sealed class BreakStmt : Stmt { }
public sealed class ContinueStmt : Stmt { }
public sealed class FuncStmt : Stmt {
    public readonly string Name; public readonly List<string> Params; public readonly List<Stmt> Body;
    public FuncStmt(string n, List<string> p, List<Stmt> b) { Name = n; Params = p; Body = b; }
}
public sealed class GiveStmt : Stmt { public readonly Expr Value; public GiveStmt(Expr e) { Value = e; } }
public sealed class ExprStmt : Stmt { public readonly Expr Value; public ExprStmt(Expr e) { Value = e; } }

public sealed class ParseException : Exception {
    public ParseException(string message) : base(message) { }
}

/* ---------- Parser ---------- */

public sealed class Parser {
    private readonly List<Token> _t;
    private int _p;

    public Parser(List<Token> tokens) { _t = tokens; _p = 0; }

    private Token Cur => _t[_p];
    private TokenType At => _t[_p].Type;
    private Token Next() => _t[_p++];
    private bool Check(TokenType type) => At == type;
    private bool Match(TokenType type) { if (At == type) { _p++; return true; } return false; }
    private Token Expect(TokenType type, string what) {
        if (At != type) throw new ParseException($"line {Cur.Line}: expected {what}, found {Cur}");
        return Next();
    }

    public List<Stmt> ParseProgram() {
        var list = new List<Stmt>();
        while (At != TokenType.EOF) list.Add(Statement());
        return list;
    }

    private List<Stmt> Block() {
        Expect(TokenType.LBrace, "'{'");
        var list = new List<Stmt>();
        while (!Check(TokenType.RBrace) && !Check(TokenType.EOF)) list.Add(Statement());
        Expect(TokenType.RBrace, "'}'");
        return list;
    }

    private void EatSemi() {
        if (Check(TokenType.Semi)) _p++;
    }

    private Stmt Statement() {
        switch (At) {
            case TokenType.Say: {
                Next();
                var e = Expression();
                EatSemi();
                return new SayStmt(e);
            }
            case TokenType.Var: {
                Next();
                string name = Expect(TokenType.Identifier, "variable name").Value;
                Expr value = new NumExpr(0);
                if (Match(TokenType.Equals)) value = Expression();
                EatSemi();
                return new VarStmt(name, value);
            }
            case TokenType.If: {
                Next();
                var cond = Expression();
                var then = Block();
                var otherwise = new List<Stmt>();
                if (Match(TokenType.Else)) {
                    if (Check(TokenType.If)) otherwise.Add(Statement());
                    else otherwise = Block();
                }
                return new IfStmt(cond, then, otherwise);
            }
            case TokenType.While: {
                Next();
                var cond = Expression();
                return new WhileStmt(cond, Block());
            }
            case TokenType.Repeat: {
                Next();
                var count = Expression();
                return new RepeatStmt(count, Block());
            }
            case TokenType.Break:
                Next(); EatSemi(); return new BreakStmt();
            case TokenType.Continue:
                Next(); EatSemi(); return new ContinueStmt();
            case TokenType.Func: {
                Next();
                string name = Expect(TokenType.Identifier, "function name").Value;
                Expect(TokenType.LParen, "'('");
                var pars = new List<string>();
                if (!Check(TokenType.RParen)) {
                    do { pars.Add(Expect(TokenType.Identifier, "parameter name").Value); }
                    while (Match(TokenType.Comma));
                }
                Expect(TokenType.RParen, "')'");
                return new FuncStmt(name, pars, Block());
            }
            case TokenType.Give: {
                Next();
                var e = Expression();
                EatSemi();
                return new GiveStmt(e);
            }
            case TokenType.Use: {
                Next();
                string name = Expect(TokenType.Identifier, "function name").Value;
                var args = new List<Expr>();
                if (Match(TokenType.LParen)) {
                    if (!Check(TokenType.RParen)) {
                        do { args.Add(Expression()); } while (Match(TokenType.Comma));
                    }
                    Expect(TokenType.RParen, "')'");
                }
                EatSemi();
                return new ExprStmt(new CallExpr(name, args));
            }
            case TokenType.LBrace:
                return new BlockStmt(Block());
            case TokenType.Identifier: {
                string name = Cur.Value;
                if (_p + 1 < _t.Count && _t[_p + 1].Type == TokenType.Equals) {
                    Next(); Next();
                    var e = Expression();
                    EatSemi();
                    return new AssignStmt(name, e);
                }
                if (_p + 1 < _t.Count &&
                    (_t[_p + 1].Type == TokenType.PlusEq || _t[_p + 1].Type == TokenType.MinusEq ||
                     _t[_p + 1].Type == TokenType.StarEq || _t[_p + 1].Type == TokenType.SlashEq)) {
                    var op = _t[_p + 1].Type;
                    Next(); Next();
                    var e = Expression();
                    EatSemi();
                    return new CompoundStmt(name, op, e);
                }
                var ex = Expression();
                EatSemi();
                return new ExprStmt(ex);
            }
            default: {
                var e = Expression();
                EatSemi();
                return new ExprStmt(e);
            }
        }
    }

    /* ----- expressions ----- */

    private Expr Expression() => Or();

    private Expr Or() {
        var left = And();
        while (At == TokenType.Or || At == TokenType.OrOr) {
            Next(); left = new BinaryExpr(TokenType.Or, left, And());
        }
        return left;
    }

    private Expr And() {
        var left = Equality();
        while (At == TokenType.And || At == TokenType.AndAnd) {
            Next(); left = new BinaryExpr(TokenType.And, left, Equality());
        }
        return left;
    }

    private Expr Equality() {
        var left = Comparison();
        while (At == TokenType.EqualsEquals || At == TokenType.NotEquals) {
            var op = Next().Type;
            left = new BinaryExpr(op, left, Comparison());
        }
        return left;
    }

    private Expr Comparison() {
        var left = Add();
        while (At == TokenType.Greater || At == TokenType.GreaterEqual ||
               At == TokenType.Less || At == TokenType.LessEqual) {
            var op = Next().Type;
            left = new BinaryExpr(op, left, Add());
        }
        return left;
    }

    private Expr Add() {
        var left = Mul();
        while (At == TokenType.Plus || At == TokenType.Minus) {
            var op = Next().Type;
            left = new BinaryExpr(op, left, Mul());
        }
        return left;
    }

    private Expr Mul() {
        var left = Unary();
        while (At == TokenType.Star || At == TokenType.Slash || At == TokenType.Percent) {
            var op = Next().Type;
            left = new BinaryExpr(op, left, Unary());
        }
        return left;
    }

    private Expr Unary() {
        if (At == TokenType.Minus || At == TokenType.Bang || At == TokenType.Not) {
            var op = Next().Type;
            return new UnaryExpr(op, Unary());
        }
        return Primary();
    }

    private Expr Primary() {
        switch (At) {
            case TokenType.Number:
                return new NumExpr(double.Parse(Next().Value, System.Globalization.CultureInfo.InvariantCulture));
            case TokenType.String:
                return new StrExpr(Next().Value);
            case TokenType.True:
                Next(); return new NumExpr(1);
            case TokenType.False:
                Next(); return new NumExpr(0);
            case TokenType.LParen: {
                Next();
                var e = Expression();
                Expect(TokenType.RParen, "')'");
                return e;
            }
            case TokenType.Identifier: {
                string name = Next().Value;
                if (Match(TokenType.LParen)) {
                    var args = new List<Expr>();
                    if (!Check(TokenType.RParen)) {
                        do { args.Add(Expression()); } while (Match(TokenType.Comma));
                    }
                    Expect(TokenType.RParen, "')'");
                    return new CallExpr(name, args);
                }
                return new VarExpr(name);
            }
            default:
                throw new ParseException($"line {Cur.Line}: unexpected token {Cur}");
        }
    }
}

/* block as a statement-expression (for standalone '{ ... }') */
public sealed class BlockStmt : Stmt {
    public readonly List<Stmt> Statements;
    public BlockStmt(List<Stmt> s) { Statements = s; }
}
