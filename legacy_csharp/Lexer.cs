using System;
using System.Collections.Generic;
using System.Text;

public sealed class Lexer {
    private readonly string _src;
    private int _pos;
    private int _line;

    public Lexer(string source) {
        _src = source;
        _pos = 0;
        _line = 1;
    }

    private static bool IsIdStart(char c) => char.IsLetter(c) || c == '_' || c >= '\u0400';
    private static bool IsIdPart(char c) => char.IsLetterOrDigit(c) || c == '_' || c >= '\u0400';

    public List<Token> Tokenize() {
        var tokens = new List<Token>(256);
        int n = _src.Length;

        while (_pos < n) {
            char c = _src[_pos];

            if (c == '\n') { _line++; _pos++; continue; }
            if (c == ' ' || c == '\t' || c == '\r') { _pos++; continue; }

            // comments: // ... and # ...
            if (c == '/' && _pos + 1 < n && _src[_pos + 1] == '/') {
                while (_pos < n && _src[_pos] != '\n') _pos++;
                continue;
            }
            if (c == '#') {
                while (_pos < n && _src[_pos] != '\n') _pos++;
                continue;
            }

            // number
            if (char.IsDigit(c) || (c == '.' && _pos + 1 < n && char.IsDigit(_src[_pos + 1]))) {
                int start = _pos;
                while (_pos < n && char.IsDigit(_src[_pos])) _pos++;
                if (_pos < n && _src[_pos] == '.') {
                    _pos++;
                    while (_pos < n && char.IsDigit(_src[_pos])) _pos++;
                }
                if (_pos < n && (_src[_pos] == 'e' || _src[_pos] == 'E')) {
                    int save = _pos;
                    _pos++;
                    if (_pos < n && (_src[_pos] == '+' || _src[_pos] == '-')) _pos++;
                    if (_pos < n && char.IsDigit(_src[_pos])) {
                        while (_pos < n && char.IsDigit(_src[_pos])) _pos++;
                    } else _pos = save;
                }
                tokens.Add(new Token(TokenType.Number, _src.Substring(start, _pos - start), _line));
                continue;
            }

            // string with escape sequences
            if (c == '"') {
                int line = _line;
                _pos++;
                var sb = new StringBuilder(32);
                while (_pos < n) {
                    char ch = _src[_pos++];
                    if (ch == '"') break;
                    if (ch == '\\' && _pos < n) {
                        char e = _src[_pos++];
                        switch (e) {
                            case 'n': sb.Append('\n'); break;
                            case 't': sb.Append('\t'); break;
                            case 'r': sb.Append('\r'); break;
                            case '0': sb.Append('\0'); break;
                            case '"': sb.Append('"'); break;
                            case '\\': sb.Append('\\'); break;
                            default: sb.Append(e); break;
                        }
                    } else {
                        if (ch == '\n') _line++;
                        sb.Append(ch);
                    }
                }
                tokens.Add(new Token(TokenType.String, sb.ToString(), line));
                continue;
            }

            // identifier / keyword
            if (IsIdStart(c)) {
                int start = _pos;
                while (_pos < n && IsIdPart(_src[_pos])) _pos++;
                string word = _src.Substring(start, _pos - start);
                tokens.Add(new Token(Keyword(word), word, _line));
                continue;
            }

            int ln = _line;
            char next = _pos + 1 < n ? _src[_pos + 1] : '\0';

            switch (c) {
                case '+':
                    if (next == '=') { Add(tokens, TokenType.PlusEq, "+=", ln, 2); }
                    else Add(tokens, TokenType.Plus, "+", ln, 1);
                    continue;
                case '-':
                    if (next == '=') { Add(tokens, TokenType.MinusEq, "-=", ln, 2); }
                    else Add(tokens, TokenType.Minus, "-", ln, 1);
                    continue;
                case '*':
                    if (next == '=') { Add(tokens, TokenType.StarEq, "*=", ln, 2); }
                    else Add(tokens, TokenType.Star, "*", ln, 1);
                    continue;
                case '/':
                    if (next == '=') { Add(tokens, TokenType.SlashEq, "/=", ln, 2); }
                    else Add(tokens, TokenType.Slash, "/", ln, 1);
                    continue;
                case '%': Add(tokens, TokenType.Percent, "%", ln, 1); continue;
                case '=':
                    if (next == '=') { Add(tokens, TokenType.EqualsEquals, "==", ln, 2); }
                    else Add(tokens, TokenType.Equals, "=", ln, 1);
                    continue;
                case '!':
                    if (next == '=') { Add(tokens, TokenType.NotEquals, "!=", ln, 2); }
                    else Add(tokens, TokenType.Bang, "!", ln, 1);
                    continue;
                case '>':
                    if (next == '=') { Add(tokens, TokenType.GreaterEqual, ">=", ln, 2); }
                    else Add(tokens, TokenType.Greater, ">", ln, 1);
                    continue;
                case '<':
                    if (next == '=') { Add(tokens, TokenType.LessEqual, "<=", ln, 2); }
                    else Add(tokens, TokenType.Less, "<", ln, 1);
                    continue;
                case '&':
                    if (next == '&') { Add(tokens, TokenType.AndAnd, "&&", ln, 2); }
                    continue;
                case '|':
                    if (next == '|') { Add(tokens, TokenType.OrOr, "||", ln, 2); }
                    continue;
                case ';': Add(tokens, TokenType.Semi, ";", ln, 1); continue;
                case ',': Add(tokens, TokenType.Comma, ",", ln, 1); continue;
                case '(': Add(tokens, TokenType.LParen, "(", ln, 1); continue;
                case ')': Add(tokens, TokenType.RParen, ")", ln, 1); continue;
                case '{': Add(tokens, TokenType.LBrace, "{", ln, 1); continue;
                case '}': Add(tokens, TokenType.RBrace, "}", ln, 1); continue;
            }

            _pos++; // unknown character is silently skipped (as in the original)
        }

        tokens.Add(new Token(TokenType.EOF, "", _line));
        return tokens;
    }

    private void Add(List<Token> list, TokenType type, string text, int line, int len) {
        list.Add(new Token(type, text, line));
        _pos += len;
    }

    private static TokenType Keyword(string w) => w switch {
        "say" => TokenType.Say,
        "var" => TokenType.Var,
        "if" => TokenType.If,
        "else" => TokenType.Else,
        "while" => TokenType.While,
        "repeat" => TokenType.Repeat,
        "func" => TokenType.Func,
        "give" => TokenType.Give,
        "use" => TokenType.Use,
        "break" => TokenType.Break,
        "continue" => TokenType.Continue,
        "true" => TokenType.True,
        "false" => TokenType.False,
        "and" => TokenType.And,
        "or" => TokenType.Or,
        "not" => TokenType.Not,
        _ => TokenType.Identifier
    };
}
