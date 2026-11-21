# WornewScript v0.6 (C++)

Fast interpreted language with compilation to a native binary.
All code is in C++17. `gcc` is only needed for the `compile` command.

Logo: `logo.svg` (hand-crafted blue-green monogram W).

## Build

```bash
cd /home/leodog/Projects/WornewScript
make
make clean
```

Flags: `-O2 -std=c++17 -Wall -Wextra -pedantic` (0 warnings), linked with `-lX11`
(needed for windows; on Windows — `gdi32`/`user32`).

## Prebuilt binaries

Ready-to-run binaries for **Linux x86_64** and **Windows x86_64** are attached to
the [latest Release](../../releases/latest). The Windows build is statically
linked; `gcc` is only needed if you use the `compile` command.

To build the Windows binary yourself with MinGW-w64:

```bash
make windows     # produces wornew.exe
```

## Running

```bash
./wornew program.wor                          # fast bytecode VM
./wornew compile program.wor -o program       # DIRECT compilation to C (maximum speed)
./wornew compile program.wor -o program -b    # bytecode native (for comparison)
./wornew tokens program.wor
```

## Speed (1M summation iterations)

| Mode | Time | Relative to Python |
|------|------|--------------------|
| old C# | ~1800 ms | 18x slower |
| Python 3.14 | ~110 ms | 1x |
| C++ VM v0.6 | **~20 ms** | 5x faster |
| Bytecode native (`-b`) | ~30 ms | 3.5x faster |
| **Direct C (`compile`)** | **~2 ms** | **50x faster** |

Input and GUI are moved into separate translation units: the VM hot loop stays
just as fast as in v0.5.

## What's new in v0.6

### 1. Typed input

```wornew
var num1 = input: int;                 // integer
var f = input("number: "): num;        // number with a prompt
var name = input("name: "): str;       // string
var yes = input("yes/no: "): bool;     // true/false, 1/0, yes/no

// from libs/wor/input.wor:
var age = ask_int("age: ");
```

Types: `int`, `num` (aka `float`, `double`), `str` (aka `string`),
`bool` (aka `boolean`). Works in all three modes.

### 2. Including libraries with `$APT` (idea from CurrtTalk)

```wornew
$APT "libs/wor/math.wor";   // WornewScript library
$APT "libs/math.c";         // real C library (as in CurrtTalk)
```

The file is included as source at parse time; the path is relative to the
`.wor` file. Repeated imports are ignored (cycle protection). The extension
determines the type:

- `.wor` — code is pasted into the AST (regular WornewScript functions/constants);
- `.c`, `.h`, `.cc`, `.cpp`, `.hpp`, `.cxx` — source is pasted into the
  generated C, its functions are available as `c_*`.

In the VM all C functions are already built in, but the import is still needed
for compilation: it is what adds the source to the generated C.

### 3. Windows like tkinter (on top of `minigui.h`)

Immediate-mode API: the window is redrawn every frame inside `while win_open(w)`.

```wornew
var w = window("Example", 420, 260);
while win_open(w) {
    win_begin(w);
    win_clear(w, rgb(30, 30, 46));
    label(w, 20, 20, "Hello!");
    if button(w, 20, 60, 140, 36, "Press") { say "click"; }
    var text = field(w, 20, 110, 200, 34, "text");
    win_end(w);
    sleep(16);
}
win_quit(w);
```

Functions: `window`, `win_open`, `win_begin`, `win_end`, `win_clear`, `win_quit`,
`win_width`, `win_height`, `win_fullscreen`, `win_is_fullscreen`, `rgb`, `rect`,
`panel`, `label`, `label_ex`, `button`, `checkbox`, `field`, `mouse_x`, `mouse_y`,
`mouse_down`, `mouse_pressed`, `key_down`, `key_pressed`, `font`, `font_size`,
`sleep`. Key and color constants are in `libs/wor/gui.wor` and `libs/wor/colors.wor`.
Requires X11 / XWayland (on Linux) or Win32 (on Windows).

### 4. 32 libraries: 16 in WornewScript + 16 in C

#### `libs/wor/` — WornewScript libraries

| File | Provides |
|---|---|
| `io.wor` | `println`, `label_value`, `warn`, `ok` |
| `math.wor` | `m_square`, `m_fact`, `m_gcd`, `m_lcm`, `m_is_prime`, … |
| `string.wor` | `s_repeat`, `s_pad_left/right`, `s_banner` |
| `random.wor` | `r_range`, `r_int`, `r_dice`, `r_sign` |
| `time.wor` | `t_begin`, `t_elapsed`, `t_now` |
| `logic.wor` | `l_between`, `l_xor`, `l_nand`, `l_implies`, `l_choose` |
| `stats.wor` | `st_mean3`, `st_med3`, `st_var3`, `st_pct` |
| `money.wor` | `mo_discount`, `mo_vat`, `mo_tip`, `mo_split` |
| `physics.wor` | `ph_speed`, `ph_force`, `ph_energy`, `ph_fall` |
| `geometry.wor` | areas, perimeters, `geo_dist` |
| `convert.wor` | `cv_c2f`, `cv_km2mi`, `cv_kg2lb`, … |
| `input.wor` | `ask_int/num/str/bool`, `ask_positive_int` |
| `gui.wor` | key constants, `g_close_key`, `g_bg`, `g_card` |
| `colors.wor` | Catppuccin Mocha palette (`C_BLUE`, `C_GREEN`, …) |
| `loops.wor` | `lp_sum`, `lp_product`, `lp_count`, `lp_repeat` |
| `debug.wor` | `dbg_assert`, `dbg_assert_eq`, `dbg_title`, `dbg_sep` |

#### `libs/` — real C libraries

Included with the same `$APT`, but the source is pasted into the generated C.
All functions have the `c_` prefix (in C — `ws_c_*`).

| File | WornewScript functions |
|---|---|
| `col.c` | `c_rgb`, `c_gray` |
| `control.c` | `c_clamp`, `c_between` |
| `cwc.c` | `c_c2f`, `c_f2c` |
| `dlc.c` | `c_sleep`, `c_ticks` |
| `io.c` | `c_sum2`, `c_sign` |
| `keyboard.c` | `c_key_esc`, `c_key_enter` |
| `math.c` | `c_square`, `c_gcd`, `c_fact` |
| `randnum.c` | `c_randint`, `c_randrange` |
| `sysfile.c` | `c_exists`, `c_is_abs` |
| `time.c` | `c_now`, `c_elapsed` |
| `winapi.c` | `c_cls`, `c_title` |
| `string.c` | `c_strlen`, `c_padlen` |
| `stats.c` | `c_mean3`, `c_med3` |
| `money.c` | `c_discount`, `c_vat` |
| `physics.c` | `c_force`, `c_fall` |
| `geometry.c` | `c_circle_area`, `c_dist` |

Example:

```wornew
$APT "libs/math.c";
say c_fact(6) + c_square(9);   // 720 81
```

### 5. Big demo project

`demo_big/main.wor` — console report + GUI panel, using all 16 `.wor` libraries,
4 C libraries, input, recursion (`fib`), nested loops, and windows.

```bash
./wornew demo_big/main.wor                              # asks for mode 1/2
printf '1\n10\n20\n30\n' | ./wornew demo_big/main.wor   # console without dialog
./wornew compile demo_big/main.wor -o demo_big/app
```

## Syntax (reminder)

```wornew
var x = 10;
x += 5;
x %= 3;
say "x = " + x;

if x > 10 { say "greater"; }
elif x > 5 { say "slightly"; }
else { say "less"; }

for (var i = 0; i < 10; i += 1) { say i; }
repeat 3 { say "time"; }

func add(a, b) { give a + b; }
say add(2, 3);
```

Comments: `// ...` and `# ...`. Logical operators: `and/or/not`, `&&/||/!`,
`==`, `!=`, `>=`, `<=`, `%`.

Core builtins: `sqrt`, `abs`, `pow`, `min`, `max`, `len`, `int`, `str`,
`clock`, `rand`, `sin`, `cos`, `tan`, `floor`, `ceil`, `round`,
`log`, `log10`, `exp`, `sign`, `clamp` + the GUI functions above.

## Structure

| File | Purpose |
|---|---|
| `common.h` | tokens, opcode codes, bytecode |
| `lexer.*`, `parser.*` | lexer, AST, `$APT` imports |
| `compiler.*`, `vm.*` | bytecode and fast VM |
| `input_bindings.*` | input (outside the VM hot loop) |
| `gui_bindings.*` | VM GUI bindings |
| `runtime/minigui.h`, `runtime/ws_gui.h` | single header for windows |
| `runtime/gui_embedded.h` | embedding GUI in the generated C |
| `direct_compiler.*` | **AST → direct C** (maximum speed) |
| `native_compiler.*` | C generation + calling gcc `-O3` |
| `native_calls.*` | registry of C functions for the VM |
| `libs/wor/` | 16 standard WornewScript libraries |
| `libs/` | 16 standard C libraries |
| `demo_big/`, `demo_v05.wor`, `test_libs.wor`, `test_c_libs.wor` | examples |
| `main.cpp` | CLI |

Backups: `WornewScript.bak-v06-clibs-20261121.tar.gz` (C libraries),
`WornewScript.bak-v05-20261121-182111.tar.gz` (before v0.6),
`WornewScript.bak-20261121-180325.tar.gz` (before v0.5).
Old C# sources: `legacy_csharp/`.
