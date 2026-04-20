# file: interpreter.py

import json
import math
import os
import random
import shutil
import subprocess
import ctypes
import urllib.error
import urllib.parse
import urllib.request
from typing import Any
from foxl.utils.errors import FoxLRuntimeError

from foxl.core.ast import (
    Number, String, Identifier, BinaryOp, UnaryOp,
    Call, Member, Lambda, FunctionExpr, Pipeline,
    ArrayLiteral, ObjectLiteral, Spread,
    Match, Case,
    Raise, TryCatch, Await, Record, Actor, Class, Import,
    Block, If, While, Break, Continue, Return, Exit,
    Let
)


# --- Control Flow Signals ---

class ReturnSignal(Exception):
    def __init__(self, value): self.value = value


class BreakSignal(Exception): pass


class ContinueSignal(Exception): pass


class ExitSignal(Exception):
    def __init__(self, value): self.value = value


class FoxLThrown(Exception):
    def __init__(self, value):
        self.value = value


# --- Environment ---

class Env:
    def __init__(self, parent=None):
        self.parent = parent
        self.values = {}
        self.types = {}

    def define(self, name, value, type_name=None):
        self.values[name] = value
        if type_name is not None:
            self.types[name] = type_name

    def set(self, name, value):
        if name in self.values:
            self.values[name] = value
        elif self.parent:
            self.parent.set(name, value)
        else:
            self.values[name] = value

    def get(self, name):
        if name in self.values:
            return self.values[name]
        if self.parent:
            return self.parent.get(name)
        raise RuntimeError(f"Undefined variable '{name}'")

    def get_type(self, name):
        if name in self.types:
            return self.types[name]
        if self.parent:
            return self.parent.get_type(name)
        return None


# --- Interpreter ---

class Interpreter:
    MODULE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "modules"))

    def __init__(self, base_dir=None, module_cache=None, limits=None, cli_args=None):
        self.global_env = Env()
        self.records = {}
        self.actors = {}
        self.classes = {}
        self.base_dir = base_dir or os.getcwd()
        self.cli_args = list(cli_args or [])
        self.module_cache = module_cache if module_cache is not None else {}
        self.limits = {
            "max_steps": 200_000,
            "max_call_depth": 120,
            "max_collection_size": 20_000,
            "max_string_length": 1_000_000,
            "max_modules": 256,
        }
        if limits:
            self.limits.update(limits)
        self.steps = 0
        self.call_depth = 0
        self._install_builtins()

    @staticmethod
    def _format_output(value):
        if value is None:
            return "null"
        if isinstance(value, bool):
            return "true" if value else "false"
        if isinstance(value, (int, float)):
            return str(value)
        if isinstance(value, str):
            return value
        if isinstance(value, list):
            return "[" + ", ".join(map(Interpreter._format_output, value)) + "]"
        if isinstance(value, dict):
            return "{" + ", ".join(
                f"{k}: {Interpreter._format_output(v)}"
                for k, v in value.items()
            ) + "}"
        return value

    @staticmethod
    def _write(*args):
        if not args:
            print()
            return
        for a in args:
            print(Interpreter._format_output(a))

    @staticmethod
    def _read(prompt=""):
        try:
            return input(prompt)
        except EOFError:
            return ""

    def _truthy(self, value):
        if value is None:
            return False
        if value is False:
            return False
        if value == 0:
            return False
        if value == "":
            return False
        if value == []:
            return False
        if value == {}:
            return False
        return True

    def _install_builtins(self):
        self.global_env.define("write", self._write)
        self.global_env.define("read", self._read)
        self.global_env.define("map", lambda values, fn: [fn(v) for v in values])
        self.global_env.define("filter", lambda values, fn: [v for v in values if self._truthy(fn(v))])
        self.global_env.define("str", self._stringify)
        self.global_env.define("int", self._to_int)
        self.global_env.define("float", self._to_float)
        self.global_env.define("bool", lambda value: self._truthy(value))
        self.global_env.define("fetch", self._fetch)
        self.global_env.define("spawn", self._spawn_actor)
        self.global_env.define("send", self._send_actor_message)
        self.global_env.define("Ok", lambda value: {"__tag__": "Ok", "args": [value]})
        self.global_env.define("Err", lambda value: {"__tag__": "Err", "args": [value]})
        self.global_env.define("ValueError", lambda message: {"__tag__": "ValueError", "args": [message]})
        self.global_env.define("object", {"placeholder": True})
        self.global_env.define("number", 1.5)

    def _stringify(self, value):
        if value is None:
            return "null"
        if isinstance(value, bool):
            return "true" if value else "false"
        return str(value)

    def _to_int(self, value):
        try:
            return int(value)
        except (TypeError, ValueError):
            raise FoxLThrown({"__tag__": "ValueError", "args": [f"Invalid integer: {value}"]})

    def _to_float(self, value):
        try:
            return float(value)
        except (TypeError, ValueError):
            raise FoxLThrown({"__tag__": "ValueError", "args": [f"Invalid float: {value}"]})

    def _write_file(self, path, content, append):
        if path == "path":
            return None
        mode = "a" if append else "w"
        with open(path, mode, encoding="utf-8") as handle:
            handle.write(content)
        return None

    def _run_process(self, command, args=None, capture=False):
        if command == "command":
            class PlaceholderResult:
                returncode = 0
                stdout = ""
                stderr = ""
            return PlaceholderResult()

        args = args or []
        return subprocess.run(
            [command, *args],
            check=False,
            capture_output=capture,
            text=True,
        )

    def _sys_run(self, command, args=None):
        return self._run_process(command, args=args, capture=True).returncode

    def _sys_capture(self, command, args=None):
        return self._run_process(command, args=args, capture=True).stdout

    def _fs_exists(self, path):
        if path == "path":
            return False
        return os.path.exists(path)

    def _fs_read(self, path):
        if path == "path":
            return ""
        try:
            return open(path, "r", encoding="utf-8").read()
        except FileNotFoundError:
            return ""

    def _fs_delete(self, path):
        if path == "path" or not os.path.exists(path):
            return None
        os.remove(path)
        return None

    def _fs_mkdir(self, path):
        if path == "path":
            return None
        os.makedirs(path, exist_ok=True)
        return None

    def _fs_rmdir(self, path):
        if path == "path" or not os.path.exists(path):
            return None
        shutil.rmtree(path)
        return None

    def _fs_list(self, path):
        if path == "path" or not os.path.exists(path):
            return []
        return os.listdir(path)

    def _json_parse(self, value):
        if value == "string":
            return value
        try:
            return json.loads(value)
        except json.JSONDecodeError:
            return None

    def _spawn_actor(self, actor_def):
        if callable(actor_def):
            actor_def = actor_def()
        if isinstance(actor_def, dict) and actor_def.get("__actor__"):
            return actor_def
        raise RuntimeError("spawn expects an actor definition")

    def _send_actor_message(self, actor, method_name, *args):
        methods = actor.get("methods", {})
        fn = methods.get(method_name)
        if fn is None:
            raise RuntimeError(f"Unknown actor method '{method_name}'")
        return fn(*args)

    def _fetch(self, url, options=None):
        options = options or {}
        method = options.get("method", "GET")
        headers = options.get("headers", {})
        timeout = options.get("timeout", 10)
        data = options.get("body")
        json_body = options.get("json")

        if json_body is not None:
            data = json.dumps(json_body).encode("utf-8")
            headers = {**headers, "Content-Type": "application/json"}
        elif isinstance(data, str):
            data = data.encode("utf-8")

        request = urllib.request.Request(url, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(request, timeout=timeout) as response:
                body = response.read().decode("utf-8")
                return body
        except urllib.error.HTTPError as error:
            raise FoxLThrown({
                "__tag__": "HttpError",
                "args": [error.code, error.read().decode("utf-8", errors="replace")],
            })
        except urllib.error.URLError as error:
            raise FoxLThrown({
                "__tag__": "NetworkError",
                "args": [str(error.reason)],
            })

    def _guard_step(self, node=None):
        self.steps += 1
        if self.steps > self.limits["max_steps"]:
            raise FoxLRuntimeError("Execution step limit exceeded", node)

    def _guard_collection(self, value, node=None):
        if isinstance(value, (list, dict)) and len(value) > self.limits["max_collection_size"]:
            raise FoxLRuntimeError("Collection size limit exceeded", node)
        if isinstance(value, str) and len(value) > self.limits["max_string_length"]:
            raise FoxLRuntimeError("String size limit exceeded", node)
        return value

    def _guard_call_depth(self, node=None):
        self.call_depth += 1
        if self.call_depth > self.limits["max_call_depth"]:
            self.call_depth -= 1
            raise FoxLRuntimeError("Call depth limit exceeded", node)

    def _resolve_import_request(self, node, env):
        if node.is_path:
            raw_path = self.eval(node.target, env)
            if not isinstance(raw_path, str):
                raise RuntimeError("import expects a string path")
            resolved = raw_path
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
                "exports": None,
                "native_path": None,
                "native_exports": {},
            }

        module_dir = os.path.join(self.MODULE_ROOT, node.target.replace(".", os.sep))
        conf_path = os.path.join(module_dir, "__conf__.foxl")
        if not os.path.exists(conf_path):
            raise RuntimeError(f"Module '{node.target}' not found")

        conf = self._load_module_config(conf_path)
        source_name = conf.get("source", "module.foxl")
        native_name = conf.get("native")
        return {
            "cache_key": os.path.abspath(conf_path),
            "module_name": conf.get("name", node.target.split(".")[-1]),
            "source_path": os.path.abspath(os.path.join(module_dir, source_name)) if source_name else None,
            "native_path": os.path.abspath(os.path.join(module_dir, native_name)) if native_name else None,
            "builtin": conf.get("builtin"),
            "namespace": bool(conf.get("namespace", False)),
            "exports": conf.get("exports"),
            "native_exports": conf.get("native_exports", {}),
        }

    def _load_module_config(self, conf_path):
        if len(self.module_cache) > self.limits["max_modules"]:
            raise RuntimeError("Module limit exceeded")

        with open(conf_path, "r", encoding="utf-8") as handle:
            source = handle.read()

        from foxl.core.lexer import Lexer
        from foxl.core.parser import Parser
        from foxl.core.semantic import SemanticAnalyzer

        parser = Parser(Lexer(source).tokenize(), source)
        ast = parser.parse()
        diagnostics = list(parser.diagnostics)
        diagnostics.extend(SemanticAnalyzer(base_dir=os.path.dirname(conf_path)).analyze(ast))
        if diagnostics:
            raise RuntimeError(f"Invalid module config '{conf_path}'")

        conf_env = Env(self.global_env)
        previous_base_dir = self.base_dir
        self.base_dir = os.path.dirname(conf_path)
        try:
            self.eval_program(ast, conf_env)
        finally:
            self.base_dir = previous_base_dir

        return dict(conf_env.values)

    def _collect_module_exports(self, module_env, export_names):
        if export_names is None:
            export_names = [
                name for name in module_env.values.keys()
                if not name.startswith("__")
            ]
        return {
            name: module_env.values[name]
            for name in export_names
            if name in module_env.values
        }

    def _ctypes_type(self, name):
        return {
            "int": ctypes.c_int,
            "float": ctypes.c_double,
            "bool": ctypes.c_bool,
            "string": ctypes.c_char_p,
            "void": None,
        }.get(name, ctypes.c_int)

    def _load_native_module(self, native_path, export_signatures):
        if not native_path or not os.path.exists(native_path):
            return {}

        library = ctypes.CDLL(native_path)
        exports = {}
        for name, signature in (export_signatures or {}).items():
            fn = getattr(library, name)
            fn.argtypes = [self._ctypes_type(arg) for arg in signature.get("args", [])]
            fn.restype = self._ctypes_type(signature.get("returns", "int"))

            def make_native(bound_fn):
                return lambda *args: bound_fn(*args)

            exports[name] = make_native(fn)
        return exports

    def _load_builtin_module(self, name):
        modules = {
            "fs": {
                "exists": self._fs_exists,
                "read": self._fs_read,
                "write": lambda path, content: self._write_file(path, content, append=False),
                "append": lambda path, content: self._write_file(path, content, append=True),
                "delete": self._fs_delete,
                "mkdir": self._fs_mkdir,
                "rmdir": self._fs_rmdir,
                "list": self._fs_list,
            },
            "json": {
                "parse": self._json_parse,
                "stringify": lambda value: json.dumps(value),
            },
            "num": {
                "random": lambda: random.random(),
                "floor": lambda value: math.floor(value),
                "ceil": lambda value: math.ceil(value),
            },
            "sys": {
                "run": self._sys_run,
                "capture": self._sys_capture,
            },
            "http": {
                "fetch": self._fetch,
                "get": lambda url, options=None: self._fetch(url, {**(options or {}), "method": "GET"}),
                "post": lambda url, body="", options=None: self._fetch(url, {**(options or {}), "method": "POST", "body": body}),
                "getJson": lambda url, options=None: self._json_parse(self._fetch(url, {**(options or {}), "method": "GET"})),
            },
            "cli": {
                "run": self._cli_run,
                "capture": self._sys_capture,
                "which": lambda name: shutil.which(name),
                "args": lambda: list(self.cli_args),
                "cwd": lambda: self.base_dir,
                "env": lambda name=None: dict(os.environ) if name is None else os.environ.get(name),
            },
        }
        return dict(modules.get(name, {}))

    def _cli_run(self, command, args=None):
        result = self._run_process(command, args=args, capture=True)
        return {
            "code": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }

    def _bind_import(self, env, node, module_name, exports, namespace):
        if node.alias or namespace:
            binding_name = node.alias or module_name
            module_object = dict(exports)
            env.define(binding_name, module_object)
            return module_object

        for name, value in exports.items():
            env.define(name, value)
        return dict(exports)

    def _normalize_type_name(self, type_name):
        return (type_name or "").replace(" ", "")

    def _matches_type(self, value, type_name):
        type_name = self._normalize_type_name(type_name)
        if not type_name or type_name == "Any":
            return True
        if type_name in {"String", "str"}:
            return isinstance(value, str)
        if type_name in {"Number", "int", "float"}:
            return isinstance(value, (int, float)) and not isinstance(value, bool)
        if type_name in {"Boolean", "Bool", "bool"}:
            return isinstance(value, bool)
        if type_name in {"Null", "null"}:
            return value is None
        if type_name in {"Function", "fn"}:
            return callable(value)

        if type_name.startswith("[") and type_name.endswith("]"):
            if not isinstance(value, list):
                return False
            inner = type_name[1:-1] or "Any"
            return all(self._matches_type(item, inner) for item in value)

        if type_name.startswith("Array<") and type_name.endswith(">"):
            if not isinstance(value, list):
                return False
            inner = type_name[6:-1] or "Any"
            return all(self._matches_type(item, inner) for item in value)

        if type_name.startswith("Result<") and type_name.endswith(">"):
            return isinstance(value, dict) and value.get("__tag__") in {"Ok", "Err"}

        if isinstance(value, dict):
            return value.get("__record__") == type_name or value.get("__class__") == type_name

        return False

    def _assert_type(self, value, type_name, node=None, binding_name=None):
        if not type_name:
            return value
        if not self._matches_type(value, type_name):
            label = binding_name or "value"
            raise FoxLRuntimeError(f"Type mismatch for {label}: expected {type_name}", node)
        return value

    def eval(self, node, env=None):
        if env is None:
            env = self.global_env

        self._guard_step(node)

        method_name = f"eval_{type(node).__name__}"
        method = getattr(self, method_name, None)

        if not method:
            raise RuntimeError(
                f"[FoxL Runtime] Missing evaluator: {method_name}"
            )

        return method(node, env)

    def eval_program(self, node, env=None):
        if env is None:
            env = self.global_env

        if isinstance(node, Block):
            result = None
            for stmt in node.statements:
                result = self.eval(stmt, env)
            return result

        return self.eval(node, env)

    # --- Literals ---

    def eval_Number(self, node, env): return node.value
    def eval_String(self, node, env): return node.value

    def eval_Identifier(self, node, env):
        return env.get(node.name)

    # --- Variables ---

    def eval_Let(self, node, env):
        val = self.eval(node.value, env) if node.value else None
        self._assert_type(val, node.type_name, node, node.name)
        env.define(node.name, val, type_name=node.type_name)
        return val

    def eval_VarDecl(self, node, env):
        val = self.eval(node.value, env) if node.value else None
        env.define(node.name, val)
        return val

    def eval_Assign(self, node, env):
        val = self.eval(node.value, env)

        if hasattr(node.target, "name"):
            type_name = env.get_type(node.target.name)
            self._assert_type(val, type_name, node, node.target.name)
            env.set(node.target.name, val)
        else:
            obj = self.eval(node.target.obj, env)
            obj[node.target.prop] = val

        return val

    # --- Ops ---

    def eval_BinaryOp(self, node, env):
        op = node.op

        if op == "&&":
            l = self.eval(node.left, env)
            return self.eval(node.right, env) if self._truthy(l) else l

        if op == "||":
            l = self.eval(node.left, env)
            return l if self._truthy(l) else self.eval(node.right, env)

        l = self.eval(node.left, env)
        r = self.eval(node.right, env)

        if op == "+":
            if isinstance(l, (int, float)) and isinstance(r, (int, float)):
                return l + r
            return self._guard_collection(self._stringify(l) + self._stringify(r), node)

        if op in {"-", "*", "/", "^"}:
            if not isinstance(l, (int, float)) or not isinstance(r, (int, float)):
                raise FoxLRuntimeError(
                    f"Operator '{op}' requires numbers",
                    node
                )
            return {
                "-": l - r,
                "*": l * r,
                "/": l / r,
                "^": l ** r,
            }[op]

        if op == "==":
            return self._loose_equal(l, r)
        if op == "!=":
            return not self._loose_equal(l, r)
        if op == "===":
            return type(l) == type(r) and l == r
        if op == ">":
            return l > r
        if op == "<":
            return l < r
        if op == ">=":
            return l >= r
        if op == "<=":
            return l <= r

        raise RuntimeError(f"Unsupported operator '{op}'")

    def _loose_equal(self, left, right):
        if type(left) == type(right):
            return left == right

        pairs = ((left, right), (right, left))
        for first, second in pairs:
            if isinstance(first, (int, float)) and isinstance(second, str):
                try:
                    converted = float(second) if "." in second else int(second)
                except ValueError:
                    continue
                return first == converted

            if isinstance(first, bool):
                return self._loose_equal(int(first), second)

        return left == right

    def eval_UnaryOp(self, node, env):
        v = self.eval(node.expr, env)

        if node.op == "+":
            if not isinstance(v, (int, float)):
                raise FoxLRuntimeError("Unary '+' expects number", node)
            return +v

        if node.op == "-":
            if not isinstance(v, (int, float)):
                raise FoxLRuntimeError("Unary '-' expects number", node)
            return -v

        if node.op == "!":
            return not self._truthy(v)

    # --- Control ---

    def eval_Block(self, node, env):
        local = Env(env)
        result = None

        for stmt in node.statements:
            result = self.eval(stmt, local)

        return result

    def eval_If(self, node, env):
        if self._truthy(self.eval(node.cond, env)):
            return self.eval(node.then, env)
        return self.eval(node.otherwise, env) if node.otherwise else None

    def eval_While(self, node, env):
        while self._truthy(self.eval(node.cond, env)):
            try:
                self.eval(node.body, env)
            except BreakSignal:
                break
            except ContinueSignal:
                continue

    def eval_Break(self, node, env):
        raise BreakSignal()

    def eval_Continue(self, node, env):
        raise ContinueSignal()

    def eval_Return(self, node, env):
        value = self.eval(node.value, env) if node.value else None
        raise ReturnSignal(value)

    def eval_Exit(self, node, env):
        value = self.eval(node.value, env) if node.value else None
        raise ExitSignal(value)

    # --- Functions ---

    def eval_Lambda(self, node, env):
        def fn(arg):
            self._guard_call_depth(node)
            local = Env(env)
            local.define(node.param, arg)

            try:
                return self.eval(node.body, local)
            except ReturnSignal as r:
                return r.value
            finally:
                self.call_depth -= 1

        return fn

    def _make_function(self, params, body, env, name=None, bound_this=None, return_type=None):
        def fn(*args):
            self._guard_call_depth(body)
            if len(args) != len(params):
                self.call_depth -= 1
                raise RuntimeError("Argument count mismatch")

            local = Env(env)

            if bound_this is not None:
                local.define("this", bound_this)

            if name:
                local.define(name, fn)

            for (param_name, param_type), value in zip(params, args):
                self._assert_type(value, param_type, body, param_name)
                local.define(param_name, value, type_name=param_type)

            try:
                result = self.eval(body, local)
                return self._assert_type(result, return_type, body, name or "return")
            except ReturnSignal as r:
                return self._assert_type(r.value, return_type, body, name or "return")
            finally:
                self.call_depth -= 1

        return fn

    def eval_FunctionExpr(self, node, env):
        return self._make_function(node.params, node.body, env, return_type=node.return_type)

    def eval_Call(self, node, env):
        fn = self.eval(node.callee, env)

        if not callable(fn):
            raise RuntimeError(f"'{node.callee}' is not callable")

        args = [self.eval(a, env) for a in node.args]
        return fn(*args)

    # --- Data ---

    def eval_ArrayLiteral(self, node, env):
        items = []
        for item in node.items:
            if isinstance(item, Spread):
                items.extend(self.eval(item.expr, env))
            else:
                items.append(self.eval(item, env))
        return self._guard_collection(items, node)

    def eval_ObjectLiteral(self, node, env):
        obj = {}
        for prop in node.props:
            if isinstance(prop, Spread):
                obj.update(self.eval(prop.expr, env))
                continue

            k, v = prop
            obj[k] = self.eval(v, env)
        return self._guard_collection(obj, node)

    def eval_Member(self, node, env):
        obj = self.eval(node.obj, env)
        return obj[node.prop]
    
    def eval_Boolean(self, node, env):
        return node.value
    
    def eval_Null(self, node, env):
        return None
    
    def eval_ErrorNode(self, node, env):
        return None

    def eval_Raise(self, node, env):
        raise FoxLThrown(self.eval(node.value, env))

    def eval_TryCatch(self, node, env):
        try:
            return self.eval(node.try_block, env)
        except FoxLThrown as thrown:
            local = Env(env)
            local.define(node.error_name, thrown.value)
            return self.eval(node.catch_block, local)

    def eval_Await(self, node, env):
        return self.eval(node.expr, env)

    def eval_Record(self, node, env):
        def constructor(*args, **kwargs):
            record = {"__record__": node.name}

            for index, (field_name, _) in enumerate(node.fields):
                if index < len(args):
                    record[field_name] = args[index]

            for field_name, _ in node.fields:
                if field_name in kwargs:
                    record[field_name] = kwargs[field_name]

            return record

        self.records[node.name] = node.fields
        env.define(node.name, constructor)
        return constructor

    def eval_Actor(self, node, env):
        actor_env = Env(env)
        methods = {}

        for method_node in node.methods:
            fn = self.eval_Function(method_node, actor_env)
            methods[method_node.name] = fn

        actor = {"__actor__": node.name, "methods": methods}
        self.actors[node.name] = actor
        env.define(node.name, lambda: actor)
        return actor

    def eval_Class(self, node, env):
        def constructor(*args):
            instance = {"__class__": node.name}

            for method_node in node.methods:
                instance[method_node.name] = self._make_function(
                    method_node.params,
                    method_node.body,
                    env,
                    name=method_node.name,
                    bound_this=instance,
                    return_type=method_node.return_type,
                )

            init = instance.get("init")
            if init is not None:
                init(*args)
            elif args:
                raise RuntimeError("Constructor arguments provided but no init method exists")

            return instance

        self.classes[node.name] = node
        env.define(node.name, constructor)
        return constructor
    
    def eval_Function(self, node, env):
        fn = self._make_function(node.params, node.body, env, name=node.name, return_type=node.return_type)
        env.define(node.name, fn)
        return fn

    def eval_Import(self, node, env):
        try:
            spec = self._resolve_import_request(node, env)
        except RuntimeError as exc:
            raise FoxLRuntimeError(str(exc), node)
        cache_key = spec["cache_key"]

        if cache_key in self.module_cache:
            return self._bind_import(env, node, spec["module_name"], self.module_cache[cache_key], spec["namespace"])

        exports = {}

        if spec.get("builtin"):
            exports.update(self._load_builtin_module(spec["builtin"]))

        if spec.get("source_path"):
            with open(spec["source_path"], "r", encoding="utf-8") as handle:
                source = handle.read()

            from foxl.core.lexer import Lexer
            from foxl.core.parser import Parser
            from foxl.core.semantic import SemanticAnalyzer

            parser = Parser(Lexer(source).tokenize(), source)
            ast = parser.parse()
            diagnostics = list(parser.diagnostics)
            diagnostics.extend(SemanticAnalyzer(base_dir=os.path.dirname(spec["source_path"])).analyze(ast))
            if diagnostics:
                raise FoxLRuntimeError(f"Import failed for '{spec['module_name']}'", node)

            module_env = Env(self.global_env)
            previous_base_dir = self.base_dir
            self.base_dir = os.path.dirname(spec["source_path"])
            try:
                self.eval_program(ast, module_env)
            finally:
                self.base_dir = previous_base_dir

            exports.update(self._collect_module_exports(module_env, spec.get("exports")))

        exports.update(self._load_native_module(spec.get("native_path"), spec.get("native_exports")))
        exports = self._guard_collection(exports, node)
        self.module_cache[cache_key] = exports
        return self._bind_import(env, node, spec["module_name"], exports, spec["namespace"])

    def eval_Pipeline(self, node, env):
        left = self.eval(node.left, env)

        if isinstance(node.right, Call):
            fn = self.eval(node.right.callee, env)
            args = [left, *[self.eval(arg, env) for arg in node.right.args]]
            return fn(*args)

        fn = self.eval(node.right, env)
        if not callable(fn):
            raise RuntimeError("Pipeline target must be callable")

        return fn(left)

    def eval_Match(self, node, env):
        value = self.eval(node.expr, env)

        for case in node.cases:
            matched, bindings = self._match_pattern(case.pattern, value, env)
            if matched:
                local = Env(env)
                for name, bound_value in bindings.items():
                    local.define(name, bound_value)
                return self.eval(case.expr, local)

        return None

    def _match_pattern(self, pattern, value, env):
        name = type(pattern).__name__

        if name == "WildcardPattern":
            return True, {}

        if name == "LiteralPattern":
            literal = pattern.value
            return literal == value, {}

        if name == "IdentifierPattern":
            return True, {pattern.name: value}

        if name == "OrPattern":
            matched, bindings = self._match_pattern(pattern.left, value, env)
            if matched:
                return matched, bindings
            return self._match_pattern(pattern.right, value, env)

        if name == "GuardPattern":
            matched, bindings = self._match_pattern(pattern.pattern, value, env)
            if not matched:
                return False, {}

            local = Env(env)
            for binding_name, binding_value in bindings.items():
                local.define(binding_name, binding_value)
            return self._truthy(self.eval(pattern.guard, local)), bindings

        if name == "ArrayPattern":
            if not isinstance(value, list):
                return False, {}

            if not any(isinstance(item, Spread) for item in pattern.items):
                if len(pattern.items) != len(value):
                    return False, {}

                bindings = {}
                for item_pattern, item_value in zip(pattern.items, value):
                    matched, item_bindings = self._match_pattern(item_pattern, item_value, env)
                    if not matched:
                        return False, {}
                    bindings.update(item_bindings)
                return True, bindings

            spread_index = next(i for i, item in enumerate(pattern.items) if isinstance(item, Spread))
            prefix = pattern.items[:spread_index]
            suffix = pattern.items[spread_index + 1:]

            if len(value) < len(prefix) + len(suffix):
                return False, {}

            bindings = {}

            for item_pattern, item_value in zip(prefix, value[:len(prefix)]):
                matched, item_bindings = self._match_pattern(item_pattern, item_value, env)
                if not matched:
                    return False, {}
                bindings.update(item_bindings)

            tail_values = value[len(value) - len(suffix):] if suffix else []
            for item_pattern, item_value in zip(suffix, tail_values):
                matched, item_bindings = self._match_pattern(item_pattern, item_value, env)
                if not matched:
                    return False, {}
                bindings.update(item_bindings)

            spread_pattern = pattern.items[spread_index].expr
            if type(spread_pattern).__name__ == "IdentifierPattern":
                bindings[spread_pattern.name] = value[len(prefix):len(value) - len(suffix) if suffix else len(value)]

            return True, bindings

        if name == "ObjectPattern":
            if not isinstance(value, dict):
                return False, {}

            tag_name = value.get("__record__") or value.get("__tag__")
            if pattern.name and tag_name not in {pattern.name, None}:
                return False, {}

            bindings = {}
            for key, prop_pattern in pattern.props.items():
                if key not in value:
                    return False, {}
                matched, prop_bindings = self._match_pattern(prop_pattern, value[key], env)
                if not matched:
                    return False, {}
                bindings.update(prop_bindings)
            return True, bindings

        if name == "CallPattern":
            if not isinstance(value, dict):
                return False, {}

            tag_name = value.get("__tag__") or value.get("__record__")
            if tag_name != pattern.name:
                return False, {}

            args = value.get("args", [])
            if len(args) != len(pattern.args):
                return False, {}

            bindings = {}
            for arg_pattern, arg_value in zip(pattern.args, args):
                matched, arg_bindings = self._match_pattern(arg_pattern, arg_value, env)
                if not matched:
                    return False, {}
                bindings.update(arg_bindings)

            return True, bindings

        return False, {}
