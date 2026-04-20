# file: FoxLPy/semantic.py

import os
from typing import List, Dict, Optional, Set
from foxl.utils.diagnostics import Diagnostic, Span, suggest

from foxl.core.ast import (
    Node, Number, String, Identifier,
    BinaryOp, UnaryOp, Call, Member,
    Lambda, FunctionExpr, Pipeline,
    ArrayLiteral, ObjectLiteral, Spread,
    Match, Case,
    Block, If, While, Break, Continue, Return, Exit, Let, Function,
    Raise, TryCatch, Await, Record, Actor, Class, Import
)


# --- Nodes (kept for compatibility) ---

class VarDecl(Node):
    def __init__(self, name, value):
        super().__init__()
        self.name = name
        self.value = value


class Assign(Node):
    def __init__(self, target, value):
        super().__init__()
        self.target = target
        self.value = value


# --- Scope ---

class Scope:
    def __init__(self, parent=None):
        self.parent = parent
        self.symbols: Dict[str, Node] = {}

    def define(self, name, node):
        self.symbols[name] = node

    def exists_in_current(self, name):
        return name in self.symbols

    def resolve(self, name) -> Optional[Node]:
        if name in self.symbols:
            return self.symbols[name]
        if self.parent:
            return self.parent.resolve(name)
        return None

    def all_symbols(self) -> Set[str]:
        out = set(self.symbols.keys())
        if self.parent:
            out |= self.parent.all_symbols()
        return out


# --- Analyzer ---

class SemanticAnalyzer:
    MODULE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "modules"))

    def __init__(self, base_dir=None, module_cache=None):
        self.global_scope = Scope()
        self.scope = self.global_scope
        self.diagnostics: List[Diagnostic] = []
        self.base_dir = base_dir or os.getcwd()
        self.module_cache = module_cache if module_cache is not None else set()

        self.in_function = 0
        self.loop_depth = 0

        self.builtins = {
            "write", "read", "map", "filter", "str", "int", "float", "bool",
            "fetch", "spawn", "send", "Ok", "Err", "ValueError",
            "object", "number"
        }

    # --- utils ---

    def error(self, message, node, hint=None):
        start = getattr(node, "start", None)
        end = getattr(node, "end", None)
        if start is None or end is None:
            start = end = self._fallback_position()
        span = Span(start, end)
        self.diagnostics.append(Diagnostic("error", message, span, hint=hint))

    def warn(self, message, node):
        span = Span(node.start, node.end)
        self.diagnostics.append(Diagnostic("warning", message, span))

    def analyze(self, node: Node):
        self.visit(node)
        return self.diagnostics

    def _fallback_position(self):
        from foxl.utils.diagnostics import Position
        return Position(1, 1, 0)

    # --- visitor core ---

    def visit(self, node):
        if node is None:
            return

        method = getattr(self, f"visit_{type(node).__name__}", self.generic)
        return method(node)

    def generic(self, node):
        for v in vars(node).values():
            if isinstance(v, Node):
                self.visit(v)
            elif isinstance(v, list):
                for i in v:
                    if isinstance(i, Node):
                        self.visit(i)

    # --- literals ---

    def visit_Number(self, node): pass
    def visit_String(self, node): pass

    # --- identifiers ---

    def visit_Identifier(self, node):
        if self.scope.resolve(node.name):
            return

        if node.name in self.builtins:
            return

        suggestion = suggest(node.name, list(self.scope.all_symbols() | self.builtins))
        hint = f"did you mean '{suggestion}'?" if suggestion else None

        self.error(f"Undefined variable '{node.name}'", node, hint)

    # --- variables ---

    def visit_Let(self, node: Let):
        self.scope.define(node.name, node)

        if node.value:
            self.visit(node.value)

    def visit_VarDecl(self, node: VarDecl):
        self.scope.define(node.name, node)

        if node.value:
            self.visit(node.value)

    def visit_Assign(self, node: Assign):
        # 🔥 implicit declaration
        if isinstance(node.target, Identifier):
            if not self.scope.resolve(node.target.name):
                self.scope.define(node.target.name, node)

        elif isinstance(node.target, Member):
            self.visit(node.target.obj)
        else:
            self.error("Invalid assignment target", node.target)

        self.visit(node.value)

    # --- control flow ---

    def visit_Exit(self, node):
        if node.value:
            self.visit(node.value)

    def visit_While(self, node):
        self.visit(node.cond)

        self.loop_depth += 1
        self.visit(node.body)
        self.loop_depth -= 1

    def visit_Break(self, node):
        if self.loop_depth == 0:
            self.error(
                "Break outside of loop",
                node,
                hint="break can only be used inside a loop"
            )

    def visit_Continue(self, node):
        if self.loop_depth == 0:
            self.error(
                "Continue outside of loop",
                node,
                hint="continue can only be used inside a loop"
            )

    def visit_Return(self, node):
        if self.in_function == 0:
            self.error(
                "Return outside of function",
                node,
                hint="return can only be used inside a lambda/function"
            )

        if node.value:
            self.visit(node.value)

    def visit_If(self, node):
        self.visit(node.cond)
        self.visit(node.then)
        if node.otherwise:
            self.visit(node.otherwise)

    def visit_Block(self, node):
        prev = self.scope
        self.scope = Scope(prev)

        for stmt in node.statements:
            self.visit(stmt)

        self.scope = prev

    # --- functions ---

    def visit_Lambda(self, node):
        prev_scope = self.scope
        self.scope = Scope(prev_scope)

        self.in_function += 1

        self.scope.define(node.param, node)
        self.visit(node.body)

        self.in_function -= 1
        self.scope = prev_scope

    def visit_FunctionExpr(self, node):
        prev_scope = self.scope
        self.scope = Scope(prev_scope)

        self.in_function += 1

        for param_name, _ in node.params:
            self.scope.define(param_name, node)
        self.visit(node.body)

        self.in_function -= 1
        self.scope = prev_scope

    def visit_Function(self, node):
        # define function name first (hoisting-like)
        self.scope.define(node.name, node)

        prev_scope = self.scope
        self.scope = Scope(prev_scope)

        self.in_function += 1

        # params are local vars
        for param_name, _ in node.params:
            self.scope.define(param_name, node)

        self.visit(node.body)

        self.in_function -= 1
        self.scope = prev_scope

    def visit_Record(self, node):
        self.scope.define(node.name, node)

    def visit_Actor(self, node):
        self.scope.define(node.name, node)

        prev_scope = self.scope
        self.scope = Scope(prev_scope)
        self.scope.define("this", node)
        for method in node.methods:
            self.visit(method)
        self.scope = prev_scope

    def visit_Class(self, node):
        self.scope.define(node.name, node)

        prev_scope = self.scope
        self.scope = Scope(prev_scope)
        self.scope.define("this", node)
        for method in node.methods:
            self.visit(method)
        self.scope = prev_scope

    # --- expressions ---

    def visit_BinaryOp(self, node):
        self.visit(node.left)
        self.visit(node.right)

    def visit_UnaryOp(self, node):
        self.visit(node.expr)

    def visit_Call(self, node):
        self.visit(node.callee)
        for arg in node.args:
            self.visit(arg)

    def visit_Await(self, node):
        self.visit(node.expr)

    def visit_Import(self, node):
        spec = self._resolve_import_spec(node)
        if spec is None:
            return

        if spec["cache_key"] in self.module_cache:
            self._define_import_bindings(node, spec["module_name"], spec["export_names"], spec["namespace"])
            return

        self.module_cache.add(spec["cache_key"])

        export_names = set(spec["export_names"])
        if spec.get("source_path"):
            try:
                with open(spec["source_path"], "r", encoding="utf-8") as handle:
                    source = handle.read()
            except FileNotFoundError:
                self.error(f"Imported file not found '{spec['source_path']}'", node)
                return

            from foxl.core.lexer import Lexer
            from foxl.core.parser import Parser

            parser = Parser(Lexer(source).tokenize(), source)
            imported_ast = parser.parse()
            self.diagnostics.extend(parser.diagnostics)

            previous_base_dir = self.base_dir
            self.base_dir = os.path.dirname(spec["source_path"])
            try:
                export_names.update(self._analyze_imported_exports(imported_ast))
            finally:
                self.base_dir = previous_base_dir

        self._define_import_bindings(node, spec["module_name"], export_names, spec["namespace"])

    def _resolve_import_spec(self, node):
        if node.is_path:
            if isinstance(node.target, String):
                resolved = node.target.value
            else:
                return None

            if not os.path.isabs(resolved):
                resolved = os.path.join(self.base_dir, resolved)
            if not resolved.endswith(".foxl"):
                resolved += ".foxl"
            resolved = os.path.abspath(resolved)
            return {
                "cache_key": resolved,
                "module_name": os.path.splitext(os.path.basename(resolved))[0],
                "source_path": resolved,
                "namespace": False,
                "export_names": set(),
            }

        module_dir = os.path.join(self.MODULE_ROOT, node.target.replace(".", os.sep))
        conf_path = os.path.join(module_dir, "__conf__.foxl")
        if not os.path.exists(conf_path):
            self.error(f"Module not found '{node.target}'", node)
            return None

        conf = self._read_module_conf(conf_path)
        source_name = conf.get("source", "module.foxl")
        native_exports = conf.get("native_exports", {})
        export_names = set(conf.get("exports", [])) | set(native_exports.keys())

        return {
            "cache_key": os.path.abspath(conf_path),
            "module_name": conf.get("name", node.target.split(".")[-1]),
            "source_path": os.path.abspath(os.path.join(module_dir, source_name)) if source_name else None,
            "namespace": bool(conf.get("namespace", False)),
            "export_names": export_names,
        }

    def _read_module_conf(self, conf_path):
        try:
            with open(conf_path, "r", encoding="utf-8") as handle:
                source = handle.read()
        except FileNotFoundError:
            return {}

        from foxl.core.lexer import Lexer
        from foxl.core.parser import Parser

        parser = Parser(Lexer(source).tokenize(), source)
        ast = parser.parse()
        self.diagnostics.extend(parser.diagnostics)

        conf = {}
        if isinstance(ast, Block):
            statements = ast.statements
        else:
            statements = [ast]

        for stmt in statements:
            if isinstance(stmt, Let):
                literal = self._extract_literal(stmt.value)
                if literal is not None:
                    conf[stmt.name] = literal
        return conf

    def _extract_literal(self, node):
        if isinstance(node, String):
            return node.value
        if isinstance(node, Number):
            return node.value
        if type(node).__name__ == "Boolean":
            return node.value
        if type(node).__name__ == "Null":
            return None
        if isinstance(node, ArrayLiteral):
            values = []
            for item in node.items:
                if isinstance(item, Spread):
                    return None
                value = self._extract_literal(item)
                if value is None and type(item).__name__ != "Null":
                    return None
                values.append(value)
            return values
        if isinstance(node, ObjectLiteral):
            values = {}
            for prop in node.props:
                if isinstance(prop, Spread):
                    return None
                key, value_node = prop
                value = self._extract_literal(value_node)
                if value is None and type(value_node).__name__ != "Null":
                    return None
                values[key] = value
            return values
        return None

    def _analyze_imported_exports(self, imported_ast):
        export_names = set()
        statements = imported_ast.statements if isinstance(imported_ast, Block) else [imported_ast]
        for stmt in statements:
            if isinstance(stmt, (Let, Function, Class, Record, Actor)):
                export_names.add(stmt.name)
            self.visit(stmt)
        return export_names

    def _define_import_bindings(self, node, module_name, export_names, namespace):
        if node.alias or namespace:
            self.scope.define(node.alias or module_name, node)
            return

        for export_name in export_names:
            self.scope.define(export_name, node)

    def visit_Member(self, node):
        self.visit(node.obj)

    def visit_Pipeline(self, node):
        self.visit(node.left)
        self.visit(node.right)

    # --- data ---

    def visit_ArrayLiteral(self, node):
        for i in node.items:
            self.visit(i)

    def visit_ObjectLiteral(self, node):
        for _, v in node.props:
            self.visit(v)

    def visit_Spread(self, node):
        self.visit(node.expr)

    # --- match ---

    def visit_Match(self, node):
        self.visit(node.expr)
        for case in node.cases:
            self.visit(case)

    def visit_Case(self, node):
        prev_scope = self.scope
        self.scope = Scope(prev_scope)
        self._bind_pattern_names(node.pattern)
        self.visit(node.expr)
        self.scope = prev_scope

    def _bind_pattern_names(self, pattern):
        if pattern is None:
            return

        pattern_name = type(pattern).__name__

        if pattern_name == "IdentifierPattern":
            self.scope.define(pattern.name, pattern)
            return

        if pattern_name == "OrPattern":
            self._bind_pattern_names(pattern.left)
            self._bind_pattern_names(pattern.right)
            return

        if pattern_name == "GuardPattern":
            self._bind_pattern_names(pattern.pattern)
            self.visit(pattern.guard)
            return

        if pattern_name == "ArrayPattern":
            for item in pattern.items:
                if isinstance(item, Spread):
                    self._bind_pattern_names(item.expr)
                else:
                    self._bind_pattern_names(item)
            return

        if pattern_name == "ObjectPattern":
            for child in pattern.props.values():
                self._bind_pattern_names(child)
            return

        if pattern_name == "CallPattern":
            for arg in pattern.args:
                self._bind_pattern_names(arg)
            return

    def visit_Raise(self, node):
        self.visit(node.value)

    def visit_TryCatch(self, node):
        self.visit(node.try_block)

        prev_scope = self.scope
        self.scope = Scope(prev_scope)
        self.scope.define(node.error_name, node)
        self.visit(node.catch_block)
        self.scope = prev_scope
