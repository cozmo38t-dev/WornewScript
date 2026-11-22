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
        self.notebook: ttk.Notebook
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
        style = ttk.Style(self)
        style.theme_use("clam")

        style.configure("TFrame", background=BASE)
        style.configure("Status.TFrame", background=MANTLE)
        style.configure("TLabel", background=BASE, foreground=TEXT, font=UI_FONT)
        style.configure("Muted.TLabel", foreground=MUTED)
        style.configure("Kicker.TLabel", foreground=LAVENDER, font=(UI_FONT[0], 9, "bold"))
        style.configure("Status.TLabel", background=MANTLE, foreground=MUTED, font=(UI_FONT[0], 9))

        style.configure(
            "TButton",
            background=SURFACE,
            foreground=TEXT,
            borderwidth=0,
            focusthickness=0,
            padding=(10, 6),
            font=UI_FONT,
        )
        style.map(
            "TButton",
            background=[("active", SURFACE2), ("disabled", SURFACE)],
            foreground=[("disabled", MUTED)],
        )
        style.configure("Accent.TButton", background=LAVENDER, foreground=MANTLE)
        style.map("Accent.TButton", background=[("active", BLUE)])

        style.configure("TNotebook", background=BASE, borderwidth=0)
        style.configure(
            "TNotebook.Tab",
            background=SURFACE,
            foreground=MUTED,
            padding=(14, 7),
            borderwidth=0,
            font=UI_FONT,
        )
        style.map(
            "TNotebook.Tab",
            background=[("selected", BASE)],
            foreground=[("selected", LAVENDER)],
        )

        style.configure(
            "Treeview",
            background=MANTLE,
            fieldbackground=MANTLE,
            foreground=TEXT,
            rowheight=24,
            borderwidth=0,
            font=UI_FONT,
        )
        style.map("Treeview", background=[("selected", SURFACE2)])
        style.configure("Treeview.Heading", background=SURFACE, foreground=LAVENDER, font=UI_FONT)

        style.configure("TScrollbar", background=SURFACE, troughcolor=MANTLE, borderwidth=0)
        style.configure("TPanedwindow", background=BASE)

    def _build_ui(self) -> None:
        toolbar = ttk.Frame(self, padding=(10, 8))
        toolbar.pack(side="top", fill="x")

        def button(label: str, command, accent: bool = False) -> ttk.Button:
            b = ttk.Button(
                toolbar,
                text=label,
                command=command,
                style="Accent.TButton" if accent else "TButton",
            )
            b.pack(side="left", padx=(0, 6))
            return b

        button("◫ Open", self.open_file)
        button("◇ Save", lambda: self.save_file())
        ttk.Separator(toolbar, orient="vertical").pack(side="left", fill="y", padx=8)
        self.run_button = button("▶ Run", lambda: self.run_current("run"), accent=True)
        button("⚙ Build", lambda: self.run_current("compile"))
        button("⌘ Tokens", lambda: self.run_current("tokens"))
        self.stop_button = button("■ Stop", self.stop_process)
        self.stop_button.configure(state="disabled")
        ttk.Separator(toolbar, orient="vertical").pack(side="left", fill="y", padx=8)
        button("⋯ Interpreter", self.choose_interpreter)

        status = ttk.Frame(self, height=26, padding=(12, 4), style="Status.TFrame")
        status.pack(side="bottom", fill="x")
        self.status_left = ttk.Label(status, text="", style="Status.TLabel")
        self.status_left.pack(side="left")
        self.status_right = ttk.Label(status, text="", style="Status.TLabel")
        self.status_right.pack(side="right")

        main = ttk.PanedWindow(self, orient="horizontal")
        main.pack(fill="both", expand=True)

        sidebar = ttk.Frame(main, width=230)
        work = ttk.Frame(main)
        main.add(sidebar, weight=0)
        main.add(work, weight=1)

        explorer_head = ttk.Frame(sidebar, padding=(10, 8))
        explorer_head.pack(fill="x")
        ttk.Label(explorer_head, text="EXPLORER", style="Kicker.TLabel").pack(side="left")
        ttk.Button(explorer_head, text="⋯", width=3, command=self.choose_explorer_dir).pack(side="right")
        ttk.Button(explorer_head, text="↻", width=3, command=self.populate_explorer).pack(side="right", padx=(0, 4))

        tree_frame = ttk.Frame(sidebar)
        tree_frame.pack(fill="both", expand=True)
        self.tree = ttk.Treeview(tree_frame, show="tree", selectmode="browse")
        tree_scroll = ttk.Scrollbar(tree_frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=tree_scroll.set)
        self.tree.pack(side="left", fill="both", expand=True)
        tree_scroll.pack(side="right", fill="y")
        self.tree.bind("<Double-1>", self._on_tree_double)
        self.populate_explorer()

        self.notebook = ttk.Notebook(work)
        self.notebook.pack(fill="both", expand=True)
        self.notebook.bind("<<NotebookTabChanged>>", self._on_tab_changed)

        bottom = ttk.Frame(work, height=210)
        bottom.pack(fill="x", side="bottom")
        out_head = ttk.Frame(bottom, padding=(10, 6))
        out_head.pack(fill="x")
        ttk.Label(out_head, text="OUTPUT", style="Kicker.TLabel").pack(side="left")
        ttk.Button(out_head, text="Clear", command=self.clear_output).pack(side="right")
        ttk.Button(out_head, text="Copy", command=self.copy_output).pack(side="right", padx=(0, 6))

        self.output = tk.Text(
            bottom,
            height=9,
            wrap="word",
            bg=CRUST,
            fg=TEXT,
            font=EDITOR_FONT,
            relief="flat",
            borderwidth=0,
            padx=12,
            pady=8,
            state="disabled",
        )
        self.output.tag_configure("info", foreground=MUTED)
        self.output.tag_configure("err", foreground=RED)
        self.output.tag_configure("ok", foreground=GREEN)
        self.output.tag_configure("input", foreground=BLUE)
        self.output.pack(fill="both", expand=True)

        input_row = ttk.Frame(bottom, padding=(10, 6))
        input_row.pack(fill="x")
        ttk.Label(input_row, text="›", style="Kicker.TLabel").pack(side="left", padx=(0, 6))
        self.input_var = tk.StringVar()
        self.input_entry = ttk.Entry(input_row, textvariable=self.input_var, state="disabled")
        self.input_entry.pack(side="left", fill="x", expand=True)
        self.input_entry.bind("<Return>", self.send_input)
        ttk.Button(input_row, text="Send", command=self.send_input).pack(side="left", padx=(6, 0))

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

        self.stop_button.configure(state="normal")
        self.input_entry.configure(state="normal")
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
        self.stop_button.configure(state="disabled")
        self.input_entry.configure(state="disabled")
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
