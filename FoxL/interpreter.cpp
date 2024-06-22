#include <unordered_map>
#include <functional>
#include <cmath>
#include <stdexcept>
#include <variant>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <optional>

class ReturnException : public std::runtime_error {
public:
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> value;

    ReturnException(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& value)
        : std::runtime_error("Return"), value(value) {}
};

class Interpreter {
public:
    void interpret(const std::vector<std::unique_ptr<Statement>>& statements) {
        for (const auto& statement : statements) {
            execute(statement.get());
        }
    }

private:
    struct Variable {
        std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> value;
        bool isConstant;
    };

    using Function = std::pair<std::vector<std::string>, std::vector<std::unique_ptr<Statement>>>;

    std::unordered_map<std::string, Variable> variables;
    std::unordered_map<std::string, Function> functions;

    void execute(const Statement* statement);

    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluate(const Expression* expr);

    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateNumberExpression(const NumberExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateStringExpression(const StringExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateBoolExpression(const BoolExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateVariableExpression(const VariableExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateBinaryExpression(const BinaryExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateReadExpression(const ReadExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateIndexExpression(const IndexExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateArrayExpression(const ArrayExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateFunctionCallExpression(const FunctionCallExpression* expr);

    bool isNumber(const std::string& s);
    bool isDouble(const std::string& s);

    template <typename T>
    static void printValue(const T& value);

    template <typename T>
    static void printValue(const std::vector<T>& vec);

    std::vector<std::unique_ptr<Statement>> cloneStatements(const std::vector<std::unique_ptr<Statement>>& statements);
    std::unique_ptr<Statement> cloneStatement(const Statement* stmt);
    std::unique_ptr<Expression> cloneExpression(const Expression* expr);
};

void Interpreter::execute(const Statement* statement) {
    try {
        if (const auto* writeStmt = dynamic_cast<const WriteStatement*>(statement)) {
            auto value = evaluate(writeStmt->messageExpr.get());
            std::visit([](auto&& arg) { printValue(arg); }, value);
        } else if (const auto* varDecl = dynamic_cast<const VariableDeclaration*>(statement)) {
            if (variables.find(varDecl->name) != variables.end() && variables[varDecl->name].isConstant) {
                throw std::runtime_error("Cannot reassign constant variable: " + varDecl->name);
            }

            if (varDecl->initializer) {
                variables[varDecl->name] = { evaluate(varDecl->initializer.get()), varDecl->type == "const" };
            } else {
                if (varDecl->type == "int") {
                    variables[varDecl->name] = { 0, varDecl->type == "const" };
                } else if (varDecl->type == "double") {
                    variables[varDecl->name] = { 0.0, varDecl->type == "const" };
                } else if (varDecl->type == "str") {
                    variables[varDecl->name] = { std::string(""), varDecl->type == "const" };
                } else if (varDecl->type == "bool") {
                    variables[varDecl->name] = { false, varDecl->type == "const" };
                }
            }
        } else if (const auto* varExpr = dynamic_cast<const VariableExpression*>(statement)) {
            if (variables.find(varExpr->name) != variables.end() && !variables[varExpr->name].isConstant) {
                variables[varExpr->name] = { evaluate(varExpr), false };
            } else {
                throw std::runtime_error("Cannot reassign constant variable: " + varExpr->name);
            }
        } else if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(statement)) {
            functions[funcDecl->name] = { funcDecl->parameters, cloneStatements(funcDecl->body) };
        } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(statement)) {
            if (std::get<bool>(evaluate(ifStmt->condition.get()))) {
                execute(ifStmt->thenBranch.get());
            } else if (ifStmt->elseBranch) {
                execute(ifStmt->elseBranch.get());
            }
        } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(statement)) {
            execute(forStmt->initializer.get());
            while (std::get<bool>(evaluate(forStmt->condition.get()))) {
                execute(forStmt->body.get());
                execute(forStmt->increment.get());
            }
        } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(statement)) {
            while (std::get<bool>(evaluate(whileStmt->condition.get()))) {
                execute(whileStmt->body.get());
            }
        } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(statement)) {
            auto value = evaluate(returnStmt->expression.get());
            throw ReturnException(value);
        } else if (const auto* blockStmt = dynamic_cast<const BlockStatement*>(statement)) {
            for (const auto& stmt : blockStmt->statements) {
                execute(stmt.get());
            }
        } else if (const auto* includeStmt = dynamic_cast<const IncludeStatement*>(statement)) {
            std::ifstream file(includeStmt->fileName);
            if (!file) {
                throw std::runtime_error("Error: Could not open include file " + includeStmt->fileName);
            }

            std::string input((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            Lexer lexer(input);
            Parser parser(lexer);

            try {
                std::vector<std::unique_ptr<Statement>> statements;
                while (auto ast = parser.parse()) {
                    if (auto stmt = dynamic_cast<Statement*>(ast.get())) {
                        statements.push_back(std::unique_ptr<Statement>(stmt));
                        ast.release();
                    }
                }

                interpret(statements);
            } catch (const std::exception& e) {
                throw std::runtime_error("Error in included file: " + std::string(e.what()));
            }
        }
    } catch (const ReturnException& e) {
        throw; // Rethrow to handle return value
    } catch (const std::exception& e) {
        std::cerr << "Error executing statement: " << e.what() << std::endl;
    }
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluate(const Expression* expr) {
    try {
        if (const auto* numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
            return evaluateNumberExpression(numberExpr);
        } else if (const auto* strExpr = dynamic_cast<const StringExpression*>(expr)) {
            return evaluateStringExpression(strExpr);
        } else if (const auto* boolExpr = dynamic_cast<const BoolExpression*>(expr)) {
            return evaluateBoolExpression(boolExpr);
        } else if (const auto* varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            return evaluateVariableExpression(varExpr);
        } else if (const auto* binExpr = dynamic_cast<const BinaryExpression*>(expr)) {
            return evaluateBinaryExpression(binExpr);
        } else if (const auto* readExpr = dynamic_cast<const ReadExpression*>(expr)) {
            return evaluateReadExpression(readExpr);
        } else if (const auto* indexExpr = dynamic_cast<const IndexExpression*>(expr)) {
            return evaluateIndexExpression(indexExpr);
        } else if (const auto* arrayExpr = dynamic_cast<const ArrayExpression*>(expr)) {
            return evaluateArrayExpression(arrayExpr);
        } else if (const auto* funcCallExpr = dynamic_cast<const FunctionCallExpression*>(expr)) {
            return evaluateFunctionCallExpression(funcCallExpr);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " at line " + std::to_string(expr->line));
    }

    std::stringstream ss;
    ss << "Unsupported expression type: " << typeid(*expr).name() << " at line " << std::to_string(expr->line);
    throw std::runtime_error(ss.str());
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateNumberExpression(const NumberExpression* expr) {
    return static_cast<int>(expr->value); // Convert double to int for consistency
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateStringExpression(const StringExpression* expr) {
    return expr->value;
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateBoolExpression(const BoolExpression* expr) {
    return expr->value;
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateVariableExpression(const VariableExpression* expr) {
    if (variables.find(expr->name) != variables.end()) {
        return variables[expr->name].value;
    } else {
        throw std::runtime_error("Undefined variable: " + expr->name);
    }
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateBinaryExpression(const BinaryExpression* expr) {
    auto left = evaluate(expr->left.get());
    auto right = evaluate(expr->right.get());

    if (expr->op == "=") {
        if (const auto* leftVar = dynamic_cast<const VariableExpression*>(expr->left.get())) {
            if (variables.find(leftVar->name) == variables.end()) {
                throw std::runtime_error("Undefined variable: " + leftVar->name);
            }
            if (variables[leftVar->name].isConstant) {
                throw std::runtime_error("Cannot assign to constant variable: " + leftVar->name);
            }
            auto rightValue = evaluate(expr->right.get());
            variables[leftVar->name].value = rightValue;
            return rightValue;
        } else if (const auto* indexExpr = dynamic_cast<const IndexExpression*>(expr->left.get())) {
            auto arrayVar = dynamic_cast<const VariableExpression*>(indexExpr->array.get());
            if (!arrayVar) {
                throw std::runtime_error("Array must be a variable.");
            }

            auto arrayValue = evaluate(arrayVar);
            auto index = std::get<int>(evaluate(indexExpr->index.get()));
            auto rightValue = evaluate(expr->right.get());

            if (std::holds_alternative<std::vector<int>>(arrayValue)) {
                auto& array = std::get<std::vector<int>>(variables[arrayVar->name].value);
                if (index >= 0 && index < array.size()) {
                    array[index] = std::get<int>(rightValue);
                } else {
                    throw std::runtime_error("Index out of bounds");
                }
            } else if (std::holds_alternative<std::vector<std::string>>(arrayValue)) {
                auto& array = std::get<std::vector<std::string>>(variables[arrayVar->name].value);
                if (index >= 0 && index < array.size()) {
                    array[index] = std::get<std::string>(rightValue);
                } else {
                    throw std::runtime_error("Index out of bounds");
                }
            } else {
                throw std::runtime_error("Variable is not an array");
            }
            return rightValue;
        } else {
            throw std::runtime_error("Unsupported left-hand side in assignment.");
        }
    }

    if (expr->op == "+") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) + std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) + std::get<double>(right);
        } else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
            return std::get<std::string>(left) + std::get<std::string>(right);
        } else if (std::holds_alternative<std::string>(left) && std::holds_alternative<int>(right)) {
            return std::get<std::string>(left) + std::to_string(std::get<int>(right));
        } else if (std::holds_alternative<int>(left) && std::holds_alternative<std::string>(right)) {
            return std::to_string(std::get<int>(left)) + std::get<std::string>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '+'.");
        }
    } else if (expr->op == "-") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) - std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) - std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '-'.");
        }
    } else if (expr->op == "*") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) * std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) * std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '*'.");
        }
    } else if (expr->op == "/") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            if (std::get<int>(right) == 0) {
                throw std::runtime_error("Division by zero.");
            }
            return std::get<int>(left) / std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            if (std::get<double>(right) == 0.0) {
                throw std::runtime_error("Division by zero.");
            }
            return std::get<double>(left) / std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '/'.");
        }
    } else if (expr->op == "==") {
        return left == right;
    } else if (expr->op == "!=") {
        return left != right;
    } else if (expr->op == "<") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) < std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) < std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '<'.");
        }
    } else if (expr->op == ">") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) > std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) > std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '>'.");
        }
    } else if (expr->op == "<=") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) <= std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) <= std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '<='.");
        }
    } else if (expr->op == ">=") {
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            return std::get<int>(left) >= std::get<int>(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            return std::get<double>(left) >= std::get<double>(right);
        } else {
            throw std::runtime_error("Unsupported operand types for '>='.");
        }
    } else {
        throw std::runtime_error("Unsupported binary operator: " + expr->op);
    }
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateReadExpression(const ReadExpression* expr) {
    std::string prompt = "Enter value: ";
    if (expr->prompt) {
        prompt = std::get<std::string>(evaluate(expr->prompt.get()));
    }
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    if (isNumber(input)) {
        return std::stoi(input);
    } else if (isDouble(input)) {
        return std::stod(input);
    } else {
        return input;
    }
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateIndexExpression(const IndexExpression* expr) {
    auto arrayVar = evaluate(expr->array.get());
    auto index = std::get<int>(evaluate(expr->index.get()));

    if (std::holds_alternative<std::vector<int>>(arrayVar)) {
        const auto& array = std::get<std::vector<int>>(arrayVar);
        if (index >= 0 && index < array.size()) {
            return array[index];
        } else {
            throw std::runtime_error("Index out of bounds");
        }
    } else if (std::holds_alternative<std::vector<std::string>>(arrayVar)) {
        const auto& array = std::get<std::vector<std::string>>(arrayVar);
        if (index >= 0 && index < array.size()) {
            return array[index];
        } else {
            throw std::runtime_error("Index out of bounds");
        }
    } else {
        throw std::runtime_error("Variable is not an array");
    }
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateArrayExpression(const ArrayExpression* expr) {
    if (!expr->elements.empty()) {
        if (dynamic_cast<const NumberExpression*>(expr->elements[0].get())) {
            std::vector<int> intArray;
            for (const auto& elem : expr->elements) {
                intArray.push_back(std::get<int>(evaluate(elem.get())));
            }
            return intArray;
        } else if (dynamic_cast<const StringExpression*>(expr->elements[0].get())) {
            std::vector<std::string> stringArray;
            for (const auto& elem : expr->elements) {
                stringArray.push_back(std::get<std::string>(evaluate(elem.get())));
            }
            return stringArray;
        } else {
            throw std::runtime_error("Unsupported array element type.");
        }
    }
    return {};
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluateFunctionCallExpression(const FunctionCallExpression* expr) {
    if (functions.find(expr->functionName) == functions.end()) {
        throw std::runtime_error("Undefined function: " + expr->functionName);
    }
    auto& function = functions[expr->functionName];
    auto& parameters = function.first;
    auto& body = function.second;
    if (parameters.size() != expr->arguments.size()) {
        throw std::runtime_error("Function " + expr->functionName + " called with incorrect number of arguments");
    }
    std::unordered_map<std::string, Variable> localVariables = variables;
    for (size_t i = 0; i < parameters.size(); ++i) {
        localVariables[parameters[i]] = { evaluate(expr->arguments[i].get()), false };
    }
    auto oldVariables = variables;
    variables = localVariables;
    try {
        for (const auto& stmt : body) {
            execute(stmt.get());
        }
    } catch (const ReturnException& e) {
        variables = oldVariables;
        return e.value;
    }
    variables = oldVariables;
    return 0; // Default return value if no return statement is encountered.
}

bool Interpreter::isNumber(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

bool Interpreter::isDouble(const std::string& s) {
    std::istringstream iss(s);
    double d;
    iss >> d;
    return iss.eof() && !iss.fail();
}

template <typename T>
void Interpreter::printValue(const T& value) {
    std::cout << value << std::endl;
}

template <typename T>
void Interpreter::printValue(const std::vector<T>& vec) {
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i < vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

std::vector<std::unique_ptr<Statement>> Interpreter::cloneStatements(const std::vector<std::unique_ptr<Statement>>& statements) {
    std::vector<std::unique_ptr<Statement>> clones;
    for (const auto& stmt : statements) {
        clones.push_back(cloneStatement(stmt.get()));
    }
    return clones;
}

std::unique_ptr<Statement> Interpreter::cloneStatement(const Statement* stmt) {
    if (const auto* writeStmt = dynamic_cast<const WriteStatement*>(stmt)) {
        return std::make_unique<WriteStatement>(cloneExpression(writeStmt->messageExpr.get()), writeStmt->line);
    } else if (const auto* varDecl = dynamic_cast<const VariableDeclaration*>(stmt)) {
        return std::make_unique<VariableDeclaration>(varDecl->type, varDecl->name, cloneExpression(varDecl->initializer.get()), varDecl->line);
    } else if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(stmt)) {
        return std::make_unique<FunctionDeclaration>(funcDecl->name, funcDecl->parameters, cloneStatements(funcDecl->body), funcDecl->line);
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(stmt)) {
        return std::make_unique<IfStatement>(cloneExpression(ifStmt->condition.get()), cloneStatement(ifStmt->thenBranch.get()), ifStmt->elseBranch ? cloneStatement(ifStmt->elseBranch.get()) : nullptr, ifStmt->line);
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(stmt)) {
        return std::make_unique<ForStatement>(cloneStatement(forStmt->initializer.get()), cloneExpression(forStmt->condition.get()), cloneStatement(forStmt->increment.get()), cloneStatement(forStmt->body.get()), forStmt->line);
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(stmt)) {
        return std::make_unique<WhileStatement>(cloneExpression(whileStmt->condition.get()), cloneStatement(whileStmt->body.get()), whileStmt->line);
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(stmt)) {
        return std::make_unique<ReturnStatement>(cloneExpression(returnStmt->expression.get()), returnStmt->line);
    } else if (const auto* blockStmt = dynamic_cast<const BlockStatement*>(stmt)) {
        return std::make_unique<BlockStatement>(cloneStatements(blockStmt->statements), blockStmt->line);
    } else if (const auto* includeStmt = dynamic_cast<const IncludeStatement*>(stmt)) {
        return std::make_unique<IncludeStatement>(includeStmt->fileName, includeStmt->line);
    } else {
        throw std::runtime_error("Unsupported statement type");
    }
}

std::unique_ptr<Expression> Interpreter::cloneExpression(const Expression* expr) {
    if (const auto* numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
        return std::make_unique<NumberExpression>(numberExpr->value, numberExpr->line);
    } else if (const auto* strExpr = dynamic_cast<const StringExpression*>(expr)) {
        return std::make_unique<StringExpression>(strExpr->value, strExpr->line);
    } else if (const auto* boolExpr = dynamic_cast<const BoolExpression*>(expr)) {
        return std::make_unique<BoolExpression>(boolExpr->value, boolExpr->line);
    } else if (const auto* varExpr = dynamic_cast<const VariableExpression*>(expr)) {
        return std::make_unique<VariableExpression>(varExpr->name, varExpr->line);
    } else if (const auto* binExpr = dynamic_cast<const BinaryExpression*>(expr)) {
        return std::make_unique<BinaryExpression>(cloneExpression(binExpr->left.get()), binExpr->op, cloneExpression(binExpr->right.get()), binExpr->line);
    } else if (const auto* readExpr = dynamic_cast<const ReadExpression*>(expr)) {
        return std::make_unique<ReadExpression>(readExpr->prompt ? cloneExpression(readExpr->prompt.get()) : nullptr, readExpr->line);
    } else if (const auto* indexExpr = dynamic_cast<const IndexExpression*>(expr)) {
        return std::make_unique<IndexExpression>(cloneExpression(indexExpr->array.get()), cloneExpression(indexExpr->index.get()), indexExpr->line);
    } else if (const auto* arrayExpr = dynamic_cast<const ArrayExpression*>(expr)) {
        std::vector<std::unique_ptr<Expression>> elements;
        for (const auto& elem : arrayExpr->elements) {
            elements.push_back(cloneExpression(elem.get()));
        }
        return std::make_unique<ArrayExpression>(std::move(elements), arrayExpr->line);
    } else if (const auto* funcCallExpr = dynamic_cast<const FunctionCallExpression*>(expr)) {
        std::vector<std::unique_ptr<Expression>> arguments;
        for (const auto& arg : funcCallExpr->arguments) {
            arguments.push_back(cloneExpression(arg.get()));
        }
        return std::make_unique<FunctionCallExpression>(funcCallExpr->functionName, std::move(arguments), funcCallExpr->line);
    } else {
        throw std::runtime_error("Unsupported expression type");
    }
}
