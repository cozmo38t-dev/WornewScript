using System;
using System.Collections.Generic;
using System.Globalization;

/* AST -> bytecode compiler. Single pass over the tree, no string
 * recomputation at runtime. */
public sealed class Compiler {
    public static readonly string[] Builtins = {
        "sqrt", "abs", "pow", "min", "max", "len", "int", "str", "clock", "rand"
    };

    private readonly BytecodeProgram _p = new BytecodeProgram();
    private readonly Dictionary<string, int> _globals = new Dictionary<string, int>();
    private readonly Dictionary<string, int> _funcs = new Dictionary<string, int>();
    private readonly Dictionary<string, int> _builtins = new Dictionary<string, int>();
    private readonly Dictionary<string, int> _numPool = new Dictionary<string, int>();
    private readonly Dictionary<string, int> _strPool = new Dictionary<string, int>();
    private readonly List<Dictionary<string, int>> _scopes = new List<Dictionary<string, int>>();
    private readonly List<LoopCtx> _loops = new List<LoopCtx>();
    private List<int> _code = new List<int>();
    private int _localCount;
    private int _repCounter;

    private sealed class LoopCtx {
        public readonly List<int> Breaks = new List<int>();
        public readonly List<int> Continues = new List<int>();
    }

    public Compiler() {
        for (int i = 0; i < Builtins.Length; i++) _builtins[Builtins[i]] = i;
    }

    public BytecodeProgram Compile(List<Stmt> statements) {
        // register all functions first — calls before declaration are allowed
        foreach (var s in statements) {
            if (s is FuncStmt f && !_funcs.ContainsKey(f.Name)) {
                _funcs[f.Name] = _p.Functions.Count;
                _p.Functions.Add(new Function { Name = f.Name, Arity = f.Params.Count });
            }
        }

        _code = _p.MainCode;
        _localCount = 0;
        CompileBlock(statements);
        Emit(Op.Halt);

        foreach (var s in statements) {
            if (s is not FuncStmt fn) continue;
            int idx = _funcs[fn.Name];
            var func = _p.Functions[idx];
            _code = func.Code;
            _localCount = 0;
            _scopes.Clear();
            PushScope();
            foreach (string par in fn.Params) DeclareLocal(par);
            CompileBlock(fn.Body);
            EmitNum(0);
            Emit(Op.Ret);
            func.LocalCount = _localCount;
            PopScope();
        }

        return _p;
    }

    /* ----- pools ----- */

    private int NumIndex(double v) {
        string key = v.ToString("R", CultureInfo.InvariantCulture);
        if (_numPool.TryGetValue(key, out int i)) return i;
        i = _p.Numbers.Count;
        _p.Numbers.Add(v);
        _numPool[key] = i;
        return i;
    }

    private int StrIndex(string s) {
        if (_strPool.TryGetValue(s, out int i)) return i;
        i = _p.Strings.Count;
        _p.Strings.Add(s);
        _strPool[s] = i;
        return i;
    }

    private int GlobalIndex(string name) {
        if (_globals.TryGetValue(name, out int i)) return i;
        i = _p.Names.Count;
        _p.Names.Add(name);
        _globals[name] = i;
        return i;
    }

    /* ----- locals ----- */

    private void PushScope() => _scopes.Add(new Dictionary<string, int>());
    private void PopScope() => _scopes.RemoveAt(_scopes.Count - 1);

    private int DeclareLocal(string name) {
        int slot = _localCount++;
        if (_scopes.Count > 0) _scopes[_scopes.Count - 1][name] = slot;
        return slot;
    }

    private bool TryLocal(string name, out int slot) {
        for (int i = _scopes.Count - 1; i >= 0; --i) {
            if (_scopes[i].TryGetValue(name, out slot)) return true;
        }
        slot = 0;
        return false;
    }

    private void Load(string name) {
        if (TryLocal(name, out int slot)) Emit1(Op.LoadL, slot);
        else Emit1(Op.LoadG, GlobalIndex(name));
    }

    private void Store(string name) {
        if (TryLocal(name, out int slot)) Emit1(Op.StoreL, slot);
        else Emit1(Op.StoreG, GlobalIndex(name));
    }

    /* ----- emitting ----- */

    private void Emit(Op op) => _code.Add((int)op);
    private void Emit1(Op op, int a) { Emit(op); _code.Add(a); }
    private void Emit2(Op op, int a, int b) { Emit(op); _code.Add(a); _code.Add(b); }
    private void EmitNum(double v) => Emit1(Op.PushNum, NumIndex(v));
    private void EmitStr(string s) => Emit1(Op.PushStr, StrIndex(s));

    private int EmitJump(Op op) {
        Emit(op);
        int pos = _code.Count;
        _code.Add(-1);
        return pos;
    }

    private void Patch(int pos) => _code[pos] = _code.Count;

    /* ----- statements ----- */

    private void CompileBlock(List<Stmt> statements) {
        foreach (var s in statements) CompileStmt(s);
    }

    private void CompileStmt(Stmt s) {
        switch (s) {
            case SayStmt x:
                CompileExpr(x.Value);
                Emit(Op.Print);
                break;

            case VarStmt x: {
                CompileExpr(x.Value);
                if (_scopes.Count > 0) {
                    int slot = DeclareLocal(x.Name);
                    Emit1(Op.StoreL, slot);
                } else Store(x.Name);
                break;
            }

            case AssignStmt x:
                CompileExpr(x.Value);
                Store(x.Name);
                break;

            case CompoundStmt x: {
                Load(x.Name);
                CompileExpr(x.Value);
                Emit(x.Op switch {
                    TokenType.PlusEq => Op.Add,
                    TokenType.MinusEq => Op.Sub,
                    TokenType.StarEq => Op.Mul,
                    TokenType.SlashEq => Op.Div,
                    _ => Op.Add
                });
                Store(x.Name);
                break;
            }

            case IfStmt x: {
                CompileExpr(x.Cond);
                int toElse = EmitJump(Op.JmpF);
                PushScope(); CompileBlock(x.Then); PopScope();
                int toEnd = EmitJump(Op.Jmp);
                Patch(toElse);
                PushScope(); CompileBlock(x.Else); PopScope();
                Patch(toEnd);
                break;
            }

            case WhileStmt x: {
                int start = _code.Count;
                CompileExpr(x.Cond);
                int exit = EmitJump(Op.JmpF);
                var loop = new LoopCtx();
                _loops.Add(loop);
                PushScope(); CompileBlock(x.Body); PopScope();
                foreach (int p in loop.Continues) _code[p] = start;
                Emit1(Op.Jmp, start);
                Patch(exit);
                foreach (int p in loop.Breaks) _code[p] = _code.Count;
                _loops.RemoveAt(_loops.Count - 1);
                break;
            }

            case RepeatStmt x: {
                string counter = "$rep" + (_repCounter++);
                int cidx = GlobalIndex(counter);
                EmitNum(0);
                Emit1(Op.StoreG, cidx);
                int start = _code.Count;
                Emit1(Op.LoadG, cidx);
                CompileExpr(x.Count);
                Emit(Op.Lt);
                int exit = EmitJump(Op.JmpF);
                var loop = new LoopCtx();
                _loops.Add(loop);
                PushScope(); CompileBlock(x.Body); PopScope();
                foreach (int p in loop.Continues) _code[p] = _code.Count;
                Emit1(Op.LoadG, cidx);
                EmitNum(1);
                Emit(Op.Add);
                Emit1(Op.StoreG, cidx);
                Emit1(Op.Jmp, start);
                Patch(exit);
                foreach (int p in loop.Breaks) _code[p] = _code.Count;
                _loops.RemoveAt(_loops.Count - 1);
                break;
            }

            case BreakStmt:
                if (_loops.Count == 0) throw new ParseException("break outside loop");
                _loops[_loops.Count - 1].Breaks.Add(EmitJump(Op.Jmp));
                break;

            case ContinueStmt:
                if (_loops.Count == 0) throw new ParseException("continue outside loop");
                _loops[_loops.Count - 1].Continues.Add(EmitJump(Op.Jmp));
                break;

            case GiveStmt x:
                CompileExpr(x.Value);
                Emit(Op.Ret);
                break;

            case FuncStmt:
                break; // functions are compiled separately

            case ExprStmt x:
                CompileExpr(x.Value);
                Emit(Op.Pop);
                break;

            case BlockStmt x:
                PushScope(); CompileBlock(x.Statements); PopScope();
                break;
        }
    }

    /* ----- expressions ----- */

    private void CompileExpr(Expr e) {
        switch (e) {
            case NumExpr x: EmitNum(x.Value); break;
            case StrExpr x: EmitStr(x.Value); break;
            case VarExpr x: Load(x.Name); break;

            case UnaryExpr x:
                CompileExpr(x.Operand);
                Emit(x.Op == TokenType.Minus ? Op.Neg : Op.Not);
                break;

            case BinaryExpr x:
                CompileExpr(x.Left);
                CompileExpr(x.Right);
                Emit(x.Op switch {
                    TokenType.Plus => Op.Add,
                    TokenType.Minus => Op.Sub,
                    TokenType.Star => Op.Mul,
                    TokenType.Slash => Op.Div,
                    TokenType.Percent => Op.Mod,
                    TokenType.EqualsEquals => Op.Eq,
                    TokenType.NotEquals => Op.Ne,
                    TokenType.Less => Op.Lt,
                    TokenType.LessEqual => Op.Le,
                    TokenType.Greater => Op.Gt,
                    TokenType.GreaterEqual => Op.Ge,
                    TokenType.And => Op.And,
                    TokenType.Or => Op.Or,
                    _ => Op.Nop
                });
                break;

            case CallExpr x:
                foreach (var a in x.Args) CompileExpr(a);
                if (_builtins.TryGetValue(x.Name, out int bid))
                    Emit2(Op.Builtin, bid, x.Args.Count);
                else if (_funcs.TryGetValue(x.Name, out int fid))
                    Emit2(Op.Call, fid, x.Args.Count);
                else
                    throw new ParseException($"unknown function '{x.Name}'");
                break;
        }
    }
}
