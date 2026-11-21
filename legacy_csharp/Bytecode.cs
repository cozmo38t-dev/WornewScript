using System.Collections.Generic;

/* Opcode values — must match the enum in the C runtime (NativeCompiler). */
public enum Op {
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
    Builtin = 29, Pop = 30
}

public sealed class Function {
    public string Name = "";
    public int Arity;
    public int LocalCount;
    public List<int> Code = new List<int>();
}

public sealed class BytecodeProgram {
    public List<int> MainCode = new List<int>();
    public List<Function> Functions = new List<Function>();
    public List<double> Numbers = new List<double>();
    public List<string> Strings = new List<string>();
    public List<string> Names = new List<string>();
}
