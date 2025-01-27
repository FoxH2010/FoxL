#ifndef AST_NODES_H
#define AST_NODES_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <utility>

// Base class for all AST nodes
class ASTNode {
public:
    int line;
    virtual ~ASTNode() = default;
    virtual void print() const = 0;
    explicit ASTNode(int line) : line(line) {}
};

// Base class for expressions
class Expression : public ASTNode {
public:
    std::string name;
    using ASTNode::ASTNode;

    Expression(int line) : ASTNode(line), name("") {}
    Expression(std::string name, int line) : ASTNode(line), name(std::move(name)) {}

    void print() const override = 0;
};

// Base class for statements
class Statement : public ASTNode {
public:
    using ASTNode::ASTNode;

    void print() const override = 0;
};

// A statement to write an expression
class WriteStatement : public Statement {
public:
    std::unique_ptr<Expression> messageExpr;

    explicit WriteStatement(std::unique_ptr<Expression> messageExpr, int line)
        : Statement(line), messageExpr(std::move(messageExpr)) {}

    void print() const override;
};

// An expression for reading input
class ReadExpression : public Expression {
public:
    std::unique_ptr<Expression> prompt;

    ReadExpression(std::unique_ptr<Expression> prompt, int line)
        : Expression(line), prompt(std::move(prompt)) {}

    void print() const override;
};

// A statement for declaring variables
class VariableDeclaration : public Statement {
public:
    std::string type;
    std::string name;
    std::unique_ptr<Expression> initializer;

    VariableDeclaration(std::string type, std::string name, std::unique_ptr<Expression> initializer, int line)
        : Statement(line), type(std::move(type)), name(std::move(name)), initializer(std::move(initializer)) {}

    virtual ~VariableDeclaration() = default;
    void print() const override;
};

// Expression for numeric values
class NumberExpression : public Expression {
public:
    double value;

    NumberExpression(double value, int line) : Expression(line), value(value) {}

    void print() const override;
};

// Expression for string literals
class StringExpression : public Expression {
public:
    std::string value;

    StringExpression(std::string value, int line) : Expression(line), value(std::move(value)) {}

    void print() const override;
};

// Expression for boolean values
class BoolExpression : public Expression {
public:
    bool value;

    BoolExpression(bool value, int line) : Expression(line), value(value) {}

    void print() const override;
};

// Expression for arrays
class ArrayExpression : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;

    ArrayExpression(std::vector<std::unique_ptr<Expression>> elements, int line)
        : Expression(line), elements(std::move(elements)) {}

    void print() const override;
};

// Expression for variable references
class VariableExpression : public Expression {
public:
    VariableExpression(std::string name, int line) : Expression(std::move(name), line) {}

    void print() const override;
};

// Expression for binary operations
class BinaryExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Expression> rightElse; // For ternary operator

    // Constructor for binary operations
    BinaryExpression(const std::string &op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, int line)
        : Expression(line), op(op), left(std::move(left)), right(std::move(right)), rightElse(nullptr) {}

    // Constructor for ternary operations
    BinaryExpression(const std::string &op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, std::unique_ptr<Expression> rightElse, int line)
        : Expression(line), op(op), left(std::move(left)), right(std::move(right)), rightElse(std::move(rightElse)) {}

    void print() const override;
};

// Expression for unary operations
class UnaryExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> operand;

    UnaryExpression(std::string op, std::unique_ptr<Expression> operand, int line)
        : Expression(line), op(std::move(op)), operand(std::move(operand)) {}

    void print() const override;
};

// Expression for function calls
class FunctionCallExpression : public Expression {
public:
    std::string functionName;
    std::vector<std::unique_ptr<Expression>> arguments;

    FunctionCallExpression(std::string functionName, std::vector<std::unique_ptr<Expression>> arguments, int line)
        : Expression(line), functionName(std::move(functionName)), arguments(std::move(arguments)) {}

    void print() const override;
};

// Expression for array indexing
class IndexExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    IndexExpression(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index, int line)
        : Expression(line), array(std::move(array)), index(std::move(index)) {}

    void print() const override;
};

// Function declaration
class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<Statement> body;

    FunctionDeclaration(std::string name, std::vector<std::string> parameters, std::unique_ptr<Statement> body, int line)
        : Statement(line), name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}

    void print() const override;
};

// Class for class members (fields or methods)
class ClassMember : public ASTNode {
public:
    std::string modifier; // public, private, or protected

    using ASTNode::ASTNode;

    ClassMember(int line, std::string modifier)
        : ASTNode(line), modifier(std::move(modifier)) {}

    virtual void print() const override = 0;
};

// Field declaration (class variables)
class FieldDeclaration : public ClassMember {
public:
    std::string type;
    std::string name;

    FieldDeclaration(std::string modifier, std::string type, std::string name, int line)
        : ClassMember(line, std::move(modifier)), type(std::move(type)), name(std::move(name)) {}

    void print() const override;
};

// Method declaration (class methods)
class MethodDeclaration : public ClassMember {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<Statement> body;

    MethodDeclaration(std::string modifier, std::string name, std::vector<std::string> parameters,
                       std::unique_ptr<Statement> body, int line)
        : ClassMember(line, std::move(modifier)), name(std::move(name)),
          parameters(std::move(parameters)), body(std::move(body)) {}

    void print() const override;
};

// Class declaration
class ClassDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<ClassMember>> members;

    ClassDeclaration(std::string name, std::vector<std::unique_ptr<ClassMember>> members, int line)
        : Statement(line), name(std::move(name)), members(std::move(members)) {}

    void print() const override;
};

// If statement
class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;

    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> thenBranch,
                std::unique_ptr<Statement> elseBranch, int line)
        : Statement(line), condition(std::move(condition)),
          thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    void print() const override;
};

// For statement
class ForStatement : public Statement {
public:
    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> increment;
    std::unique_ptr<Statement> body;

    ForStatement(std::unique_ptr<Statement> initializer,
                 std::unique_ptr<Expression> condition,
                 std::unique_ptr<Statement> increment,
                 std::unique_ptr<Statement> body,
                 int line)
        : Statement(line),
          initializer(std::move(initializer)),
          condition(std::move(condition)),
          increment(std::move(increment)),
          body(std::move(body)) {}

    void print() const override {
        std::cout << "ForStatement(line: " << line << ")\n";
        if (initializer) {
            std::cout << "Initializer: ";
            initializer->print();
        }
        if (condition) {
            std::cout << "Condition: ";
            condition->print();
        }
        if (increment) {
            std::cout << "Increment: ";
            increment->print();
        }
        if (body) {
            std::cout << "Body: ";
            body->print();
        }
    }
};

class VariableReassignmentStatement : public Statement {
public:
    std::string variableName;
    std::string op; // Assignment operator (e.g., =, +=, -=)
    std::unique_ptr<Expression> valueExpr;

    VariableReassignmentStatement(std::string variableName, std::string op, std::unique_ptr<Expression> valueExpr, int line)
        : Statement(line), variableName(std::move(variableName)), op(std::move(op)), valueExpr(std::move(valueExpr)) {}

    void print() const override {
        std::cout << "VariableReassignmentStatement(" << variableName << " " << op << " ..., line: " << line << ")\n";
        if (valueExpr) valueExpr->print();
    }
};

// While statement
class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body, int line)
        : Statement(line), condition(std::move(condition)), body(std::move(body)) {}

    void print() const override;
};

class ReadStatement : public Statement {
public:
    std::unique_ptr<Expression> variable;
    std::unique_ptr<Expression> prompt;

    // Constructor with a default value for the line parameter
    ReadStatement(std::unique_ptr<Expression> variable, 
                  std::unique_ptr<Expression> prompt = nullptr, 
                  int line = -1);

    void print() const override;
};

// Include statement
class IncludeStatement : public Statement {
public:
    std::string fileName;
    std::unique_ptr<Expression> target; // Optional target

    IncludeStatement(std::string fileName, int line, std::unique_ptr<Expression> target = nullptr)
        : Statement(line), fileName(std::move(fileName)), target(std::move(target)) {}

    void print() const override;
};

// Return statement
class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> returnValue;

    ReturnStatement(std::unique_ptr<Expression> returnValue, int line)
        : Statement(line), returnValue(std::move(returnValue)) {}

    void print() const override;
};

// For-each statement
class ForEachStatement : public Statement {
public:
    std::string variable;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;

    ForEachStatement(std::string variable, std::unique_ptr<Expression> iterable, std::unique_ptr<Statement> body, int line)
        : Statement(line), variable(std::move(variable)), iterable(std::move(iterable)), body(std::move(body)) {}

    void print() const override;
};

// Block statement
class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    BlockStatement(std::vector<std::unique_ptr<Statement>> statements, int line)
        : Statement(line), statements(std::move(statements)) {}

    void print() const override;
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    ExpressionStatement(std::unique_ptr<Expression> expr, int line)
        : Statement(line), expression(std::move(expr)) {}

    void print() const override;
};

std::unique_ptr<Expression> cloneExpression(const Expression *expr);
std::unique_ptr<Statement> cloneStatement(const Statement *stmt);

#endif
