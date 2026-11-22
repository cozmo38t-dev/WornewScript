#!/usr/bin/env python3
"""WIDE - a small IDE for WornewScript.

Cross-platform, built only with the Python standard library (tkinter).
Run:  wide file.wor    /    wide.cmd file.wor
"""

from __future__ import annotations

import json
import os
import queue
import re
import shlex
import shutil
import subprocess
import sys
import threading
from pathlib import Path

import tkinter as tk
import tkinter.font as tkfont
from tkinter import filedialog, messagebox, ttk

APP_NAME = "WIDE"
APP_VERSION = "0.1.0"
BASE_DIR = Path(__file__).resolve().parent
CONFIG_PATH = BASE_DIR / "wide_config.json"

# Catppuccin Mocha / Lavender, same palette as the website.
CRUST = "#11111b"
MANTLE = "#181825"
BASE = "#1e1e2e"
SURFACE = "#313244"
SURFACE2 = "#45475a"
TEXT = "#cdd6f4"
MUTED = "#a6adc8"
LAVENDER = "#cba6f7"
BLUE = "#89b4fa"
TEAL = "#94e2d5"
PINK = "#f5c2e7"
GREEN = "#a6e3a1"
YELLOW = "#f9e2af"
RED = "#f38ba8"
COMMENT = "#6c7086"

EDITOR_FONT = ("Consolas" if os.name == "nt" else "DejaVu Sans Mono", 11)
UI_FONT = ("Segoe UI" if os.name == "nt" else "DejaVu Sans", 10)

KEYWORDS = (
    "var func give if else elif while for repeat break continue "
    "and or not input say"
).split()
TYPES = "int num float double str string bool boolean".split()

HIGHLIGHT_RE = re.compile(
    r"(?P<comment>//[^\n]*)"
    r"|(?P<string>\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*')"
    r"|(?P<apt>\$APT)"
    r"|(?P<keyword>\b(?:" + "|".join(KEYWORDS) + r")\b)"
    r"|(?P<type>\b(?:" + "|".join(TYPES) + r")\b)"
    r"|(?P<number>\b\d+(?:\.\d+)?\b)"
    r"|(?P<func>\b[A-Za-z_]\w*(?=\s*\())",
    re.MULTILINE,
)

TAG_COLORS = {
    "comment": COMMENT,
    "string": GREEN,
    "apt": PINK,
    "keyword": LAVENDER,
    "type": TEAL,
    "number": YELLOW,
    "func": BLUE,
}

STARTER = """// New WornewScript file
var name = input("Name: "): str;
say "Hello, " + name + "!";
"""


def mix(color_a: str, color_b: str, factor: float) -> str:
    """Linear blend of two #rrggbb colors."""
    a = tuple(int(color_a[i:i + 2], 16) for i in (1, 3, 5))
    b = tuple(int(color_b[i:i + 2], 16) for i in (1, 3, 5))
    return "#" + "".join(
        f"{round(a[i] + (b[i] - a[i]) * factor):02x}" for i in range(3)
    )


def round_rect(canvas: tk.Canvas, x1, y1, x2, y2, radius=10, **kwargs) -> int:
    r = min(radius, (x2 - x1) / 2, (y2 - y1) / 2)
    points = [
        x1 + r, y1, x2 - r, y1, x2, y1, x2, y1 + r,
        x2, y2 - r, x2, y2, x2 - r, y2, x1 + r, y2,
        x1, y2, x1, y2 - r, x1, y1 + r, x1, y1,
    ]
    return canvas.create_polygon(points, smooth=True, **kwargs)


class RoundButton(tk.Canvas):
    """Flat rounded button drawn on a Canvas - no legacy bevels."""

    def __init__(self, master, text, command=None, accent=False,
                 width=None, height=34, bg=BASE):
        self._font = tkfont.Font(family=UI_FONT[0], size=UI_FONT[1], weight="bold")
        req_width = width or max(56, self._font.measure(text) + 30)
        self._bg = bg
        super().__init__(
            master, width=req_width, height=height,
            bg=bg, highlightthickness=0, bd=0, cursor="hand2"
        )
        self._text = text
        self._command = command
        self._accent = accent
        self._req_width = req_width
        self._req_height = height
        self._hover = False
        self._disabled = False

        self.bind("<Enter>", self._on_enter)
        self.bind("<Leave>", self._on_leave)
        self.bind("<ButtonPress-1>", self._on_press)
        self.bind("<ButtonRelease-1>", self._on_release)
        self.bind("<Configure>", lambda _e: self._draw())
        self._draw()

    def _on_enter(self, _event=None):
        self._hover = True
        self._draw()

    def _on_leave(self, _event=None):
        self._hover = False
        self._draw()

    def _on_press(self, _event=None):
        if not self._disabled:
            self.configure(bg=mix(self._bg, CRUST, 0.5))

    def _on_release(self, _event=None):
        self.configure(bg=self._bg)
        if not self._disabled and self._command:
            self._command()

    def set_disabled(self, disabled: bool) -> None:
        self._disabled = bool(disabled)
        self.configure(cursor="arrow" if self._disabled else "hand2")
        self._draw()

    def _draw(self) -> None:
        w = self.winfo_width() or self._req_width
        h = self.winfo_height() or self._req_height
        self.delete("all")

        if self._disabled:
            fill, fg, outline = SURFACE, MUTED, ""
        elif self._accent:
            fill = BLUE if self._hover else LAVENDER
            fg, outline = MANTLE, ""
        else:
            fill = SURFACE2 if self._hover else SURFACE
            fg = TEXT
            outline = LAVENDER if self._hover else ""

        round_rect(self, 2, 3, w - 1, h - 1, 10,
                   fill=mix(self._bg, CRUST, 0.5), outline="")
        round_rect(
            self, 1, 1, w - 3, h - 3, 10,
            fill=fill, outline=outline, width=1 if outline else 0,
        )
        self.create_text(w / 2, h / 2, text=self._text, font=self._font, fill=fg)


class RoundEntry(tk.Canvas):
    """Rounded input field that behaves like a normal tk.Entry."""

    def __init__(self, master, textvariable=None, width=320, height=34, bg=BASE):
        self._bg = bg
        super().__init__(
            master, width=width, height=height,
            bg=bg, highlightthickness=0, bd=0
        )
        self._req_width = width
        self._req_height = height
        self._entry = tk.Entry(
            self,
            textvariable=textvariable,
            bg=SURFACE2,
            fg=TEXT,
            insertbackground=LAVENDER,
            selectbackground=SURFACE2,
            selectforeground=TEXT,
            relief="flat",
            bd=0,
            highlightthickness=0,
            font=UI_FONT,
            disabledbackground=SURFACE,
            disabledforeground=MUTED,
        )
        self._window = self.create_window(width / 2, height / 2, window=self._entry)
        self._focused = False
        self._entry.bind("<FocusIn>", lambda _e: self._set_focus(True))
        self._entry.bind("<FocusOut>", lambda _e: self._set_focus(False))
        tk.Canvas.bind(self, "<Configure>", lambda _e: self._draw())
        self._draw()

    def _set_focus(self, focused: bool) -> None:
        self._focused = focused
        self._draw()

    def _draw(self) -> None:
        w = self.winfo_width() or self._req_width
        h = self.winfo_height() or self._req_height
        self.delete("shape")
        round_rect(
            self, 1, 1, w - 2, h - 2, 10,
            fill=SURFACE2,
            outline=BLUE if self._focused else SURFACE2,
            width=1,
            tags=("shape",),
        )
        self.tag_lower("shape")
        self.coords(self._window, w / 2, h / 2)
        self.itemconfigure(self._window, width=max(20, w - 26))

    def bind(self, sequence=None, func=None, add=None):
        return self._entry.bind(sequence, func, add)

    def focus_set(self) -> None:
        self._entry.focus_set()

    def set_state(self, state: str) -> None:
        self._entry.configure(state=state)
        self._draw()


class ModernNotebook(tk.Frame):
    """Flat VS Code-like tab bar drawn with plain tk widgets."""

    def __init__(self, master, on_close=None):
        super().__init__(master, bg=BASE)
        self._tabbar = tk.Frame(self, bg=MANTLE, height=40)
        self._tabbar.pack(side="top", fill="x")
        self._underline = tk.Frame(self._tabbar, bg=LAVENDER, height=2)
        tk.Frame(self, bg=SURFACE, height=1).pack(side="top", fill="x")
        self._container = tk.Frame(self, bg=BASE)
        self._container.pack(fill="both", expand=True)
        self._tabs: list[dict] = []
        self._current = None
        self._on_close = on_close

    def add(self, frame, text: str = "") -> None:
        item = {
            "frame": frame,
            "active": False,
            "hover": False,
        }
        item["label"] = tk.Label(
            self._tabbar, text=text, bg=MANTLE, fg=MUTED,
            font=(UI_FONT[0], 9, "bold"), padx=14, pady=8,
            bd=0, cursor="hand2",
        )
        item["close"] = tk.Label(
            self._tabbar, text="×", bg=MANTLE, fg=MUTED,
            font=(UI_FONT[0], 12), padx=8, pady=8,
            bd=0, cursor="hand2",
        )
        item["label"].bind("<Button-1>", lambda _e, f=frame: self.select(f))
        item["close"].bind("<Button-1>", lambda _e, f=frame: self._request_close(f))
        for widget in (item["label"], item["close"]):
            widget.bind("<Enter>", lambda _e, i=item: self._hover(i, True))
            widget.bind("<Leave>", lambda _e, i=item: self._hover(i, False))
        item["label"].pack(side="left", padx=(6, 0))
        item["close"].pack(side="left", padx=(0, 6))
        self._tabs.append(item)
        self.select(frame)

    def _hover(self, item: dict, hovering: bool) -> None:
        item["hover"] = hovering
        self._refresh_item(item)

    def _refresh_item(self, item: dict) -> None:
        if item["active"]:
            bg, fg, close_fg = BASE, LAVENDER, PINK
        elif item["hover"]:
            bg, fg, close_fg = SURFACE, TEXT, PINK
        else:
            bg, fg, close_fg = MANTLE, MUTED, MUTED
        item["label"].configure(bg=bg, fg=fg)
        item["close"].configure(bg=bg, fg=close_fg)

    def _request_close(self, frame) -> None:
        self.select(frame)
        if self._on_close:
            self._on_close()

    def select(self, frame=None):
        if frame is None:
            return str(self._current) if self._current is not None else ""
        target = str(frame)
        found = None
        for item in self._tabs:
            item["active"] = str(item["frame"]) == target
            item["frame"].pack_forget()
            self._refresh_item(item)
            if item["active"]:
                found = item
        if found is None:
            return ""
        found["frame"].pack(in_=self._container, fill="both", expand=True)
        self._current = found["frame"]
        self.after(1, lambda: self._place_underline(found))
        self.event_generate("<<NotebookTabChanged>>")
        return str(self._current)

    def _place_underline(self, item: dict) -> None:
        try:
            self._underline.place(
                x=item["label"].winfo_x() + 6,
                y=38,
                width=item["label"].winfo_width() + item["close"].winfo_width(),
                height=2,
            )
        except tk.TclError:
            pass

    def index(self, frame) -> int:
        for i, item in enumerate(self._tabs):
            if str(item["frame"]) == str(frame):
                return i
        return -1

    def forget(self, index: int) -> None:
        if not 0 <= index < len(self._tabs):
            return
        item = self._tabs.pop(index)
        was_current = str(item["frame"]) == str(self._current)
        item["frame"].pack_forget()
        item["label"].destroy()
        item["close"].destroy()
        if was_current:
            self._current = None
            self._underline.place_forget()
            if self._tabs:
                self.select(self._tabs[0]["frame"])
            else:
                self.event_generate("<<NotebookTabChanged>>")

    def tab(self, index: int, text: str | None = None):
        if not 0 <= index < len(self._tabs):
            return None
        if text is not None:
            self._tabs[index]["label"].configure(text=text)
        return self._tabs[index]["label"].cget("text")


def load_config() -> dict:
    try:
        data = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
        return data if isinstance(data, dict) else {}
    except (OSError, ValueError):
        return {}


def save_config(data: dict) -> None:
    try:
        CONFIG_PATH.write_text(
            json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8"
        )
    except OSError:
        pass


def read_text_file(path: Path) -> str:
    for enc in ("utf-8-sig", "utf-8", "cp1251"):
        try:
            return path.read_text(encoding=enc)
        except UnicodeDecodeError:
            continue
    return path.read_text(encoding="utf-8", errors="replace")


def find_interpreter(config: dict) -> str | None:
    candidates: list[str | Path] = []
    if config.get("interpreter"):
        candidates.append(config["interpreter"])
    for name in ("wornew", "wornew.exe"):
        candidates.append(BASE_DIR / name)
        candidates.append(BASE_DIR.parent / name)
        found = shutil.which(name)
        if found:
            candidates.append(found)
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    return None


class EditorTab:
    """One open file: text widget + line-number gutter + highlighting."""

    def __init__(self, app: "Wide", path: Path | None, text: str):
        self.app = app
        self.path = path
        self.modified = False
        self._highlight_after: str | None = None

        self.frame = ttk.Frame(app.notebook)

        self.gutter = tk.Text(
            self.frame,
            width=4,
            height=1,
            wrap="none",
            bg=MANTLE,
            fg=COMMENT,
            font=EDITOR_FONT,
            relief="flat",
            borderwidth=0,
            padx=6,
            pady=6,
            state="disabled",
        )
        self.text = tk.Text(
            self.frame,
            wrap="none",
            bg=BASE,
            fg=TEXT,
            insertbackground=LAVENDER,
            selectbackground=SURFACE2,
            selectforeground=TEXT,
            font=EDITOR_FONT,
            relief="flat",
            borderwidth=0,
            padx=10,
            pady=6,
            undo=True,
            autoseparators=True,
        )
        self.scrollbar = ttk.Scrollbar(
            self.frame, orient="vertical", command=self._scroll_both
        )
        self.text.configure(yscrollcommand=self._text_scrolled)

        self.gutter.pack(side="left", fill="y")
        self.text.pack(side="left", fill="both", expand=True)
        self.scrollbar.pack(side="right", fill="y")

        for tag, color in TAG_COLORS.items():
            self.text.tag_configure(tag, foreground=color)

        self.text.insert("1.0", text)
        self.text.edit_modified(False)
        self._install_bindings()
        self.refresh_gutter()
        self.highlight()

    def _install_bindings(self) -> None:
        self.text.bind("<<Modified>>", self._on_modified)
        self.text.bind("<Return>", self._on_return)
        self.text.bind("<Tab>", self._on_tab)
        self.text.bind("<KeyRelease>", self._on_cursor_moved)
        self.text.bind("<ButtonRelease-1>", self._on_cursor_moved)

    def _text_scrolled(self, first: str, last: str) -> None:
        try:
            self.gutter.yview_moveto(float(first))
        except (ValueError, tk.TclError):
            pass
        self.scrollbar.set(first, last)

    def _scroll_both(self, *args) -> None:
        self.text.yview(*args)
        self.gutter.yview(*args)

    def _on_modified(self, _event=None) -> None:
        if self.text.edit_modified():
            self.modified = True
            self.app.update_tab_title(self)
            self._schedule_highlight()
            self.app.update_status()
        self.text.edit_modified(False)

    def _schedule_highlight(self) -> None:
        if self._highlight_after is not None:
            try:
                self.app.after_cancel(self._highlight_after)
            except tk.TclError:
                pass
        self._highlight_after = self.app.after(120, self.highlight)

    def _on_return(self, _event=None) -> str:
        index = self.text.index("insert")
        line = self.text.get(index + " linestart", index)
        indent = re.match(r"[ \t]*", line).group(0)
        if line.rstrip().endswith("{"):
            indent += "    "
        if indent:
            self.text.insert("insert", "\n" + indent)
        else:
            self.text.insert("insert", "\n")
        return "break"

    def _on_tab(self, _event=None) -> str:
        self.text.insert("insert", "    ")
        return "break"

    def _on_cursor_moved(self, _event=None) -> None:
        self.app.update_status()

    def refresh_gutter(self) -> None:
        count = int(self.text.index("end-1c").split(".")[0])
        self.gutter.configure(width=max(4, len(str(count)) + 1))
        numbers = "\n".join(str(i) for i in range(1, count + 1))
        self.gutter.configure(state="normal")
        self.gutter.delete("1.0", "end")
        self.gutter.insert("1.0", numbers)
        self.gutter.configure(state="disabled")

    def highlight(self) -> None:
        keep = {"sel"}
        for tag in list(self.text.tag_names()):
            if tag not in keep:
                self.text.tag_remove(tag, "1.0", "end")
        content = self.text.get("1.0", "end-1c")
        for match in HIGHLIGHT_RE.finditer(content):
            tag = match.lastgroup
            if tag:
                start = f"1.0+{match.start()}c"
                end = f"1.0+{match.end()}c"
                self.text.tag_add(tag, start, end)
        self.refresh_gutter()

    def cursor_info(self) -> tuple[int, int]:
        line, col = self.text.index("insert").split(".")[:2]
        return int(line), int(col) + 1

    def display_name(self) -> str:
        base = self.path.name if self.path else "Untitled"
        return base + (" *" if self.modified else "")


class Wide(tk.Tk):
    def __init__(self, files: list[str] | None = None):
        super().__init__()
        self.config_data = load_config()
        self.interpreter = find_interpreter(self.config_data)
        self.tabs: list[EditorTab] = []
        self.notebook: ModernNotebook
        self.explorer_dir = Path(
            self.config_data.get("last_dir") or Path.cwd()
        )

        self.queue: queue.Queue = queue.Queue()
        self.proc: subprocess.Popen | None = None
        self._polling = False

        self.title(f"{APP_NAME} - WornewScript IDE")
        geometry = self.config_data.get("geometry", "1280x820")
        self.geometry(geometry)
        self.minsize(880, 560)

        self._build_style()
        self._build_ui()
        self._bind_keys()

        for file in files or []:
            self.open_file(file)
        if not self.tabs:
            self.new_file()

        self.update_status()

    # ---------- UI ----------

    def _build_style(self) -> None:
        # ttk is kept only for the tree, scrollbars and the paned divider.
        # Every other widget is drawn with flat tk widgets in the site palette.
        style = ttk.Style(self)
        style.theme_use("clam")

        style.configure("TFrame", background=BASE)
        style.configure(
            "Treeview",
            background=MANTLE,
            fieldbackground=MANTLE,
            foreground=TEXT,
            rowheight=26,
            borderwidth=0,
            font=UI_FONT,
        )
        style.map(
            "Treeview",
            background=[("selected", SURFACE2)],
            foreground=[("selected", LAVENDER)],
        )
        style.configure(
            "Treeview.Heading",
            background=SURFACE,
            foreground=LAVENDER,
            relief="flat",
            borderwidth=0,
            font=UI_FONT,
        )
        style.configure(
            "TScrollbar",
            background=SURFACE,
            troughcolor=MANTLE,
            borderwidth=0,
            relief="flat",
            arrowcolor=MUTED,
        )
        style.configure("TPanedwindow", background=BASE)

    def _build_header(self) -> None:
        header = tk.Frame(self, bg=CRUST, height=52)
        header.pack(side="top", fill="x")

        logo = tk.Canvas(header, width=34, height=34, bg=CRUST,
                         highlightthickness=0, bd=0)
        logo.pack(side="left", padx=(14, 8), pady=8)
        round_rect(logo, 1, 1, 33, 33, 10, fill=LAVENDER)
        logo.create_text(17, 17, text="W", fill=MANTLE,
                         font=(UI_FONT[0], 15, "bold"))

        brand = tk.Frame(header, bg=CRUST)
        brand.pack(side="left")
        tk.Label(brand, text="WIDE", bg=CRUST, fg=TEXT,
                 font=(UI_FONT[0], 14, "bold")).pack(anchor="w")
        tk.Label(brand, text="WORNEW IDE", bg=CRUST, fg=COMMENT,
                 font=(UI_FONT[0], 8, "bold")).pack(anchor="w")

        tk.Frame(header, bg=SURFACE, width=1, height=28).pack(
            side="left", padx=(18, 12), pady=12
        )

        def button(label: str, command, accent: bool = False, width=None):
            b = RoundButton(
                header, label, command=command, accent=accent,
                width=width, height=32, bg=CRUST,
            )
            b.pack(side="left", padx=(0, 6), pady=10)
            return b

        button("◫ Open", self.open_file)
        button("◇ Save", lambda: self.save_file())
        tk.Frame(header, bg=SURFACE, width=1, height=24).pack(
            side="left", padx=(6, 12), pady=14
        )
        button("▶ Run", lambda: self.run_current("run"), accent=True)
        button("⚙ Build", lambda: self.run_current("compile"))
        button("⌘ Tokens", lambda: self.run_current("tokens"))
        self.stop_button = button("■ Stop", self.stop_process)
        self.stop_button.set_disabled(True)
        tk.Frame(header, bg=SURFACE, width=1, height=24).pack(
            side="left", padx=(6, 12), pady=14
        )
        button("⋯ Interpreter", self.choose_interpreter)

        self.run_chip = tk.Label(
            header, text="● READY", bg=CRUST, fg=GREEN,
            font=(UI_FONT[0], 9, "bold"), padx=10,
        )
        self.run_chip.pack(side="right", padx=(0, 16))

        strip = tk.Canvas(self, height=3, bg=CRUST,
                          highlightthickness=0, bd=0)
        strip.pack(side="top", fill="x")

        def paint_gradient(_event=None) -> None:
            w = strip.winfo_width()
            strip.delete("all")
            if w < 4:
                return
            steps = min(160, max(4, w // 5))
            for i in range(steps):
                t = i / (steps - 1)
                color = (mix(LAVENDER, BLUE, t * 2) if t < 0.5
                         else mix(BLUE, PINK, (t - 0.5) * 2))
                strip.create_line(
                    i * w / steps, 0, (i + 1) * w / steps + 1, 3,
                    fill=color, width=1,
                )

        strip.bind("<Configure>", paint_gradient)

    def _build_ui(self) -> None:
        self.configure(bg=BASE)
        self._build_header()

        status = tk.Frame(self, bg=MANTLE, height=28)
        status.pack(side="bottom", fill="x")
        tk.Frame(status, bg=SURFACE, height=1).pack(side="top", fill="x")
        self.status_left = tk.Label(
            status, text="", bg=MANTLE, fg=MUTED,
            font=(UI_FONT[0], 9), anchor="w",
        )
        self.status_left.pack(side="left", padx=(14, 0), pady=5)
        self.status_right = tk.Label(
            status, text="", bg=MANTLE, fg=COMMENT,
            font=(UI_FONT[0], 9), anchor="e",
        )
        self.status_right.pack(side="right", padx=(0, 14), pady=5)

        main = tk.PanedWindow(
            self, orient="horizontal", bg=BASE,
            sashwidth=6, sashrelief="flat", opaqueresize=True,
        )
        main.pack(fill="both", expand=True)

        sidebar = tk.Frame(main, bg=MANTLE, width=240)
        work = tk.Frame(main, bg=BASE)
        main.add(sidebar, stretch="never")
        main.add(work, stretch="always")

        explorer_head = tk.Frame(sidebar, bg=MANTLE, height=42)
        explorer_head.pack(fill="x", padx=(14, 10), pady=(12, 6))
        tk.Label(
            explorer_head, text="EXPLORER", bg=MANTLE, fg=LAVENDER,
            font=(UI_FONT[0], 9, "bold"),
        ).pack(side="left")
        RoundButton(
            explorer_head, "⋯", command=self.choose_explorer_dir,
            width=28, height=26, bg=MANTLE,
        ).pack(side="right")
        RoundButton(
            explorer_head, "↻", command=self.populate_explorer,
            width=28, height=26, bg=MANTLE,
        ).pack(side="right", padx=(0, 4))
        tk.Frame(sidebar, bg=SURFACE, height=1).pack(fill="x")

        tree_frame = tk.Frame(sidebar, bg=MANTLE)
        tree_frame.pack(fill="both", expand=True, padx=8, pady=(4, 10))
        self.tree = ttk.Treeview(tree_frame, show="tree", selectmode="browse")
        tree_scroll = ttk.Scrollbar(tree_frame, orient="vertical",
                                    command=self.tree.yview)
        self.tree.configure(yscrollcommand=tree_scroll.set)
        self.tree.pack(side="left", fill="both", expand=True)
        tree_scroll.pack(side="right", fill="y")
        self.tree.bind("<Double-1>", self._on_tree_double)
        self.populate_explorer()

        self.notebook = ModernNotebook(work, on_close=self.close_tab)
        self.notebook.pack(fill="both", expand=True)
        self.notebook.bind("<<NotebookTabChanged>>", self._on_tab_changed)

        bottom = tk.Frame(work, bg=CRUST, height=210)
        bottom.pack(fill="both", side="bottom")
        tk.Frame(bottom, bg=SURFACE, height=1).pack(side="top", fill="x")

        out_head = tk.Frame(bottom, bg=MANTLE, height=34)
        out_head.pack(fill="x")
        tk.Label(
            out_head, text="OUTPUT", bg=MANTLE, fg=LAVENDER,
            font=(UI_FONT[0], 9, "bold"),
        ).pack(side="left", padx=(14, 0))
        RoundButton(
            out_head, "Copy", command=self.copy_output,
            width=58, height=26, bg=MANTLE,
        ).pack(side="right", padx=(0, 12))
        RoundButton(
            out_head, "Clear", command=self.clear_output,
            width=58, height=26, bg=MANTLE,
        ).pack(side="right")

        input_row = tk.Frame(bottom, bg=MANTLE, height=44)
        input_row.pack(fill="x", side="bottom")
        tk.Frame(input_row, bg=SURFACE, height=1).pack(side="top", fill="x")
        tk.Label(
            input_row, text="›", bg=MANTLE, fg=LAVENDER,
            font=(UI_FONT[0], 14, "bold"),
        ).pack(side="left", padx=(14, 8))
        self.input_var = tk.StringVar()
        self.input_entry = RoundEntry(
            input_row, textvariable=self.input_var,
            width=360, height=30, bg=MANTLE,
        )
        self.input_entry.pack(side="left", fill="x", expand=True, pady=7)
        self.input_entry.bind("<Return>", self.send_input)
        self.input_entry.set_state("disabled")
        RoundButton(
            input_row, "Send", command=self.send_input,
            width=64, height=30, bg=MANTLE,
        ).pack(side="right", padx=(8, 12))

        self.output = tk.Text(
            bottom,
            height=8,
            wrap="word",
            bg=CRUST,
            fg=TEXT,
            font=EDITOR_FONT,
            relief="flat",
            borderwidth=0,
            padx=14,
            pady=8,
            state="disabled",
        )
        self.output.tag_configure("info", foreground=MUTED)
        self.output.tag_configure("err", foreground=RED)
        self.output.tag_configure("ok", foreground=GREEN)
        self.output.tag_configure("input", foreground=BLUE)
        self.output.pack(fill="both", expand=True)

    def _bind_keys(self) -> None:
        self.bind("<Control-o>", lambda _e: self.open_file())
        self.bind("<Control-s>", lambda _e: self.save_file())
        self.bind("<Control-Shift-S>", lambda _e: self.save_as())
        self.bind("<F5>", lambda _e: self.run_current("run"))
        self.bind("<F6>", lambda _e: self.run_current("compile"))
        self.bind("<Control-w>", lambda _e: self.close_tab())
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    # ---------- tabs / files ----------

    def current_tab(self) -> EditorTab | None:
        current = self.notebook.select()
        if not current:
            return None
        for tab in self.tabs:
            if str(tab.frame) == str(current):
                return tab
        return None

    def new_file(self) -> EditorTab:
        tab = EditorTab(self, None, STARTER)
        self.tabs.append(tab)
        self.notebook.add(tab.frame, text=tab.display_name())
        self.notebook.select(tab.frame)
        self.update_status()
        return tab

    def open_file(self, path: str | None = None) -> None:
        if not path:
            chosen = filedialog.askopenfilename(
                title="Open WornewScript file",
                filetypes=[
                    ("WornewScript / source", "*.wor *.c *.h *.cpp *.cc *.txt"),
                    ("All files", "*"),
                ],
                initialdir=str(self.explorer_dir),
            )
            if not chosen:
                return
            path = chosen

        target = Path(path).resolve()
        for tab in self.tabs:
            if tab.path == target:
                self.notebook.select(tab.frame)
                return
        try:
            content = read_text_file(target)
        except OSError as exc:
            messagebox.showerror("WIDE", f"Cannot open file:\n{exc}")
            return

        tab = EditorTab(self, target, content)
        self.tabs.append(tab)
        self.notebook.add(tab.frame, text=tab.display_name())
        self.notebook.select(tab.frame)
        self.set_explorer_dir(target.parent)
        self._log(f"opened {target}", "info")
        self.update_status()

    def save_file(self, tab: EditorTab | None = None) -> bool:
        tab = tab or self.current_tab()
        if tab is None:
            return False
        if tab.path is None:
            return self.save_as(tab)
        try:
            tab.path.write_text(tab.text.get("1.0", "end-1c"), encoding="utf-8")
        except OSError as exc:
            messagebox.showerror("WIDE", f"Cannot save file:\n{exc}")
            return False
        tab.modified = False
        tab.text.edit_modified(False)
        self.update_tab_title(tab)
        self.populate_explorer()
        self.update_status()
        return True

    def save_as(self, tab: EditorTab | None = None) -> bool:
        tab = tab or self.current_tab()
        if tab is None:
            return False
        chosen = filedialog.asksaveasfilename(
            title="Save WornewScript file",
            defaultextension=".wor",
            filetypes=[("WornewScript", "*.wor"), ("All files", "*")],
            initialdir=str(self.explorer_dir),
            initialfile=tab.path.name if tab.path else "program.wor",
        )
        if not chosen:
            return False
        tab.path = Path(chosen).resolve()
        return self.save_file(tab)

    def close_tab(self) -> None:
        tab = self.current_tab()
        if tab is None:
            return
        if tab.modified:
            answer = messagebox.askyesnocancel(
                "WIDE", f"Save changes to {tab.display_name().strip('* ')}?"
            )
            if answer is None:
                return
            if answer and not self.save_file(tab):
                return
        index = self.notebook.index(tab.frame)
        self.notebook.forget(index)
        self.tabs.remove(tab)
        if not self.tabs:
            self.new_file()
        self.update_status()

    def update_tab_title(self, tab: EditorTab) -> None:
        try:
            index = self.notebook.index(tab.frame)
            self.notebook.tab(index, text=tab.display_name())
        except tk.TclError:
            pass

    def _on_tab_changed(self, _event=None) -> None:
        self.update_status()

    # ---------- explorer ----------

    def choose_explorer_dir(self) -> None:
        chosen = filedialog.askdirectory(initialdir=str(self.explorer_dir))
        if chosen:
            self.set_explorer_dir(Path(chosen))

    def set_explorer_dir(self, path: Path) -> None:
        if not path.is_dir():
            path = path.parent if path.parent.is_dir() else Path.cwd()
        self.explorer_dir = path.resolve()
        self.config_data["last_dir"] = str(self.explorer_dir)
        save_config(self.config_data)
        self.populate_explorer()

    def populate_explorer(self) -> None:
        for item in self.tree.get_children():
            self.tree.delete(item)
        root = self.explorer_dir
        self.tree.insert("", "end", iid=str(root), text=root.name or str(root), open=True)
        inserted = {str(root)}

        hidden = {".git", "__pycache__", "build", "dist", ".venv", "node_modules"}
        extensions = {".wor", ".c", ".h", ".cc", ".cpp", ".cxx", ".hpp", ".md", ".txt"}
        try:
            for dirpath, dirnames, filenames in os.walk(root):
                current = Path(dirpath)
                depth = len(current.relative_to(root).parts)
                if depth >= 4:
                    dirnames[:] = []
                    continue
                dirnames[:] = sorted(
                    d for d in dirnames if d not in hidden and not d.startswith(".")
                )
                parent = str(current) if str(current) in inserted else str(root)

                for name in dirnames:
                    full = current / name
                    try:
                        self.tree.insert(parent, "end", iid=str(full), text=name)
                        inserted.add(str(full))
                    except tk.TclError:
                        pass
                for name in sorted(filenames):
                    full = current / name
                    if full.suffix.lower() in extensions:
                        try:
                            self.tree.insert(parent, "end", iid=str(full), text=name)
                            inserted.add(str(full))
                        except tk.TclError:
                            pass
        except (OSError, ValueError):
            pass

    def _on_tree_double(self, _event=None) -> None:
        selection = self.tree.selection()
        if not selection:
            return
        target = Path(selection[0])
        if target.is_file():
            self.open_file(str(target))
        elif target.is_dir():
            state = self.tree.item(str(target), "open")
            self.tree.item(str(target), open=not bool(state))

    # ---------- interpreter / process ----------

    def ensure_interpreter(self, force: bool = False) -> str | None:
        if self.interpreter and not force and Path(self.interpreter).is_file():
            return self.interpreter
        chosen = filedialog.askopenfilename(
            title="Locate the wornew interpreter",
            filetypes=[("wornew executable", "*"), ("All files", "*")],
            initialdir=str(BASE_DIR.parent),
        )
        if not chosen:
            return None
        self.interpreter = chosen
        self.config_data["interpreter"] = chosen
        save_config(self.config_data)
        self.update_status()
        return self.interpreter

    def choose_interpreter(self) -> None:
        self.ensure_interpreter(force=True)

    def run_current(self, mode: str) -> None:
        tab = self.current_tab()
        if tab is None:
            return
        if tab.path is None:
            messagebox.showinfo("WIDE", "Save the file first (Ctrl+S).")
            return
        if tab.modified and not self.save_file(tab):
            return
        interpreter = self.ensure_interpreter()
        if not interpreter:
            return

        source = tab.path
        if mode == "run":
            cmd = [interpreter, str(source)]
        elif mode == "compile":
            suffix = ".exe" if os.name == "nt" else ""
            output = source.with_name(source.stem + suffix)
            cmd = [interpreter, "compile", str(source), "-o", str(output)]
        elif mode == "tokens":
            cmd = [interpreter, "tokens", str(source)]
        else:
            return

        self._start_process(cmd, source.parent)

    def _start_process(self, cmd: list[str], cwd: Path) -> None:
        if self.proc is not None and self.proc.poll() is None:
            messagebox.showinfo("WIDE", "A program is already running. Stop it first.")
            return

        self.clear_output()
        shown = " ".join(shlex.quote(part) for part in cmd)
        self._append_output(f"> {shown}\n", "info")

        kwargs = {}
        if os.name == "nt":
            kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        try:
            self.proc = subprocess.Popen(
                cmd,
                cwd=str(cwd),
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,
                **kwargs,
            )
        except OSError as exc:
            self.proc = None
            messagebox.showerror("WIDE", f"Cannot start interpreter:\n{exc}")
            return

        self.stop_button.set_disabled(False)
        self.input_entry.set_state("normal")
        self.run_chip.configure(text="● RUNNING", fg=YELLOW)
        self._polling = True
        thread = threading.Thread(target=self._reader, args=(self.proc,), daemon=True)
        thread.start()
        self.after(50, self._poll_queue)

    def _reader(self, proc: subprocess.Popen) -> None:
        try:
            assert proc.stdout is not None
            while True:
                chunk = proc.stdout.read(1)
                if not chunk:
                    break
                self.queue.put(("out", chunk))
        finally:
            try:
                if proc.stdout:
                    proc.stdout.close()
            except OSError:
                pass
            code = proc.wait()
            self.queue.put(("done", code))

    def _poll_queue(self) -> None:
        if not self._polling:
            return
        try:
            while True:
                kind, data = self.queue.get_nowait()
                if kind == "out":
                    self._append_output(str(data))
                elif kind == "done":
                    self._process_done(int(data))
        except queue.Empty:
            pass
        if self._polling:
            self.after(50, self._poll_queue)

    def _process_done(self, code: int) -> None:
        self._polling = False
        self.proc = None
        self.stop_button.set_disabled(True)
        self.input_entry.set_state("disabled")
        self.run_chip.configure(text="● READY", fg=GREEN)
        tag = "ok" if code == 0 else "err"
        self._append_output(f"\n[process exited with code {code}]\n", tag)

    def send_input(self, _event=None) -> None:
        line = self.input_var.get()
        self.input_var.set("")
        if self.proc is None or self.proc.stdin is None:
            return
        if self.proc.poll() is not None:
            return
        self._append_output(line + "\n", "input")
        try:
            self.proc.stdin.write(line + "\n")
            self.proc.stdin.flush()
        except (OSError, ValueError):
            pass

    def stop_process(self) -> None:
        if self.proc is None:
            return
        try:
            self.proc.terminate()
        except OSError:
            try:
                self.proc.kill()
            except OSError:
                pass

    # ---------- output / status ----------

    def _append_output(self, text: str, tag: str | None = None) -> None:
        self.output.configure(state="normal")
        if tag:
            self.output.insert("end", text, tag)
        else:
            self.output.insert("end", text)
        self.output.see("end")
        self.output.configure(state="disabled")

    def _log(self, text: str, tag: str = "info") -> None:
        self._append_output(text + "\n", tag)

    def clear_output(self) -> None:
        self.output.configure(state="normal")
        self.output.delete("1.0", "end")
        self.output.configure(state="disabled")

    def copy_output(self) -> None:
        text = self.output.get("1.0", "end-1c")
        try:
            self.clipboard_clear()
            self.clipboard_append(text)
        except tk.TclError:
            pass

    def update_status(self) -> None:
        tab = self.current_tab()
        if tab:
            line, col = tab.cursor_info()
            self.status_left.configure(text=f" {tab.display_name()}    Ln {line}, Col {col}")
        else:
            self.status_left.configure(text=" no file")
        interp = Path(self.interpreter).name if self.interpreter else "interpreter not set"
        self.status_right.configure(text=f"WornewScript  •  {interp}  •  UTF-8  •  {APP_NAME} {APP_VERSION}")

    # ---------- shutdown ----------

    def on_close(self) -> None:
        if self.proc is not None and self.proc.poll() is None:
            self.stop_process()
        for tab in list(self.tabs):
            if tab.modified:
                answer = messagebox.askyesnocancel(
                    "WIDE", f"Save changes to {tab.display_name().strip('* ')}?"
                )
                if answer is None:
                    return
                if answer and not self.save_file(tab):
                    return
        self.config_data["geometry"] = self.geometry()
        save_config(self.config_data)
        self.destroy()


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if "-h" in args or "--help" in args:
        print(f"{APP_NAME} {APP_VERSION} - IDE for WornewScript")
        print("Usage: wide [file.wor ...]")
        print("       wide --version")
        return 0
    if "--version" in args:
        print(f"{APP_NAME} {APP_VERSION}")
        return 0

    files = [arg for arg in args if not arg.startswith("-")]
    app = Wide(files)
    if "--smoke" in args:
        app.after(200, app.destroy)
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
