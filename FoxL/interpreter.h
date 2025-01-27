#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast_nodes.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <functional>

// Forward declaration to avoid circular dependency
struct Value;

std::string stringifyValue(const Value &value);

// Define the Value type to store different kinds of values
struct Value : public std::variant<std::monostate, int, double, std::string, bool, std::vector<Value>> {
    using std::variant<std::monostate, int, double, std::string, bool, std::vector<Value>>::variant;

    // Helper methods
    bool isNull() const { return std::holds_alternative<std::monostate>(*this); }
    bool isNumber() const { return std::holds_alternative<int>(*this) || std::holds_alternative<double>(*this); }
    bool isString() const { return std::holds_alternative<std::string>(*this); }
    bool isBool() const { return std::holds_alternative<bool>(*this); }
    bool isArray() const { return std::holds_alternative<std::vector<Value>>(*this); }

    // Conversion helpers
    double toDouble() const {
        if (std::holds_alternative<int>(*this)) return static_cast<double>(std::get<int>(*this));
        if (std::holds_alternative<double>(*this)) return std::get<double>(*this);
        throw std::runtime_error("Value is not a number.");
    }

    std::string toString() const {
        if (std::holds_alternative<std::string>(*this)) return std::get<std::string>(*this);
        if (std::holds_alternative<int>(*this)) return std::to_string(std::get<int>(*this));
        if (std::holds_alternative<double>(*this)) return std::to_string(std::get<double>(*this));
        if (std::holds_alternative<bool>(*this)) return std::get<bool>(*this) ? "true" : "false";
        throw std::runtime_error("Value is not convertible to string.");
    }
};

// Runtime environment for variable and function storage
class Environment {
public:
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, std::function<Value(const std::vector<Value> &)>> functions;

    // Set a variable in the current environment
    void setVariable(const std::string &name, const Value &value);

    // Get a variable's value, throwing an error if it doesn't exist
    Value getVariable(const std::string &name) const;
    bool hasVariable(const std::string &name) const;

    void setFunction(const std::string &name, std::function<Value(const std::vector<Value> &)> function);
    std::function<Value(const std::vector<Value> &)> getFunction(const std::string &name) const;
    bool hasFunction(const std::string &name) const;
};

// Interpreter class that interprets and executes the AST
class Interpreter {
public:
    explicit Interpreter(std::shared_ptr<Environment> env);

    // Main entry point for interpretation
    void interpret(const std::unique_ptr<ASTNode> &node);

private:
    std::shared_ptr<Environment> environment;

    // Evaluate an expression
    Value evaluateExpression(const Expression *expr);
    Value evaluateBinaryExpression(const BinaryExpression *binExpr);
    Value evaluateUnaryExpression(const UnaryExpression *unaryExpr);

    // Execute a statement
    void executeStatement(const Statement *stmt);

    // Execute specific types of statements
    void executeBlock(const BlockStatement *block);
    void executeIfStatement(const IfStatement *ifStmt);
    void executeWhileStatement(const WhileStatement *whileStmt);
    void executeForStatement(const ForStatement *forStmt);
    void executeForEachStatement(const ForEachStatement *forEachStmt);
    void executeWriteStatement(const WriteStatement *writeStmt);
    void executeReturnStatement(const ReturnStatement *returnStmt);
    void executeReadStatement(const ReadStatement *readStmt);
    void executeVariableDeclaration(const VariableDeclaration *varDecl);
    void executeVariableReassignment(const VariableReassignmentStatement *stmt);
    void executeFunctionDeclaration(const FunctionDeclaration *funcDecl);
    Value evaluateFunctionCall(const FunctionCallExpression *funcCall);
    Value executeReadExpression(const ReadExpression *readExpr);

    // Utility for printing values
    void printValue(const Value &value) const;

    // Utility for checking and converting types
    bool isNumber(const Value &value) const;
    bool isDouble(const Value &value) const;
    double toDouble(const Value &value) const;
};

#endif
