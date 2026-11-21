# WornewScript Big Demo Project

`main.wor` demonstrates the language's convenience: input, `$APT` imports of all
16 `.wor` libraries from `../libs/wor/` and C libraries from `../libs/`,
functions, recursion, `for`/`while` loops, math, debugging helpers, and
tkinter-like windows.

## Running

```bash
cd /home/leodog/Projects/WornewScript

# interactive (it will ask for the mode)
./wornew demo_big/main.wor

# console mode without dialog
printf '1\n10\n20\n30\n' | ./wornew demo_big/main.wor

# native binary (direct C)
./wornew compile demo_big/main.wor -o demo_big/app
./demo_big/app < /dev/null   # or with input as above

# window mode (requires X11/XWayland)
printf '2\n' | ./wornew demo_big/main.wor
```

Window mode closes itself after 180 frames (~3 sec) or on ESC / the "Exit"
button — this is intentional so the demo does not hang in automated tests.
