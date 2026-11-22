# WIDE - Wornew IDE

Small IDLE-like editor for WornewScript, styled like the project website
(Catppuccin Mocha / Lavender). Written in Python with `tkinter`, so it uses only
the standard library.

## Files

| File | Purpose |
|---|---|
| `wide.py` | shared IDE source (all platforms) |
| `wide` | Linux launcher: `./wide file.wor` |
| `wide.cmd` | Windows launcher: `wide file.wor` |
| `wide.desktop` | ready-made desktop entry for Linux |
| `wide_config.json` | created automatically on first run (interpreter, folder, window size) |

## Requirements

- Python 3.10+ with `tkinter` (included in the official Python installers).
- The `wornew` interpreter. WIDE looks next to itself, in the parent folder
  (the repository root), and then in `PATH`. You can also set it manually with
  the **Interpreter** button.

## Shortcuts

| Key | Action |
|---|---|
| `Ctrl+O` | Open file |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save as |
| `Ctrl+W` | Close tab |
| `F5` | Run (VM) |
| `F6` | Build (direct C) |
