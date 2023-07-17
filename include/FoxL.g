grammar FoxL;

parse
  : program EOF
  ;

program
  : statement+
  ;

statement
  : assignment
  | print
  | ifStatement
  | forStatement
  | whileStatement
  ;

assignment
  : identifier '=' expression
  ;

print
  : 'print' expression
  ;

ifStatement
  : 'if' '(' expression ')' block ('else' block)?
  ;

forStatement
  : 'for' '(' (declaration | expression) ';' expression ';' expression ')' block
  ;

whileStatement
  : 'while' '(' expression ')' block
  ;

declaration
  : identifier '=' expression
  ;

expression
  : logicalOrExpression
  ;

logicalOrExpression
  : logicalAndExpression ('or' logicalAndExpression)*
  ;

logicalAndExpression
  : equalityExpression ('and' equalityExpression)*
  ;

equalityExpression
  : relationalExpression (('==' | '!=') relationalExpression)*
  ;

relationalExpression
  : additiveExpression (('<' | '>' | '<=' | '>=')) additiveExpression)*
  ;

additiveExpression
  : multiplicativeExpression (('+' | '-') multiplicativeExpression)*
  ;

multiplicativeExpression
  : unaryExpression (('*' | '/') unaryExpression)*
  ;

unaryExpression
  : ('+' | '-') unaryExpression
  | primary
  ;

primary
  : identifier
  | literal
  | '(' expression ')'
  ;

identifier
  : ID
  ;

literal
  : INT
  | FLOAT
  | STRING
  ;

INT : [0-9]+;

FLOAT : [0-9]+('.'[0-9]+)?;

STRING : '"' ('\\'. | ~[\r\n"\\])* '"';

ID : [a-zA-Z_]+;

WS : [ \r\n\t]+ -> skip;