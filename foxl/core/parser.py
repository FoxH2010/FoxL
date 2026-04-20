# file: FoxLPy/parser.py

from typing import List, Optional
from foxl.core.lexer import Token
from foxl.core.ast import *
from foxl.utils.errors import ParseError
from foxl.core.semantic import Assign
from foxl.utils.diagnostics import Diagnostic, Span


# ------------------------
# Patterns
# ------------------------

class Pattern:
    pass


class WildcardPattern(Pattern):
    pass


class LiteralPattern(Pattern):
    def __init__(self, value):
        self.value = value


class IdentifierPattern(Pattern):
    def __init__(self, name):
        self.name = name


class OrPattern(Pattern):
    def __init__(self, left, right):
        self.left, self.right = left, right


class GuardPattern(Pattern):
    def __init__(self, pattern, guard):
        self.pattern, self.guard = pattern, guard


class ArrayPattern(Pattern):
    def __init__(self, items):
        self.items = items


class ObjectPattern(Pattern):
    def __init__(self, name, props):
        self.name, self.props = name, props


class CallPattern(Pattern):
    def __init__(self, name, args):
        self.name, self.args = name, args


# ------------------------
# Parser
# ------------------------

class Parser:
    RIGHT_ASSOC = {"ARROW", "POW", "ASSIGN"}

    BP = {
        "ASSIGN": 0,
        "ARROW": 0,
        "OR": 1,
        "AND": 2,
        "PIPELINE": 1,
        "PIPE": 2,
        "EQ": 3, "EQ_STRICT": 3,
        "GT": 4, "LT": 4, "GTE": 4, "LTE": 4,
        "PLUS": 5, "MINUS": 5,
        "STAR": 6, "SLASH": 6,
        "POW": 7,
        "DOT": 100,
        "LPAREN": 100
    }

    def __init__(self, tokens: List[Token], source: str = ""):
        self.tokens = tokens
        self.source = source
        self.pos = 0
        self.symbols = set()
        self.diagnostics = []

    # ------------------------
    # Core helpers
    # ------------------------

    def peek(self):
        if self.pos >= len(self.tokens):
            return self.tokens[-1]  # EOF token
        return self.tokens[self.pos]

    def peek_next(self) -> Optional[Token]:
        return self.tokens[self.pos + 1] if self.pos + 1 < len(self.tokens) else None

    def advance(self):
        if self.pos >= len(self.tokens):
            return self.tokens[-1]
        tok = self.tokens[self.pos]
        self.pos += 1
        return tok

    def match(self, *types):
        if self.peek().type in types:
            return self.advance()
        return None

    def match_keyword(self, value: str):
        tok = self.peek()
        if tok.type == "KEYWORD" and tok.value == value:
            return self.advance()
        return None

    def expect(self, type_, context=None):
        tok = self.peek()

        if tok.type != type_:
            self.error(f"Expected {type_}", tok, context)
            return tok  # recovery: return bad token

        return self.advance()

    def error(self, message, token=None, expected=None, context=None, start=None, notes=None):
        span = Span(token.start, token.end)

        self.diagnostics.append(
            Diagnostic(
                "error",
                message,
                span,
                hint=context or "syntax error"
            )
        )

    def synchronize(self):
        while self.peek().type != "EOF":
            if self.peek().type == "SEMICOLON":
                self.advance()
                return
            if self.peek().type == "KEYWORD":
                return
            self.advance()

    # ------------------------
    # Entry
    # ------------------------

    def parse(self):
        statements = []

        while self.peek().type != "EOF":
            stmt = self.statement()
            if stmt is not None:
                statements.append(stmt)
            self.match("SEMICOLON")

        return Block(statements)

    # ------------------------
    # Statements
    # ------------------------

    def statement(self):
        tok = self.peek()

        if self.match_keyword("async"):
            if not self.match_keyword("function"):
                self.error("Expected function", self.peek(), context="async function")
            return self.parse_function(is_async=True)

        if self.match_keyword("function"):
            return self.parse_function()

        if tok.type == "KEYWORD":
            if tok.value == "if":
                return self.parse_if(self.advance())

            if tok.value == "while":
                return self.parse_while(self.advance())

            if tok.value == "match":
                return self.parse_match(self.advance())

            if tok.value == "try":
                return self.parse_try(self.advance())

            if tok.value == "let":
                return self.parse_let(self.advance())

            if tok.value == "return":
                return self.parse_return(self.advance())

            if tok.value == "raise":
                return self.parse_raise(self.advance())

            if tok.value == "record":
                return self.parse_record(self.advance())

            if tok.value == "actor":
                return self.parse_actor(self.advance())

            if tok.value == "class":
                return self.parse_class(self.advance())

            if tok.value == "import":
                return self.parse_import(self.advance())

            if tok.value == "exit":
                if self.peek_next() and self.peek_next().type == "LPAREN":
                    return self.expression(0)
                return self.parse_exit(self.advance())

            if tok.value == "break":
                self.advance()
                return Break()

            if tok.value == "continue":
                self.advance()
                return Continue()

        if tok.type == "LBRACE":
            return self.parse_block()

        return self.expression(0)
    
    def parse_function(self, is_async=False):
        name_tok = self.expect("IDENTIFIER", "function name")
        params, return_type, body = self.parse_function_parts()
        return set_span(Function(name_tok.value, params, body, is_async=is_async, return_type=return_type), name_tok, body.end)

    def parse_function_expression(self, start_tok, is_async=False):
        params, return_type, body = self.parse_function_parts()
        return set_span(FunctionExpr(params, body, is_async=is_async, return_type=return_type), start_tok, body.end)

    def parse_function_parts(self):
        self.expect("LPAREN", "function params")

        params = []
        if self.peek().type != "RPAREN":
            while True:
                tok = self.expect("IDENTIFIER", "param")
                type_name = None
                if self.match("COLON"):
                    type_name = self.parse_type_expression(stop_tokens={"COMMA", "RPAREN"})
                params.append((tok.value, type_name))
                if not self.match("COMMA"):
                    break

        self.expect("RPAREN", "function params")

        return_type = None
        if self.match("ARROW"):
            return_type = self.parse_type_expression()

        body = self.parse_block()
        return params, return_type, body

    # ------------------------
    # Expression (Pratt)
    # ------------------------

    def expression(self, min_bp):
        first = self.advance()
        left = self.nud(first)

        while True:
            tok = self.peek()

            # 🔥 STOP tokens (VERY IMPORTANT)
            if tok.type in {"RPAREN", "RBRACE", "RBRACKET", "COMMA", "SEMICOLON", "EOF"}:
                break

            if tok.type not in self.BP:
                break

            bp = self.BP[tok.type]
            if bp < min_bp:
                break

            op = self.advance()

            # postfix
            if op.type in {"LPAREN", "DOT"}:
                left = self.led(op, left)
                continue

            # infix
            next_bp = bp if op.type in self.RIGHT_ASSOC else bp + 1
            right = self.expression(next_bp)

            left = self.led(op, left, right)

        return left

    # ------------------------
    # NUD
    # ------------------------

    def nud(self, tok):
        t = tok.type

        if t == "NUMBER":
            value = tok.value
            num = float(value) if "." in value else int(value)
            return set_span(Number(num), tok, tok)
        
        if t == "BOOLEAN":
            return set_span(Boolean(tok.value == "true"), tok, tok)
        
        if t == "NULL":
            return set_span(Null(), tok, tok)

        if t == "KEYWORD" and tok.value == "function":
            return self.parse_function_expression(tok)

        if t == "KEYWORD" and tok.value == "if":
            return self.parse_if(tok)

        if t == "KEYWORD" and tok.value == "this":
            return set_span(Identifier("this"), tok, tok)

        if t == "KEYWORD" and tok.value in {"send", "read", "fetch"}:
            return set_span(Identifier(tok.value), tok, tok)

        if t == "KEYWORD" and tok.value == "spawn":
            target = self.expression(10)
            callee = set_span(Identifier("spawn"), tok, tok)
            return set_span(Call(callee, [target]), tok, target.end)

        if t == "KEYWORD" and tok.value == "await":
            expr = self.expression(10)
            return set_span(Await(expr), tok, expr.end)

        if t == "STRING":
            return set_span(String(tok.value), tok, tok)

        if t == "IDENTIFIER":
            self.symbols.add(tok.value)
            return set_span(Identifier(tok.value), tok, tok)

        if t == "LPAREN":
            expr = self.expression(0)
            end = self.expect("RPAREN", "grouping")
            return set_span(expr, tok, end)

        if t in ("PLUS", "MINUS", "BANG"):
            expr = self.expression(10)
            return set_span(UnaryOp(tok.value, expr), tok, expr.end)

        if t == "LBRACKET":
            return self.parse_array(tok)

        if t == "LBRACE":
            return self.parse_brace(tok)

        self.error("Unexpected token", tok, context="expression")
        return set_span(ErrorNode(), tok, tok)

    def parse_array(self, start):
        items = []
        if self.peek().type != "RBRACKET":
            while True:
                if self.match("SPREAD"):
                    items.append(Spread(self.expression(0)))
                else:
                    items.append(self.expression(0))
                if not self.match("COMMA"):
                    break
        end = self.expect("RBRACKET", "array")
        return set_span(ArrayLiteral(items), start, end)

    def parse_brace(self, start):
        if self.peek().type == "IDENTIFIER":
            nxt = self.peek_next()
            if nxt and nxt.type in {"COLON", "COMMA", "RBRACE"}:
                return self.parse_object_literal(start)

        self.pos -= 1
        return self.parse_block()

    # ------------------------
    # LED
    # ------------------------

    def led(self, tok, left, right=None):
        t = tok.type

        if t in {"PLUS","MINUS","STAR","SLASH","EQ","EQ_STRICT","GT","LT","GTE","LTE","PIPE","POW", "AND", "OR"}:
            return set_span(BinaryOp(left, tok.value, right), left.start, right.end)

        if t == "PIPELINE":
            return set_span(Pipeline(left, right), left.start, right.end)

        if t == "DOT":
            prop = self.expect("IDENTIFIER", "member access")
            return set_span(Member(left, prop.value), left.start, prop)

        if t == "ASSIGN":
            if not isinstance(left, (Identifier, Member)):
                self.error("Invalid assignment target", tok)
                return left
            return set_span(Assign(left, right), left.start, right.end)

        if t == "LPAREN":
            args = []
            if self.peek().type != "RPAREN":
                while True:
                    args.append(self.expression(0))
                    if not self.match("COMMA"):
                        break
            end = self.expect("RPAREN", "call")
            return set_span(Call(left, args), left.start, end)

        if t == "ARROW":
            if not isinstance(left, Identifier):
                self.error("Lambda param must be identifier", tok)
                return set_span(ErrorNode(), tok, right.end)
            return set_span(Lambda(left.name, right), left.start, right.end)

        self.error("Unknown operator", tok)
        return left  # recovery

    # ------------------------
    # Blocks / Objects
    # ------------------------

    def parse_block(self):
        start = self.expect("LBRACE", "block")
        statements = []

        while self.peek().type not in {"RBRACE", "EOF"}:
            statements.append(self.statement())
            self.match("SEMICOLON")

        end = self.expect("RBRACE", "block")
        return set_span(Block(statements), start, end)

    def parse_object_literal(self, start):
        props = []

        while self.peek().type != "RBRACE":
            if self.match("SPREAD"):
                props.append(Spread(self.expression(0)))
            else:
                key_tok = self.expect("IDENTIFIER")
                key = key_tok.value
                if self.match("COLON"):
                    val = self.expression(0)
                else:
                    val = set_span(Identifier(key), key_tok, key_tok)
                props.append((key, val))

            if not self.match("COMMA"):
                break

        end = self.expect("RBRACE")
        return set_span(ObjectLiteral(props), start, end)

    # ------------------------
    # Control Flow
    # ------------------------

    def parse_if(self, start):
        cond = self.expression(0)
        then = self.parse_block() if self.peek().type == "LBRACE" else self.statement()

        else_branch = None
        if self.match_keyword("else"):
            else_branch = self.parse_block() if self.peek().type == "LBRACE" else self.statement()

        end = else_branch.end if else_branch else then.end
        return set_span(If(cond, then, else_branch), start, end)

    def parse_while(self, start):
        cond = self.expression(0)
        body = self.parse_block()
        return set_span(While(cond, body), start, body.end)

    def parse_let(self, start):
        name = self.expect("IDENTIFIER").value
        type_name = None
        if self.match("COLON"):
            type_name = self.parse_type_expression(stop_tokens={"ASSIGN"})
        self.expect("ASSIGN")
        value = self.expression(0)
        return set_span(Let(name, value, type_name=type_name), start, value.end)

    def parse_return(self, start):
        if self.peek().type in ("SEMICOLON", "RBRACE", "EOF"):
            return set_span(Return(None), start, start)
        val = self.expression(0)
        return set_span(Return(val), start, val.end)

    def parse_exit(self, start_tok):
        if self.peek().type in ("SEMICOLON", "RBRACE", "EOF"):
            return set_span(Exit(None), start_tok, start_tok)

        value = self.expression(0)
        return set_span(Exit(value), start_tok, value.end)

    def parse_raise(self, start_tok):
        value = self.expression(0)
        return set_span(Raise(value), start_tok, value.end)

    def parse_try(self, start_tok):
        try_block = self.parse_block()
        if not self.match_keyword("catch"):
            self.error("Expected catch", self.peek(), context="try/catch")
        self.expect("LPAREN", "catch")
        error_name = self.expect("IDENTIFIER", "catch variable").value
        self.expect("RPAREN", "catch")
        catch_block = self.parse_block()
        return set_span(TryCatch(try_block, error_name, catch_block), start_tok, catch_block.end)

    def parse_record(self, start_tok):
        name_tok = self.expect("IDENTIFIER", "record name")
        self.expect("LBRACE", "record body")
        fields = []

        while self.peek().type not in {"RBRACE", "EOF"}:
            field_name = self.expect("IDENTIFIER", "record field").value
            field_type = None
            if self.match("COLON"):
                field_type = self.parse_type_expression(stop_tokens={"COMMA", "RBRACE"})
            fields.append((field_name, field_type))
            if not self.match("COMMA"):
                break

        end = self.expect("RBRACE", "record body")
        return set_span(Record(name_tok.value, fields), start_tok, end)

    def parse_actor(self, start_tok):
        name_tok = self.expect("IDENTIFIER", "actor name")
        self.expect("LBRACE", "actor body")
        methods = []

        while self.peek().type not in {"RBRACE", "EOF"}:
            if self.match_keyword("async"):
                if not self.match_keyword("function"):
                    self.error("Expected function", self.peek(), context="actor")
                methods.append(self.parse_function(is_async=True))
                self.match("SEMICOLON")
                continue
            if self.match_keyword("function"):
                methods.append(self.parse_function())
                self.match("SEMICOLON")
                continue
            self.error("Only functions are allowed in actor bodies", self.peek(), context="actor")
            self.advance()

        end = self.expect("RBRACE", "actor body")
        return set_span(Actor(name_tok.value, methods), start_tok, end)

    def parse_class(self, start_tok):
        name_tok = self.expect("IDENTIFIER", "class name")
        self.expect("LBRACE", "class body")
        methods = []

        while self.peek().type not in {"RBRACE", "EOF"}:
            if self.match_keyword("async"):
                if not self.match_keyword("function"):
                    self.error("Expected function", self.peek(), context="class")
                methods.append(self.parse_function(is_async=True))
                self.match("SEMICOLON")
                continue
            if self.match_keyword("function"):
                methods.append(self.parse_function())
                self.match("SEMICOLON")
                continue
            self.error("Only functions are allowed in class bodies", self.peek(), context="class")
            self.advance()

        end = self.expect("RBRACE", "class body")
        return set_span(Class(name_tok.value, methods), start_tok, end)

    def parse_import(self, start_tok):
        alias = None

        if self.peek().type == "STRING":
            target = self.expression(0)
            end = target.end
            if self.match_keyword("as"):
                alias_tok = self.expect("IDENTIFIER", "import alias")
                alias = alias_tok.value
                end = alias_tok
            return set_span(Import(target, alias=alias, is_path=True), start_tok, end)

        parts = [self.expect("IDENTIFIER", "module name")]
        while self.match("DOT"):
            parts.append(self.expect("IDENTIFIER", "module name"))

        end = parts[-1]
        if self.match_keyword("as"):
            alias_tok = self.expect("IDENTIFIER", "import alias")
            alias = alias_tok.value
            end = alias_tok

        module_name = ".".join(part.value for part in parts)
        return set_span(Import(module_name, alias=alias, is_path=False), start_tok, end)

    def parse_type_expression(self, stop_tokens=None):
        stop_tokens = stop_tokens or set()
        depth = 0
        parts = []

        while self.peek().type != "EOF":
            tok = self.peek()

            if depth == 0 and tok.type in stop_tokens:
                break

            if tok.type in {"IDENTIFIER", "COMMA"}:
                parts.append(tok.value)
                self.advance()
                continue

            if tok.type in {"LT", "LBRACKET", "LPAREN"}:
                parts.append(tok.value)
                depth += 1
                self.advance()
                continue

            if tok.type in {"GT", "RBRACKET", "RPAREN"}:
                if depth == 0:
                    break
                parts.append(tok.value)
                depth -= 1
                self.advance()
                continue

            break

        return "".join(parts).strip() or None

    # ------------------------
    # Match / Pattern
    # ------------------------

    def parse_match(self, start):
        expr = self.expression(0)
        self.expect("LBRACE")

        cases = []
        while self.peek().type != "RBRACE":
            pat = self.parse_pattern()
            self.expect("ARROW")
            body = self.expression(0)
            self.match("SEMICOLON")
            cases.append(Case(pat, body))

        end = self.expect("RBRACE")
        return set_span(Match(expr, cases), start, end)

    def parse_pattern(self):
        pat = self.parse_pattern_atom()

        if self.match_keyword("if"):
            guard = self.expression(1)
            pat = GuardPattern(pat, guard)

        while self.match("PIPE"):
            pat = OrPattern(pat, self.parse_pattern_atom())

        return pat

    def parse_pattern_atom(self):
        tok = self.advance()

        if tok.type == "WILDCARD":
            return WildcardPattern()

        if tok.type == "NUMBER":
            value = float(tok.value) if "." in tok.value else int(tok.value)
            return LiteralPattern(value)

        if tok.type == "STRING":
            return LiteralPattern(tok.value)

        if tok.type == "BOOLEAN":
            return LiteralPattern(tok.value == "true")

        if tok.type == "NULL":
            return LiteralPattern(None)

        if tok.type == "IDENTIFIER":
            if self.match("LPAREN"):
                args = []
                if self.peek().type != "RPAREN":
                    while True:
                        args.append(self.parse_pattern())
                        if not self.match("COMMA"):
                            break
                self.expect("RPAREN")
                return CallPattern(tok.value, args)

            if self.match("LBRACE"):
                props = {}
                while self.peek().type != "RBRACE":
                    key = self.expect("IDENTIFIER").value
                    self.expect("COLON")
                    props[key] = self.parse_pattern()
                    self.match("COMMA")
                self.expect("RBRACE")
                return ObjectPattern(tok.value, props)
            return IdentifierPattern(tok.value)

        if tok.type == "LBRACKET":
            items = []
            while self.peek().type != "RBRACKET":
                if self.match("SPREAD"):
                    if self.peek().type in {"COMMA", "RBRACKET"}:
                        items.append(Spread(WildcardPattern()))
                    else:
                        items.append(Spread(self.parse_pattern()))
                else:
                    items.append(self.parse_pattern())
                self.match("COMMA")
            self.expect("RBRACKET")
            return ArrayPattern(items)

        self.error("Invalid pattern", tok)
