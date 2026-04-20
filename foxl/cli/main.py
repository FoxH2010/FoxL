import os
import sys

from foxl.core.lexer import Lexer, LexerError
from foxl.core.parser import Parser
from foxl.utils.errors import ParseError
from foxl.core.semantic import SemanticAnalyzer
from foxl.core.interpreter import Interpreter, ExitSignal
from foxl.utils.diagnostics import DiagnosticRenderer
from foxl.utils.diagnostics import Diagnostic, Span, Position
from foxl.utils.errors import FoxLRuntimeError

def runtime_to_diagnostic(err: FoxLRuntimeError):
    if err.start and err.end:
        span = Span(err.start, err.end)
    elif not err.start or not err.end:
        return Diagnostic(
            level="error",
            message=err.message,
            span=Span(
                Position(1, 1, 0),
                Position(1, 1, 0)
            )
        )

    return Diagnostic(
        level="error",
        message=err.message,
        span=span,
        hint="check operand types"
    )


def run_foxl(source: str, interpreter=None, filename=None, cli_args=None):
    try:
        base_dir = os.path.dirname(os.path.abspath(filename)) if filename else os.getcwd()
        lexer = Lexer(source)
        tokens = lexer.tokenize()

        parser = Parser(tokens, source)
        ast = parser.parse()

        diagnostics = parser.diagnostics

        analyzer = SemanticAnalyzer(base_dir=base_dir)
        diagnostics += analyzer.analyze(ast)

        if diagnostics:
            renderer = DiagnosticRenderer(source)
            for d in diagnostics:
                renderer.render(d)
            return

        if interpreter is None:
            interpreter = Interpreter(base_dir=base_dir, cli_args=cli_args)

        result = interpreter.eval_program(ast)

        if result is not None:
            print(result)

    except (LexerError, ParseError) as e:
        renderer = DiagnosticRenderer(source)
        renderer.render(e)

    except FoxLRuntimeError as e:
        renderer = DiagnosticRenderer(source)
        renderer.render(runtime_to_diagnostic(e))


def run_file(source: str, filename=None, cli_args=None):
    run_foxl(source, filename=filename, cli_args=cli_args)


def repl():
    interpreter = Interpreter()

    while True:
        try:
            line = input("FoxL> ").strip()
            if not line:
                continue

            result = run_foxl(line, interpreter=interpreter)

            if result is not None:
                print(f"=> {result}")

        except ExitSignal as e:
            if e.value:
                print(e.value)
            break
        except Exception as e:
            print(f"RuntimeError: {e}")


def main():
    args = sys.argv[1:]

    if not args:
        repl()
        return

    if args[0] in ("--help", "-h"):
        print("foxl [file]")
        print("Runs FoxL language")
        return

    filename = args[0]

    try:
        with open(filename, "r", encoding="utf-8") as f:
            source = f.read()
    except FileNotFoundError:
        print(f"File not found: {filename}")
        return

    run_file(source, filename=filename, cli_args=args[1:])


if __name__ == "__main__":
    main()
