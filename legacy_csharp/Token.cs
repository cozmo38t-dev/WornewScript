using System;

public enum TokenType {
    // keywords
    Say, Var, If, Else, While, Func, Give, Use,
    Break, Continue, Repeat, True, False, And, Or, Not,

    // literals
    String, Number, Identifier,

    // operators
    Plus, Minus, Star, Slash, Percent,
    Equals, EqualsEquals, NotEquals,
    Greater, Less, GreaterEqual, LessEqual,
    AndAnd, OrOr, Bang,
    PlusEq, MinusEq, StarEq, SlashEq,

    // punctuation
    Semi, Comma, LParen, RParen, LBrace, RBrace,
    EOF
}

public readonly struct Token {
    public readonly TokenType Type;
    public readonly string Value;
    public readonly int Line;

    public Token(TokenType type, string value, int line) {
        Type = type;
        Value = value;
        Line = line;
    }

    public override string ToString() => $"{Type}({Value})";
}
