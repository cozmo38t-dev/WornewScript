using System;
using System.Diagnostics;
using System.IO;

if (args.Length == 0) {
    Console.WriteLine("WornewScript v0.2");
    Console.WriteLine();
    Console.WriteLine("Usage:");
    Console.WriteLine("  dotnet run -- <file.wor>                    run (fast VM)");
    Console.WriteLine("  dotnet run -- run <file.wor>                same thing");
    Console.WriteLine("  dotnet run -- compile <file.wor> [-o name]  build a native binary");
    Console.WriteLine("  dotnet run -- tokens <file.wor>             show tokens");
    return 0;
}

string cmd = args[0];
string path;

if (cmd is "run" or "compile" or "tokens") {
    if (args.Length < 2) {
        Console.Error.WriteLine($"Error: '{cmd}' requires a .wor file");
        return 1;
    }
    path = args[1];
} else {
    path = cmd;
    cmd = "run";
}

if (!File.Exists(path)) {
    Console.Error.WriteLine($"Error: file not found: {path}");
    return 1;
}

try {
    string source = File.ReadAllText(path);
    var tokens = new Lexer(source).Tokenize();

    if (cmd == "tokens") {
        Console.WriteLine("TOKENS:");
        foreach (var t in tokens) Console.WriteLine("  " + t);
        return 0;
    }

    var ast = new Parser(tokens).ParseProgram();
    var program = new Compiler().Compile(ast);

    if (cmd == "compile") {
        string output = Path.GetFileNameWithoutExtension(path);
        for (int i = 2; i < args.Length - 1; i++) {
            if (args[i] == "-o") output = args[i + 1];
        }
        var sw = Stopwatch.StartNew();
        bool ok = NativeCompiler.Build(program, output, out string message);
        sw.Stop();
        if (!ok) {
            Console.Error.WriteLine("C compilation error: " + message);
            return 1;
        }
        Console.WriteLine($"Done: {output} ({sw.Elapsed.TotalMilliseconds:F1} ms)");
        return 0;
    }

    var timer = Stopwatch.StartNew();
    new Vm(program).Run();
    timer.Stop();
    Console.WriteLine($"\nTime: {timer.Elapsed.TotalMilliseconds:F3} ms");
    return 0;
} catch (Exception e) {
    Console.Error.WriteLine("Error: " + e.Message);
    return 1;
}
