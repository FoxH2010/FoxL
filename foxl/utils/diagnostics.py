from dataclasses import dataclass
from typing import List, Optional


# --- Position / Span ---

@dataclass
class Position:
    line: int
    column: int
    index: int


@dataclass
class Span:
    start: Position
    end: Position


# --- Diagnostic ---

class Diagnostic:
    def __init__(
        self,
        level: str,
        message: str,
        span: Span,
        hint: Optional[str] = None,
        notes: Optional[List[tuple]] = None,
    ):
        self.level = level  # error | warning | hint
        self.message = message
        self.span = span
        self.hint = hint
        self.notes = notes or []


# --- Colors ---

class Color:
    RED = "\033[31m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    CYAN = "\033[36m"
    BOLD = "\033[1m"
    RESET = "\033[0m"


LEVEL_COLOR = {
    "error": Color.RED,
    "warning": Color.YELLOW,
    "hint": Color.CYAN,
}


# --- Renderer ---

class DiagnosticRenderer:
    def __init__(self, source: str):
        self.lines = source.splitlines()

    def render(self, diag: Diagnostic):
        color = LEVEL_COLOR.get(diag.level, "")
        header = f"{color}{diag.level.upper()}{Color.RESET}: {diag.message}"
        print(header)

        self._render_span(diag.span)

        if diag.hint:
            print(f"{Color.BLUE}help:{Color.RESET} {diag.hint}")

        for note_msg, note_span in diag.notes:
            print(f"{Color.CYAN}note:{Color.RESET} {note_msg}")
            self._render_span(note_span)

    def _render_span(self, span: Span):
        start = span.start
        end = span.end

        # 🔥 defensive fix
        if hasattr(start, "start"):  # it's a Token
            start = start.start
        if hasattr(end, "end"):
            end = end.end

        if not start or not end:
            print("     | (no location)")
            return

        for line_no in range(start.line, end.line + 1):
            line = self.lines[line_no - 1]
            print(f"{line_no:4} | {line}")

            if line_no == start.line:
                start_col = start.column
            else:
                start_col = 1

            if line_no == end.line:
                end_col = end.column
            else:
                end_col = len(line)

            underline = " " * (start_col - 1) + "^" * max(1, end_col - start_col)
            print(f"     | {Color.RED}{underline}{Color.RESET}")


# --- Suggestion (Levenshtein) ---

def levenshtein(a: str, b: str) -> int:
    dp = [[i + j if i * j == 0 else 0 for j in range(len(b) + 1)] for i in range(len(a) + 1)]

    for i in range(1, len(a) + 1):
        for j in range(1, len(b) + 1):
            cost = 0 if a[i - 1] == b[j - 1] else 1
            dp[i][j] = min(
                dp[i - 1][j] + 1,
                dp[i][j - 1] + 1,
                dp[i - 1][j - 1] + cost,
            )
    return dp[-1][-1]


def suggest(name: str, candidates: List[str]) -> Optional[str]:
    best = None
    best_dist = 3  # threshold

    for c in candidates:
        dist = levenshtein(name, c)
        if dist < best_dist:
            best_dist = dist
            best = c

    return best