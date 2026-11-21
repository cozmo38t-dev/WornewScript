using System;
using System.Collections.Generic;
using System.Globalization;

public readonly struct Value {
    public readonly double Num;
    public readonly string? Str;
    public readonly bool IsStr;

    private Value(double n, string? s, bool str) { Num = n; Str = s; IsStr = str; }
    public static Value Number(double n) => new Value(n, null, false);
    public static Value Text(string s) => new Value(0, s, true);

    public bool Truthy => IsStr ? !string.IsNullOrEmpty(Str) : Num != 0.0;

    public string AsText() => IsStr ? (Str ?? "") : Num.ToString("R", CultureInfo.InvariantCulture);
    public double AsNum() => IsStr ? (double.TryParse(Str, NumberStyles.Any, CultureInfo.InvariantCulture, out double v) ? v : 0) : Num;

    public override string ToString() => AsText();
}

/* Bytecode VM: data is always double/string, expressions are compiled once. */
public sealed class Vm {
    private sealed class Frame {
        public int[] Code = Array.Empty<int>();
        public int Ip;
        public Value[] Locals = Array.Empty<Value>();
    }

    private readonly BytecodeProgram _p;
    private readonly Value[] _globals;
    private readonly Value[] _stack = new Value[8192];
    private readonly List<Frame> _frames = new List<Frame>();
    private readonly Random _rnd = new Random();
    private readonly long _startMs = Environment.TickCount64;

    public Vm(BytecodeProgram p) {
        _p = p;
        _globals = new Value[p.Names.Count];
    }

    public void Run() {
        int[] code = _p.MainCode.ToArray();
        int ip = 0;
        Value[] locals = Array.Empty<Value>();
        int sp = 0;

        while (true) {
            Op op = (Op)code[ip++];
            switch (op) {
                case Op.Nop: break;

                case Op.PushNum: _stack[sp++] = Value.Number(_p.Numbers[code[ip++]]); break;
                case Op.PushStr: _stack[sp++] = Value.Text(_p.Strings[code[ip++]]); break;

                case Op.LoadG: _stack[sp++] = _globals[code[ip++]]; break;
                case Op.StoreG: _globals[code[ip++]] = _stack[--sp]; break;
                case Op.LoadL: _stack[sp++] = locals[code[ip++]]; break;
                case Op.StoreL: locals[code[ip++]] = _stack[--sp]; break;

                case Op.Add: {
                    Value b = _stack[--sp], a = _stack[--sp];
                    _stack[sp++] = (a.IsStr || b.IsStr)
                        ? Value.Text(a.AsText() + b.AsText())
                        : Value.Number(a.Num + b.Num);
                    break;
                }
                case Op.Sub: { double b = _stack[--sp].AsNum(), a = _stack[--sp].AsNum(); _stack[sp++] = Value.Number(a - b); break; }
                case Op.Mul: { double b = _stack[--sp].AsNum(), a = _stack[--sp].AsNum(); _stack[sp++] = Value.Number(a * b); break; }
                case Op.Div: { double b = _stack[--sp].AsNum(), a = _stack[--sp].AsNum(); _stack[sp++] = Value.Number(b == 0 ? 0 : a / b); break; }
                case Op.Mod: { double b = _stack[--sp].AsNum(), a = _stack[--sp].AsNum(); _stack[sp++] = Value.Number(b == 0 ? 0 : a % b); break; }
                case Op.Neg: _stack[sp - 1] = Value.Number(-_stack[sp - 1].AsNum()); break;
                case Op.Not: _stack[sp - 1] = Value.Number(_stack[sp - 1].Truthy ? 0 : 1); break;

                case Op.Eq: case Op.Ne: {
                    Value b = _stack[--sp], a = _stack[--sp];
                    bool eq = (a.IsStr || b.IsStr) ? a.AsText() == b.AsText() : a.Num == b.Num;
                    _stack[sp++] = Value.Number(op == Op.Eq ? (eq ? 1 : 0) : (eq ? 0 : 1));
                    break;
                }
                case Op.Lt: case Op.Le: case Op.Gt: case Op.Ge: {
                    Value b = _stack[--sp], a = _stack[--sp];
                    bool r = (a.IsStr || b.IsStr)
                        ? string.CompareOrdinal(a.AsText(), b.AsText()) < 0
                        : a.Num < b.Num;
                    if (op == Op.Le) r = (a.IsStr || b.IsStr) ? string.CompareOrdinal(a.AsText(), b.AsText()) <= 0 : a.Num <= b.Num;
                    if (op == Op.Gt) r = (a.IsStr || b.IsStr) ? string.CompareOrdinal(a.AsText(), b.AsText()) > 0 : a.Num > b.Num;
                    if (op == Op.Ge) r = (a.IsStr || b.IsStr) ? string.CompareOrdinal(a.AsText(), b.AsText()) >= 0 : a.Num >= b.Num;
                    _stack[sp++] = Value.Number(r ? 1 : 0);
                    break;
                }
                case Op.And: { Value b = _stack[--sp], a = _stack[--sp]; _stack[sp++] = Value.Number(a.Truthy && b.Truthy ? 1 : 0); break; }
                case Op.Or: { Value b = _stack[--sp], a = _stack[--sp]; _stack[sp++] = Value.Number(a.Truthy || b.Truthy ? 1 : 0); break; }

                case Op.Jmp: ip = code[ip]; break;
                case Op.JmpF: { int t = code[ip++]; if (!_stack[--sp].Truthy) ip = t; break; }
                case Op.JmpT: { int t = code[ip++]; if (_stack[--sp].Truthy) ip = t; break; }

                case Op.Print: Console.WriteLine(_stack[--sp].AsText()); break;
                case Op.Pop: --sp; break;

                case Op.Call: {
                    int fid = code[ip++], argc = code[ip++];
                    Function f = _p.Functions[fid];
                    _frames.Add(new Frame { Code = code, Ip = ip, Locals = locals });
                    var args = new Value[f.LocalCount];
                    for (int i = argc - 1; i >= 0; --i) args[i] = _stack[--sp];
                    code = f.Code.ToArray();
                    ip = 0;
                    locals = args;
                    break;
                }
                case Op.Ret: {
                    if (_frames.Count == 0) return;
                    Frame fr = _frames[_frames.Count - 1];
                    _frames.RemoveAt(_frames.Count - 1);
                    code = fr.Code;
                    ip = fr.Ip;
                    locals = fr.Locals;
                    break;
                }
                case Op.Builtin: {
                    int bid = code[ip++], argc = code[ip++];
                    var a = new Value[argc];
                    for (int i = argc - 1; i >= 0; --i) a[i] = _stack[--sp];
                    _stack[sp++] = CallBuiltin(bid, a);
                    break;
                }
                case Op.Halt:
                    return;
            }
        }
    }

    private Value CallBuiltin(int id, Value[] a) {
        double N(int i) => i < a.Length ? a[i].AsNum() : 0;
        switch (Compiler.Builtins[id]) {
            case "sqrt": return Value.Number(Math.Sqrt(N(0)));
            case "abs": return Value.Number(Math.Abs(N(0)));
            case "pow": return Value.Number(Math.Pow(N(0), N(1)));
            case "min": return Value.Number(Math.Min(N(0), N(1)));
            case "max": return Value.Number(Math.Max(N(0), N(1)));
            case "int": return Value.Number(Math.Truncate(N(0)));
            case "len": return Value.Number(a.Length > 0 && a[0].IsStr ? (a[0].Str ?? "").Length : Math.Abs(N(0)).ToString(CultureInfo.InvariantCulture).Length);
            case "str": return Value.Text(a.Length > 0 ? a[0].AsText() : "");
            case "clock": return Value.Number(Environment.TickCount64 - _startMs);
            case "rand": return Value.Number(_rnd.NextDouble());
            default: return Value.Number(0);
        }
    }
}
