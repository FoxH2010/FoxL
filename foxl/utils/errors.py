# file: errors.py

from foxl.utils.diagnostics import Diagnostic, Span, Position, suggest


def token_to_position(token):
    return Position(
        line=token.line,
        column=token.column,
        index=0  # upgrade later if lexer supports absolute index
    )


def make_span(start_tok, end_tok):
    return Span(
        start=token_to_position(start_tok),
        end=token_to_position(end_tok)
    )

class ParseError(Exception):
    def __init__(
        self,
        message,
        token,
        expected=None,
        context=None,
        span=None,
        source=None,
        symbols=None,
        notes=None,
    ):
        self.message = message
        self.token = token
        self.expected = expected or []
        self.context = context
        self.source = source
        self.symbols = symbols or []
        self.notes = notes or []

        # --- build span ---
        if span:
            self.span = make_span(span[0], span[1])
        else:
            self.span = make_span(token, token)

        # --- build hint ---
        hint = None

        if self.symbols and token.type == "IDENTIFIER":
            guess = suggest(token.value, self.symbols)
            if guess:
                hint = f"Did you mean '{guess}'?"

        if self.expected:
            expected_str = ", ".join(self.expected)
            hint = (hint + " " if hint else "") + f"Expected: {expected_str}"

        # --- build notes ---
        diag_notes = []
        for msg, (s, e) in self.notes:
            diag_notes.append((msg, make_span(s, e)))

        # --- final diagnostic ---
        self.diagnostic = Diagnostic(
            level="error",
            message=self._build_message(),
            span=self.span,
            hint=hint,
            notes=diag_notes
        )

        super().__init__(message)

    def _build_message(self):
        parts = [self.message]

        if self.context:
            parts.append(f"[{self.context}]")

        parts.append(f"(got '{self.token.value}')")

        return " ".join(parts)
    
class FoxLRuntimeError(Exception):
    def __init__(self, message, node=None):
        self.message = message
        self.node = node
        self.start = getattr(node, "start", None)
        self.end = getattr(node, "end", None)