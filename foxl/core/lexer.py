import re
from typing import List, NamedTuple
from foxl.utils.diagnostics import Position


class Token(NamedTuple):
    type: str
    value: str
    start: Position
    end: Position


class LexerError(Exception):
    pass


class Lexer:
    KEYWORDS = {
        "let", "function", "record", "match", "if", "else",
        "try", "catch", "async", "await", "actor", "spawn",
        "send", "return", "raise", "exit", "break", "continue", "while",
        "class", "import", "this", "as"
    }

    BOOLEAN_LITERALS = {"true", "false", "null"}

    TOKEN_SPEC = [
        ("COMMENT_MULTI", r"/\*[\s\S]*?\*/"),
        ("COMMENT_SINGLE", r"//.*"),
        ("WHITESPACE", r"[ \t\n\r]+"),

        # --- Operators (ordered by priority) ---
        ("PIPELINE", r"\|>"),
        ("ARROW", r"->"),
        ("SPREAD", r"\.\.\."),
        ("EQ_STRICT", r"==="),
        ("EQ", r"=="),
        ("NEQ", r"!="),
        ("GTE", r">="),
        ("LTE", r"<="),

        # --- Single-char ---
        ("AND", r"&&"),
        ("OR", r"\|\|"),
        ("BANG", r"!"),
        ("PLUS", r"\+"),
        ("MINUS", r"-"),
        ("STAR", r"\*"),
        ("SLASH", r"/"),
        ("ASSIGN", r"="),
        ("GT", r">"),
        ("LT", r"<"),
        ("PIPE", r"\|"),
        ("DOT", r"\."),

        ("LBRACE", r"\{"),
        ("RBRACE", r"\}"),
        ("LPAREN", r"\("),
        ("RPAREN", r"\)"),
        ("LBRACKET", r"\["),
        ("RBRACKET", r"\]"),
        ("COLON", r":"),
        ("COMMA", r","),
        ("SEMICOLON", r";"),

        # --- Literals ---
        ("NUMBER", r"\d+(\.\d+)?"),
        ("STRING", r'"([^"\\]|\\.)*"'),

        # --- Identifiers ---
        ("IDENTIFIER", r"[A-Za-z_][A-Za-z0-9_]*"),
    ]

    def __init__(self, code: str):
        self.code = code

        parts = []
        for name, pattern in self.TOKEN_SPEC:
            parts.append(f"(?P<{name}>{pattern})")
        self.master_pattern = re.compile("|".join(parts))

    def tokenize(self) -> List[Token]:
        tokens: List[Token] = []
        line = 1
        col = 1

        pos = 0
        index = 0
        while pos < len(self.code):
            match = self.master_pattern.match(self.code, pos)
            if not match:
                raise LexerError(f"Unexpected character at {line}:{col}")

            kind = match.lastgroup
            value = match.group()

            if kind in ("WHITESPACE", "COMMENT_SINGLE", "COMMENT_MULTI"):
                line, col = self._update_position(value, line, col)
                index += len(value)
            else:
                if kind == "IDENTIFIER":
                    if value == "_":
                        kind = "WILDCARD"
                    elif value in self.KEYWORDS:
                        kind = "KEYWORD"
                    elif value in {"true", "false"}:
                        kind = "BOOLEAN"
                    elif value == "null":
                        kind = "NULL"

                if kind == "STRING":
                    value = bytes(value[1:-1], "utf-8").decode("unicode_escape")

                start_pos = Position(line, col, index)

                line, col = self._update_position(value, line, col)
                index += len(value)

                end_pos = Position(line, col, index)

                tokens.append(Token(kind, value, start_pos, end_pos))

            pos = match.end()

        tokens.append(Token("EOF", "", Position(line, col, index), Position(line, col, index)))
        return tokens
    
    def _update_position(self, text: str, line: int, col: int):
        lines = text.split("\n")
        if len(lines) > 1:
            line += len(lines) - 1
            col = len(lines[-1]) + 1
        else:
            col += len(text)
        return line, col
