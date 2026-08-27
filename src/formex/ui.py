"""Full-screen terminal UI, built on the stdlib `curses` module.

This replaces the original ftxui-based UI. Colors are approximated with
curses' standard 8-color palette (orange -> yellow) since curses does not
portably support arbitrary RGB colors the way ftxui does.
"""

from __future__ import annotations

import curses
from dataclasses import dataclass, field

from .differentiator import Step, differentiate
from .lexer import Lexer
from .parser import Parser
from .printer import pretty_print
from .simplifier import simplify

RESULT_WIDTH = 30
FOOTER_ROWS = 4  # separator, history bar, separator, input line


@dataclass
class HistoryEntry:
    input: str
    result: str
    steps: list[Step] = field(default_factory=list)


def _init_colors() -> None:
    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_GREEN, -1)
    curses.init_pair(2, curses.COLOR_YELLOW, -1)
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW)
    curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_GREEN)


def _safe_addstr(win, y: int, x: int, s: str, attr: int = 0) -> None:
    h, w = win.getmaxyx()
    if y < 0 or y >= h or x < 0 or x >= w or not s:
        return
    s = s[: max(0, w - x - 1)]
    if not s:
        return
    try:
        win.addstr(y, x, s, attr)
    except curses.error:
        pass


def _draw(
    win,
    input_str: str,
    steps: list[Step],
    result: str,
    history: list[HistoryEntry],
    selected_history: int,
) -> None:
    win.erase()
    h, w = win.getmaxyx()

    top_h = max(0, h - FOOTER_ROWS)
    work_w = max(0, w - RESULT_WIDTH - 1)
    green = curses.color_pair(1)
    orange = curses.color_pair(2)
    badge_orange = curses.color_pair(3)
    badge_green = curses.color_pair(4)

    _safe_addstr(win, 0, 0, "WORKING", green | curses.A_DIM)
    _safe_addstr(win, 1, 0, "-" * work_w, green | curses.A_DIM)
    row = 2
    for i, step in enumerate(steps):
        if row >= top_h:
            break
        _safe_addstr(win, row, 0, f"{i + 1}  {step.rule}", green)
        row += 1
        if step.expr and row < top_h:
            _safe_addstr(win, row, 0, f"    {step.expr} -> {step.result}", orange)
            row += 1

    sep_x = work_w
    for y in range(top_h):
        _safe_addstr(win, y, sep_x, "|", green | curses.A_DIM)

    rx = sep_x + 2
    _safe_addstr(win, 0, rx, "RESULT", green | curses.A_DIM)
    _safe_addstr(win, 1, rx, "-" * max(0, w - rx), green | curses.A_DIM)
    _safe_addstr(win, 2, rx, result, orange | curses.A_BOLD)
    if top_h > 0:
        _safe_addstr(win, top_h - 1, rx, " symbolic ", badge_orange)

    sep_row = top_h
    _safe_addstr(win, sep_row, 0, "-" * w, green | curses.A_DIM)

    hist_row = sep_row + 1
    x = 0
    for i, entry in enumerate(history):
        label = f" {entry.input} {entry.result} "
        attr = green | (curses.A_REVERSE if i == selected_history else 0)
        _safe_addstr(win, hist_row, x, label, attr)
        x += len(label)
        if x < w and i < len(history) - 1:
            _safe_addstr(win, hist_row, x, "|", green | curses.A_DIM)
            x += 1

    sep_row2 = hist_row + 1
    _safe_addstr(win, sep_row2, 0, "-" * w, green | curses.A_DIM)

    input_row = sep_row2 + 1
    _safe_addstr(win, input_row, 0, " SYM ", badge_green | curses.A_BOLD)
    prompt_x = 6
    _safe_addstr(win, input_row, prompt_x, input_str, green)
    hint = " <-/-> cycle history   Esc quit "
    hint_x = max(prompt_x + len(input_str) + 1, w - len(hint))
    _safe_addstr(win, input_row, hint_x, hint, green | curses.A_DIM)

    try:
        win.move(input_row, min(prompt_x + len(input_str), max(0, w - 1)))
    except curses.error:
        pass

    win.refresh()


def _main(stdscr) -> None:
    curses.curs_set(1)
    _init_colors()
    stdscr.keypad(True)

    input_str = ""
    steps: list[Step] = []
    result = ""
    history: list[HistoryEntry] = []
    selected_history = -1

    while True:
        _draw(stdscr, input_str, steps, result, history, selected_history)

        try:
            key = stdscr.get_wch()
        except curses.error:
            continue

        if key == "\x1b":  # Esc
            break

        if key == curses.KEY_LEFT:
            if selected_history > 0:
                selected_history -= 1
                steps = history[selected_history].steps
                result = history[selected_history].result
            continue

        if key == curses.KEY_RIGHT:
            if selected_history < len(history) - 1:
                selected_history += 1
                steps = history[selected_history].steps
                result = history[selected_history].result
            continue

        if key in ("\n", "\r") or key == curses.KEY_ENTER:
            if not input_str:
                continue
            try:
                tokens = Lexer(input_str).tokenise()
                tree = Parser(tokens).construct_tree()
                diff_result = differentiate(tree, "x")

                if diff_result.result is None:
                    result = "error: unsupported expression"
                    steps = [Step("unsupported", input_str, "")]
                    input_str = ""
                    continue

                simplified = simplify(diff_result.result)
                steps = [Step(f"input: {input_str}", "", "")] + diff_result.steps
                result = pretty_print(simplified)
                history.append(HistoryEntry(input_str, result, steps))
                selected_history = len(history) - 1
                input_str = ""
            except Exception as e:  # noqa: BLE001 - mirrors the original catch-all
                result = f"error: {e}"
                steps = [Step("parse error", input_str, "")]
            continue

        if key in (curses.KEY_BACKSPACE, "\x7f", "\b") or key == 127:
            input_str = input_str[:-1]
            continue

        if isinstance(key, str) and key.isprintable():
            input_str += key


def run_ui() -> None:
    curses.wrapper(_main)
