class Node:
    def __init__(self):
        self.start = None
        self.end = None

def set_span(node, start, end):
    node.start = start
    node.end = end
    return node

class Number(Node):
    def __init__(self, value): self.value = value

class Null(Node):
    def __init__(self):
        pass

class String(Node):
    def __init__(self, value): self.value = value

class Boolean(Node):
    def __init__(self, value): self.value = value

class Identifier(Node):
    def __init__(self, name): self.name = name

class BinaryOp(Node):
    def __init__(self, left, op, right):
        self.left, self.op, self.right = left, op, right

class UnaryOp(Node):
    def __init__(self, op, expr):
        self.op, self.expr = op, expr

class Call(Node):
    def __init__(self, callee, args):
        super().__init__()
        self.callee, self.args = callee, args

class Member(Node):
    def __init__(self, obj, prop):
        super().__init__()
        self.obj, self.prop = obj, prop

class Lambda(Node):
    def __init__(self, param, body):
        super().__init__()
        self.param, self.body = param, body

class FunctionExpr(Node):
    def __init__(self, params, body, is_async=False, return_type=None):
        super().__init__()
        self.params = params
        self.body = body
        self.is_async = is_async
        self.return_type = return_type

class Pipeline(Node):
    def __init__(self, left, right):
        super().__init__()
        self.left, self.right = left, right

class ArrayLiteral(Node):
    def __init__(self, items):
        super().__init__()
        self.items = items

class ObjectLiteral(Node):
    def __init__(self, props):
        super().__init__()
        self.props = props


class Spread(Node):
    def __init__(self, expr):
        super().__init__()
        self.expr = expr

class Match(Node):
    def __init__(self, expr, cases):
        super().__init__()
        self.expr, self.cases = expr, cases


class Case(Node):
    def __init__(self, pattern, expr):
        super().__init__()
        self.pattern, self.expr = pattern, expr

class Return(Node):
    def __init__(self, value):
        super().__init__()
        self.value = value

class If(Node):
    def __init__(self, cond, then, otherwise):
        super().__init__()
        self.cond = cond
        self.then = then
        self.otherwise = otherwise

class While(Node):
    def __init__(self, cond, body):
        super().__init__()
        self.cond = cond
        self.body = body

class Break(Node):
    def __init__(self):
        super().__init__()

class Continue(Node):
    def __init__(self):
        super().__init__()

class Exit(Node):
    def __init__(self, value):
        super().__init__()
        self.value = value

class Block(Node):
    def __init__(self, statements):
        super().__init__()
        self.statements = statements

class Let(Node):
    def __init__(self, name, value, type_name=None):
        super().__init__()
        self.name = name
        self.value = value
        self.type_name = type_name

class Raise(Node):
    def __init__(self, value):
        super().__init__()
        self.value = value

class TryCatch(Node):
    def __init__(self, try_block, error_name, catch_block):
        super().__init__()
        self.try_block = try_block
        self.error_name = error_name
        self.catch_block = catch_block

class Await(Node):
    def __init__(self, expr):
        super().__init__()
        self.expr = expr

class Record(Node):
    def __init__(self, name, fields):
        super().__init__()
        self.name = name
        self.fields = fields

class Actor(Node):
    def __init__(self, name, methods):
        super().__init__()
        self.name = name
        self.methods = methods

class Class(Node):
    def __init__(self, name, methods):
        super().__init__()
        self.name = name
        self.methods = methods

class Import(Node):
    def __init__(self, target, alias=None, is_path=False):
        super().__init__()
        self.target = target
        self.alias = alias
        self.is_path = is_path

class ErrorNode(Node):
    def __init__(self):
        super().__init__()

class Function(Node):
    def __init__(self, name, params, body, is_async=False, return_type=None):
        super().__init__()
        self.name = name
        self.params = params
        self.body = body
        self.is_async = is_async
        self.return_type = return_type
