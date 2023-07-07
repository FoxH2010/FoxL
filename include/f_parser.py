import ply.yacc as yacc
import ply.lex as lex
import sys
import re
import ast

# List of token names
tokens = (
    'INT',
    'FLOAT',
    'ID',
    'FOR',
    'ELSE',
    'IF',
    'WHILE',
    'PRINT',
    'PLUS',
    'MINUS',
    'TIMES',
    'DIVIDE',
    'BLOCK',
    'IN',
    'UMINUS',
    'EOF',
    'LPAREN',
    'RPAREN'
)

t_PLUS = r"\+"
t_MINUS = r"-"
t_TIMES = r"\*"
t_DIVIDE = r"/"

def t_INT(t):
    r'\d+'
    t.value = int(t.value)
    return t

def t_FLOAT(t):
    r'\d+\.\d+'
    t.value = float(t.value)
    return t

def t_error(t):
    print("Syntax error at: '%s'" % t.value, file=sys.stderr)
    if t:
        print("Type:", t.type)

def p_error(t):
    print("Syntax error at: '%s'" % t.value, file=sys.stderr)
    if t:
        print("Expected:", t.type)

lexer = lex.lex()

# Grammar rules
def p_expression_plus(p):
    'expression : expression PLUS term'
    p[0] = p[1] + p[3]

def p_expression_minus(p):
    'expression : expression MINUS term'
    p[0] = p[1] - p[3]

def p_expression_term(p):
    'expression : term'
    p[0] = p[1]

def p_term_times(p):
    'term : term TIMES factor'
    p[0] = p[1] * p[3]

def p_term_divide(p):
    'term : term DIVIDE factor'
    p[0] = p[1] / p[3]

def p_term_factor(p):
    'term : factor'
    p[0] = p[1]

def p_factor_num(p):
    'factor : INT'
    p[0] = p[1]

def p_factor_expr(p):
    'factor : LPAREN expression RPAREN'
    p[0] = p[2]

# Error rule for syntax errors
def p_error(p):
    print("Syntax error in input!")

# Build the parser
parser = yacc.yacc()

def parser1(input_string):
  """Parses an input string and returns a list of tokens."""
  tokens = []
  for match in re.finditer(r"\S+", input_string):
    tokens.append(match.group())

  return tokens

def parse2(list):
  """Combines the things in a list to a string."""
  string = ""
  for thing in list:
    string += thing
  return string

def test(text):
  """Tries to parse the input string and prints the result."""
  try:
    result = parser.parse(text)
    print(result)
  except yacc.YaccError:
    print("Error parsing input")

def Parser(inputCode):
    test(parse2(parser1(inputCode)))

def parser(source_code):
  parser = Parser()
  parse_tree = parser.parse(source_code)
  return parse_tree

def semantic_analysis(parse_tree):
  symbol_table = {}
  for node in parse_tree:
    if isinstance(node, ast.Assign):
      variable = node.targets[0].id
      value = node.value
      symbol_table[variable] = type(value)

  for node in parse_tree:
    if isinstance(node, ast.BinOp):
      left_operand = node.left
      right_operand = node.right
      if symbol_table[left_operand] != symbol_table[right_operand]:
        raise TypeError("Types do not match")

  return symbol_table

def parse_program(source_code):
    parser = Parser(source_code)
    parse_tree = parser.parse(source_code)
    return parse_tree

if __name__ == "__main__":
    source_code = "int x = 1 + 2"
    result = parse_program(source_code)
    symbol_table = semantic_analysis(result)
    if isinstance(symbol_table, dict):
        print("The source code is syntactically correct and semantically correct")
    else:
        print("The source code is not syntactically correct or semantically correct")