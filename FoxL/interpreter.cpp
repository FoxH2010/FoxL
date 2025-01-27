#include "interpreter.h"
#include "cloner.cpp"
#include <iostream>
#include <stdexcept>
#include <cmath>

std::string stringifyValue(const Value &value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    } else if (std::holds_alternative<std::string>(value)) {
        return "\"" + std::get<std::string>(value) + "\"";
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::vector<Value>>(value)) {
        std::string result = "[";
        const auto &vec = std::get<std::vector<Value>>(value);
        for (size_t i = 0; i < vec.size(); ++i) {
            result += stringifyValue(vec[i]);
            if (i < vec.size() - 1) {
                result += ", ";
            }
        }
        result += "]";
        return result;
    }
    return "null";
}

// Environment methods
void Environment::setVariable(const std::string &name, const Value &value) {
    // Overwrite or create a new variable
    variables[name] = value;
}

Value Environment::getVariable(const std::string &name) const {
    auto it = variables.find(name);
    if (it == variables.end()) {
        throw std::runtime_error("Variable '" + name + "' not defined.");
    }
    return it->second;
}

bool Environment::hasVariable(const std::string &name) const {
    return variables.find(name) != variables.end();
}

void Environment::setFunction(const std::string &name, std::function<Value(const std::vector<Value> &)> function) {
    if (name.empty()) {
        throw std::runtime_error("Function name cannot be empty.");
    }
    functions[name] = std::move(function);
}

std::function<Value(const std::vector<Value> &)> Environment::getFunction(const std::string &name) const {
    auto it = functions.find(name);
    if (it == functions.end()) {
        throw std::runtime_error("Function '" + name + "' not defined.");
    }
    return it->second;
}

bool Environment::hasFunction(const std::string &name) const {
    return functions.find(name) != functions.end();
}

// Interpreter methods
Interpreter::Interpreter(std::shared_ptr<Environment> env) : environment(std::move(env)) {}

void Interpreter::interpret(const std::unique_ptr<ASTNode> &node) {
    if (!node) {
        return;
    }

    auto *stmt = dynamic_cast<const Statement *>(node.get());
    if (stmt) {
        executeStatement(stmt);
    } else if (const auto *funcCallExpr = dynamic_cast<const FunctionCallExpression *>(node.get())) {
        evaluateFunctionCall(funcCallExpr);
    } else {
        throw std::runtime_error("Unsupported AST node type in interpret.");
    }
}

Value Interpreter::evaluateExpression(const Expression *expr) {
    if (const auto *funcCallExpr = dynamic_cast<const FunctionCallExpression *>(expr)) {
        return evaluateFunctionCall(funcCallExpr);
    }
    if (const auto *numExpr = dynamic_cast<const NumberExpression *>(expr)) {
        return numExpr->value;
    } else if (const auto *strExpr = dynamic_cast<const StringExpression *>(expr)) {
        return strExpr->value;
    } else if (const auto *boolExpr = dynamic_cast<const BoolExpression *>(expr)) {
        return boolExpr->value;
    } else if (const auto *varExpr = dynamic_cast<const VariableExpression *>(expr)) {
        return environment->getVariable(varExpr->name);
    } else if (const auto *binExpr = dynamic_cast<const BinaryExpression *>(expr)) {
        auto left = evaluateExpression(binExpr->left.get());
        auto right = evaluateExpression(binExpr->right.get());
        if (binExpr->op == "+") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) + toDouble(right);
            } else if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
                return std::get<std::string>(left) + std::get<std::string>(right);
            }
        } else if (binExpr->op == "=") {
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                environment->setVariable(varExpr->name, right);
                return right;
            } else {
                throw std::runtime_error("Left-hand side of assignment must be a variable.");
            }
        } else if (binExpr->op == "-") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) - toDouble(right);
            }
        } else if (binExpr->op == "*") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) * toDouble(right);
            }
        } else if (binExpr->op == "/") {
            if (isNumber(left) && isNumber(right)) {
                if (toDouble(right) == 0) {
                    throw std::runtime_error("Division by zero.");
                }
                return toDouble(left) / toDouble(right);
            }
        } else if (binExpr->op == "%") {
            if (isNumber(left) && isNumber(right)) {
                if (toDouble(right) == 0) {
                    throw std::runtime_error("Division by zero.");
                }
                return std::fmod(toDouble(left), toDouble(right));
            }
        } else if (binExpr->op == "==") {
            return left == right;
        } else if (binExpr->op == "!=") {
            return left != right;
        } else if (binExpr->op == "<") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) < toDouble(right);
            }
        } else if (binExpr->op == "<=") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) <= toDouble(right);
            }
        } else if (binExpr->op == ">") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) > toDouble(right);
            }
        } else if (binExpr->op == ">=") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) >= toDouble(right);
            }
        } else if (binExpr->op == "&&") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) && toDouble(right);
            }
        } else if (binExpr->op == "||") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) || toDouble(right);
            }
        } else if (binExpr->op == "++") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) + 1;
            }
        } else if (binExpr->op == "--") {
            if (isNumber(left) && isNumber(right)) {
                return toDouble(left) - 1;
            }
        } else if (binExpr->op == "^") {
            if (isNumber(left) && isNumber(right)) {
                return std::pow(toDouble(left), toDouble(right));
            }
        } else if (binExpr->op == "+=" || binExpr->op == "-=" || binExpr->op == "*=" || 
                   binExpr->op == "/=" || binExpr->op == "%=") {
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                Value currentValue = environment->getVariable(varExpr->name);
                Value newValue;

                if (binExpr->op == "+=") {
                    newValue = toDouble(currentValue) + toDouble(right);
                } else if (binExpr->op == "-=") {
                    newValue = toDouble(currentValue) - toDouble(right);
                } else if (binExpr->op == "*=") {
                    newValue = toDouble(currentValue) * toDouble(right);
                } else if (binExpr->op == "/=") {
                    if (toDouble(right) == 0) {
                        throw std::runtime_error("Division by zero.");
                    }
                    newValue = toDouble(currentValue) / toDouble(right);
                } else if (binExpr->op == "%=") {
                    if (toDouble(right) == 0) {
                        throw std::runtime_error("Division by zero.");
                    }
                    newValue = std::fmod(toDouble(currentValue), toDouble(right));
                }

                environment->setVariable(varExpr->name, newValue); // Update the variable
                return newValue;
            } else {
                throw std::runtime_error("Left-hand side of compound assignment must be a variable.");
            }
        } else if (binExpr->op == "&&=") {
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = toDouble(value) && toDouble(right);
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "||=") {
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = toDouble(value) || toDouble(right);
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "^=") {
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = std::pow(toDouble(value), toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "^/") { // Root operator
            if (isNumber(left) && isNumber(right)) {
                if (toDouble(right) == 0) {
                    throw std::runtime_error("Root with exponent 0 is undefined.");
                }
                return std::pow(toDouble(left), 1.0 / toDouble(right));
            }
        } else if (binExpr->op == "^^") { // XOR operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) ^ static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == "<<") { // Left shift operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) << static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == ">>") { // Right shift operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) >> static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == "&") { // Bitwise AND operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) & static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == "|") { // Bitwise OR operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) | static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == "~") { // Bitwise NOT operator
            if (isNumber(right)) {
                return ~static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == "!") {
            if (isNumber(right)) {
                return !static_cast<bool>(toDouble(right));
            }
        } else if (binExpr->op == "in") {
            if (std::holds_alternative<std::vector<Value>>(right)) {
                for (const auto &elem : std::get<std::vector<Value>>(right)) {
                    if (elem == left) {
                        return true;
                    }
                }
                return false;
            }
        } else if (binExpr->op == "not in") {
            if (std::holds_alternative<std::vector<Value>>(right)) {
                for (const auto &elem : std::get<std::vector<Value>>(right)) {
                    if (elem == left) {
                        return false;
                    }
                }
                return true;
            }
        } else if (binExpr->op == "^/=") { // Root and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    if (toDouble(right) == 0) {
                        throw std::runtime_error("Root with exponent 0 is undefined.");
                    }
                    auto result = std::pow(toDouble(value), 1.0 / toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "^^=") { // XOR and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) ^ static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "<<=") { // Left shift and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) << static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == ">>=") { // Right shift and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) >> static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "&=") { // Bitwise AND and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) & static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "|=") { // Bitwise OR and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) | static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "~=") { // Bitwise NOT and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = ~static_cast<int>(toDouble(value));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == ">>=") { // Right shift and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) >> static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "<<=") { // Left shift and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) << static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "<<<") { // Zero-fill left shift operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) << static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == ">>>") { // Zero-fill right shift operator
            if (isNumber(left) && isNumber(right)) {
                return static_cast<int>(toDouble(left)) >> static_cast<int>(toDouble(right));
            }
        } else if (binExpr->op == "<<<=") { // Zero-fill left shift and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) << static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == ">>>=") { // Zero-fill right shift and assignment operation
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (isNumber(value) && isNumber(right)) {
                    auto result = static_cast<int>(toDouble(value)) >> static_cast<int>(toDouble(right));
                    environment->setVariable(varExpr->name, result);
                    return result;
                }
            }
        } else if (binExpr->op == "??") {
                return left.isNull() ? right : left;
        } else if (binExpr->op == "??=") { // Nullish coalescing assignment operator
            if (const auto *varExpr = dynamic_cast<const VariableExpression *>(binExpr->left.get())) {
                auto value = evaluateExpression(binExpr->left.get());
                if (!std::holds_alternative<std::monostate>(value)) {
                    return value; // Already defined, no assignment needed
                }
                environment->setVariable(varExpr->name, right); // Assign right
                return right;
            }
            throw std::runtime_error("Left-hand side of '??=' must be a variable.");
        } else if (binExpr->op == "?") {
            if (std::holds_alternative<bool>(left)) {
                return std::get<bool>(left) ? right : evaluateExpression(binExpr->rightElse.get());
            }
            throw std::runtime_error("Ternary condition must evaluate to a boolean.");
        } else if (binExpr->op == ":") {
            throw std::runtime_error("Unexpected ':' operator without matching ternary condition.");
        } else if (binExpr->op == "?:") {
            if (std::holds_alternative<bool>(left)) {
                return std::get<bool>(left) ? right : evaluateExpression(binExpr->rightElse.get());
            }
            throw std::runtime_error("Ternary condition must evaluate to a boolean.");
        } else if (binExpr->op == ",") {
            return right;
        }
        throw std::runtime_error("Unsupported operator '" + binExpr->op + "' with operands " +
                         stringifyValue(left) + " and " + stringifyValue(right));
    } else if (const auto *unaryExpr = dynamic_cast<const UnaryExpression *>(expr)) {
        auto operand = evaluateExpression(unaryExpr->operand.get());
        if (unaryExpr->op == "-") {
            if (isNumber(operand)) {
                return -toDouble(operand);
            }
        } else if (unaryExpr->op == "++") {
            if (isNumber(operand)) {
                double result = toDouble(operand) + 1;
                if (const auto *varExpr = dynamic_cast<const VariableExpression *>(unaryExpr->operand.get())) {
                    environment->setVariable(varExpr->name, result); // Update variable in environment
                }
                return result;
            }
        } else if (unaryExpr->op == "--") {
            if (isNumber(operand)) {
                double result = toDouble(operand) - 1;
                if (const auto *varExpr = dynamic_cast<const VariableExpression *>(unaryExpr->operand.get())) {
                    environment->setVariable(varExpr->name, result); // Update variable in environment
                }
                return result;
            }
        }
        throw std::runtime_error("Unsupported unary operator: " + unaryExpr->op);
    } else if (const auto *funcCallExpr = dynamic_cast<const FunctionCallExpression *>(expr)) {
        return evaluateFunctionCall(funcCallExpr);
    } else if (const auto *readExpr = dynamic_cast<const ReadExpression *>(expr)) {
        return executeReadExpression(readExpr);
    } else if (const auto *arrayExpr = dynamic_cast<const ArrayExpression *>(expr)) {
        std::vector<Value> elements;
        for (const auto &elem : arrayExpr->elements) {
            elements.push_back(evaluateExpression(elem.get()));
        }
        return elements;
    }
    throw std::runtime_error("Unsupported expression type.");
}

void Interpreter::executeStatement(const Statement *stmt) {
    if (const auto *blockStmt = dynamic_cast<const BlockStatement *>(stmt)) {
        executeBlock(blockStmt);
    } else if (const auto *ifStmt = dynamic_cast<const IfStatement *>(stmt)) {
        executeIfStatement(ifStmt);
    } else if (const auto *whileStmt = dynamic_cast<const WhileStatement *>(stmt)) {
        executeWhileStatement(whileStmt);
    } else if (const auto *forStmt = dynamic_cast<const ForStatement *>(stmt)) {
        executeForStatement(forStmt);
    } else if (const auto *forEachStmt = dynamic_cast<const ForEachStatement *>(stmt)) {
        executeForEachStatement(forEachStmt);
    } else if (const auto *writeStmt = dynamic_cast<const WriteStatement *>(stmt)) {
        executeWriteStatement(writeStmt);
    } else if (const auto *returnStmt = dynamic_cast<const ReturnStatement *>(stmt)) {
        executeReturnStatement(returnStmt);
    } else if (const auto *readStmt = dynamic_cast<const ReadStatement *>(stmt)) {
        executeReadStatement(readStmt);
    } else if (const auto *varDecl = dynamic_cast<const VariableDeclaration *>(stmt)) {
        executeVariableDeclaration(varDecl);
    } else if (const auto *funcDecl = dynamic_cast<const FunctionDeclaration *>(stmt)) {
        executeFunctionDeclaration(funcDecl);
    } else if (const auto *exprStmt = dynamic_cast<const ExpressionStatement *>(stmt)) {
        // Evaluate the expression statement (includes BinaryExpression for assignments)
        evaluateExpression(exprStmt->expression.get());
    } else if (const auto *reassignStmt = dynamic_cast<const VariableReassignmentStatement *>(stmt)) {
        executeVariableReassignment(reassignStmt);
    } else {
        throw std::runtime_error("Unsupported AST node type in interpret.");
    }
}

void Interpreter::executeBlock(const BlockStatement *block) {
    for (const auto &stmt : block->statements) {
        executeStatement(stmt.get());
    }
}

void Interpreter::executeIfStatement(const IfStatement *ifStmt) {
    auto condition = evaluateExpression(ifStmt->condition.get());
    if (std::holds_alternative<bool>(condition) && std::get<bool>(condition)) {
        executeBlock(dynamic_cast<const BlockStatement *>(ifStmt->thenBranch.get()));
    } else if (ifStmt->elseBranch) {
        executeBlock(dynamic_cast<const BlockStatement *>(ifStmt->elseBranch.get()));
    }
}

void Interpreter::executeWhileStatement(const WhileStatement *whileStmt) {
    while (std::holds_alternative<bool>(evaluateExpression(whileStmt->condition.get())) &&
           std::get<bool>(evaluateExpression(whileStmt->condition.get()))) {
        executeBlock(dynamic_cast<const BlockStatement *>(whileStmt->body.get()));
    }
}

void Interpreter::executeForStatement(const ForStatement *forStmt) {
    // Execute the initializer
    if (forStmt->initializer) {
        executeStatement(forStmt->initializer.get());
    }

    // Loop as long as the condition evaluates to true
    while (std::holds_alternative<bool>(evaluateExpression(forStmt->condition.get())) &&
           std::get<bool>(evaluateExpression(forStmt->condition.get()))) {
        // Execute the body
        executeBlock(dynamic_cast<const BlockStatement *>(forStmt->body.get()));

        // Execute the increment
        if (forStmt->increment) {
            executeStatement(forStmt->increment.get());
        }
    }
}

void Interpreter::executeForEachStatement(const ForEachStatement *forEachStmt) {
    auto iterable = evaluateExpression(forEachStmt->iterable.get());
    if (!std::holds_alternative<std::vector<Value>>(iterable)) {
        throw std::runtime_error("For-each loop requires an iterable.");
    }

    for (const auto &element : std::get<std::vector<Value>>(iterable)) {
        environment->setVariable(forEachStmt->variable, element);
        executeBlock(dynamic_cast<const BlockStatement *>(forEachStmt->body.get()));
    }
}

void Interpreter::executeWriteStatement(const WriteStatement *writeStmt) {
    auto value = evaluateExpression(writeStmt->messageExpr.get());
    printValue(value);
    std::cout << std::endl;
}

void Interpreter::executeReturnStatement(const ReturnStatement *returnStmt) {
    auto returnValue = returnStmt->returnValue ? evaluateExpression(returnStmt->returnValue.get()) : Value{};
    throw returnValue;
}

void Interpreter::executeReadStatement(const ReadStatement *readStmt) {
    if (readStmt->variable) {
        std::string prompt = readStmt->prompt ? stringifyValue(evaluateExpression(readStmt->prompt.get())) : "";
        if (!prompt.empty()) {
            std::cout << prompt;
        }

        std::string input;
        std::getline(std::cin, input);

        Value parsedValue;
        try {
            parsedValue = std::stoi(input);
        } catch (...) {
            parsedValue = input;
        }

        const auto *varExpr = dynamic_cast<const VariableExpression *>(readStmt->variable.get());
        if (!varExpr) {
            throw std::runtime_error("Invalid variable in 'read' statement.");
        }

        environment->setVariable(varExpr->name, parsedValue);
    } else {
        throw std::runtime_error("'read' statement must assign to a variable.");
    }
}

Value Interpreter::executeReadExpression(const ReadExpression *readExpr) {
    // Handle optional prompt
    std::string prompt = readExpr->prompt ? stringifyValue(evaluateExpression(readExpr->prompt.get())) : "";
    if (!prompt.empty()) {
        std::cout << prompt;
    }

    // Get user input
    std::string input;
    std::getline(std::cin, input);

    // Parse the input as a number or keep it as a string
    Value parsedValue;
    try {
        parsedValue = std::stoi(input);
    } catch (...) {
        parsedValue = input; // If not a number, treat as a string
    }

    return parsedValue;
}

void Interpreter::executeVariableDeclaration(const VariableDeclaration *varDecl) {
    std::string varName = varDecl->name;

    // Evaluate initializer if present
    Value initialValue = varDecl->initializer ? evaluateExpression(varDecl->initializer.get()) : Value{0};

    if (environment->hasVariable(varName)) {
        throw std::runtime_error("Variable '" + varName + "' already declared.");
    }

    // Declare variable in the environment
    environment->setVariable(varName, initialValue);
}

void Interpreter::printValue(const Value &value) const {
    if (std::holds_alternative<int>(value)) {
        std::cout << std::get<int>(value);
    } else if (std::holds_alternative<double>(value)) {
        std::cout << std::get<double>(value);
    } else if (std::holds_alternative<std::string>(value)) {
        std::cout << std::get<std::string>(value);
    } else if (std::holds_alternative<bool>(value)) {
        std::cout << (std::get<bool>(value) ? "true" : "false");
    } else {
        std::cout << "[Unsupported value type]";
    }
}

bool Interpreter::isNumber(const Value &value) const {
    return std::holds_alternative<int>(value) || std::holds_alternative<double>(value);
}

bool Interpreter::isDouble(const Value &value) const {
    return std::holds_alternative<double>(value);
}

double Interpreter::toDouble(const Value &value) const {
    if (std::holds_alternative<int>(value)) {
        return static_cast<double>(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    throw std::runtime_error("Value is not a number.");
}

void Interpreter::executeFunctionDeclaration(const FunctionDeclaration *funcDecl) {
    // Clone the function body as a shared pointer
    auto bodyClone = std::shared_ptr<Statement>(cloneStatement(funcDecl->body.get()));

    // Create a callable lambda for the function
    auto function = [funcDecl, environment = this->environment, bodyClone](const std::vector<Value> &arguments) -> Value {
        // Ensure correct argument count
        if (arguments.size() != funcDecl->parameters.size()) {
            throw std::runtime_error("Function '" + funcDecl->name + "' expects " +
                                     std::to_string(funcDecl->parameters.size()) +
                                     " arguments, but got " + std::to_string(arguments.size()) + ".");
        }

        // Create a new environment for the function call
        auto functionEnv = std::make_shared<Environment>(*environment);

        // Bind parameters to arguments
        for (size_t i = 0; i < funcDecl->parameters.size(); ++i) {
            functionEnv->setVariable(funcDecl->parameters[i], arguments[i]);
        }

        // Create a new interpreter for the function body with the new environment
        Interpreter functionInterpreter(functionEnv);

        try {
            functionInterpreter.executeStatement(bodyClone.get());
        } catch (const Value &returnValue) {
            return returnValue;
        }

        return Value{};
    };

    // Register the function in the environment
    environment->setFunction(funcDecl->name, std::move(function));
}

Value Interpreter::evaluateFunctionCall(const FunctionCallExpression *funcCall) {
    // Handle built-in `read` function
    if (funcCall->functionName == "read") {
        std::string prompt = funcCall->arguments.empty() ? "" : stringifyValue(evaluateExpression(funcCall->arguments[0].get()));
        if (!prompt.empty()) {
            std::cout << prompt;
        }

        std::string input;
        std::getline(std::cin, input);

        // Parse the input into the appropriate type
        try {
            return std::stoi(input); // Integer input
        } catch (...) {
            return input; // String input
        }
    }

    // Check if the function is defined in the environment
    if (!environment->hasFunction(funcCall->functionName)) {
        throw std::runtime_error("Function '" + funcCall->functionName + "' is not defined.");
    }

    auto function = environment->getFunction(funcCall->functionName);

    // Evaluate arguments
    std::vector<Value> arguments;
    for (const auto &arg : funcCall->arguments) {
        Value evaluatedArg = evaluateExpression(arg.get());
        arguments.push_back(evaluatedArg);
    }

    try {
        // Invoke the function
        Value result = function(arguments);
        return result;
    } catch (const Value &returnValue) {
        // Handle explicit `return` statements
        return returnValue;
    } catch (const std::exception &ex) {
        // Handle unexpected exceptions during function invocation
        throw std::runtime_error("Error while invoking function '" + funcCall->functionName + "': " + ex.what());
    }
}

void Interpreter::executeVariableReassignment(const VariableReassignmentStatement *stmt) {
    auto currentValue = environment->getVariable(stmt->variableName);
    auto newValue = evaluateExpression(stmt->valueExpr.get());

    if (stmt->op == "=") {
        environment->setVariable(stmt->variableName, newValue);
    } else if (stmt->op == "+=") {
        environment->setVariable(stmt->variableName, toDouble(currentValue) + toDouble(newValue));
    } else if (stmt->op == "-=") {
        environment->setVariable(stmt->variableName, toDouble(currentValue) - toDouble(newValue));
    } else if (stmt->op == "*=") {
        environment->setVariable(stmt->variableName, toDouble(currentValue) * toDouble(newValue));
    } else if (stmt->op == "/=") {
        if (toDouble(newValue) == 0) {
            throw std::runtime_error("Division by zero during reassignment.");
        }
        environment->setVariable(stmt->variableName, toDouble(currentValue) / toDouble(newValue));
    } else {
        throw std::runtime_error("Unsupported reassignment operator: " + stmt->op);
    }
}
