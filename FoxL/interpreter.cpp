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
#include <type_traits>

// Forward declarations of AST classes
class Statement;
class Expression;
class WriteStatement;
class VariableDeclaration;
class IfStatement;
class ForStatement;
class ReturnStatement;
class BlockStatement;
class IncludeStatement;
class NumberExpression;
class StringExpression;
class BoolExpression;
class ArrayExpression;
class VariableExpression;
class BinaryExpression;
class FunctionCallExpression;
class ReadExpression;
class IndexExpression;

class Interpreter {
public:
    void interpret(const std::vector<std::unique_ptr<Statement>>& statements) {
        for (const auto& statement : statements) {
            execute(statement.get());
        }
    }

private:
    std::unordered_map<std::string, std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>> variables;

    void execute(const Statement* statement) {
        if (const auto* writeStmt = dynamic_cast<const WriteStatement*>(statement)) {
            auto value = evaluate(writeStmt->messageExpr.get());
            std::visit([](auto&& arg) { printValue(arg); }, value);
        } else if (const auto* varDecl = dynamic_cast<const VariableDeclaration*>(statement)) {
            if (varDecl->type == "auto") {
                if (varDecl->initializer) {
                    variables[varDecl->name] = evaluate(varDecl->initializer.get());
                } else {
                    throw std::runtime_error("Variable '" + varDecl->name + "' has no initializer");
                }
            } else {
                if (varDecl->initializer) {
                    variables[varDecl->name] = evaluate(varDecl->initializer.get());
                } else {
                    if (varDecl->type == "int") {
                        variables[varDecl->name] = 0;
                    } else if (varDecl->type == "double") {
                        variables[varDecl->name] = 0.0;
                    } else if (varDecl->type == "str") {
                        variables[varDecl->name] = std::string("");
                    } else if (varDecl->type == "bool") {
                        variables[varDecl->name] = false;
                    }
                }
            }
        } else if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(statement)) {
            // Handle function declarations
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
        } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(statement)) {
            evaluate(returnStmt->expression.get());
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
    }

    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluate(const Expression* expr) {
        if (const auto* numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
            return numberExpr->value;
        } else if (const auto* strExpr = dynamic_cast<const StringExpression*>(expr)) {
            return strExpr->value;
        } else if (const auto* boolExpr = dynamic_cast<const BoolExpression*>(expr)) {
            return boolExpr->value;
        } else if (const auto* varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            if (variables.find(varExpr->name) != variables.end()) {
                return variables[varExpr->name];
            } else {
                throw std::runtime_error("Undefined variable: " + varExpr->name);
            }
        } else if (const auto* binExpr = dynamic_cast<const BinaryExpression*>(expr)) {
            auto left = evaluate(binExpr->left.get());
            auto right = evaluate(binExpr->right.get());

            if (binExpr->op == "+") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) + std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) + std::get<double>(right);
                } else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
                    return std::get<std::string>(left) + std::get<std::string>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '+'.");
                }
            } else if (binExpr->op == "-") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) - std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) - std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '-'.");
                }
            } else if (binExpr->op == "*") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) * std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) * std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '*'.");
                }
            } else if (binExpr->op == "/") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) / std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) / std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '/'.");
                }
            } else if (binExpr->op == "==") {
                return left == right;
            } else if (binExpr->op == "!=") {
                return left != right;
            } else if (binExpr->op == "<") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) < std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) < std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '<'.");
                }
            } else if (binExpr->op == ">") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) > std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) > std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '>'.");
                }
            } else if (binExpr->op == "<=") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) <= std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) <= std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '<='.");
                }
            } else if (binExpr->op == ">=") {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return std::get<int>(left) >= std::get<int>(right);
                } else if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                    return std::get<double>(left) >= std::get<double>(right);
                } else {
                    throw std::runtime_error("Unsupported operand types for '>='.");
                }
            } else {
                throw std::runtime_error("Unsupported binary operator: " + binExpr->op);
            }
        } else if (const auto* readExpr = dynamic_cast<const ReadExpression*>(expr)) {
            std::string prompt = "Enter value: ";
            if (readExpr->prompt) {
                prompt = std::get<std::string>(evaluate(readExpr->prompt.get()));
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
        } else if (const auto* indexExpr = dynamic_cast<const IndexExpression*>(expr)) {
            auto arrayVar = evaluate(indexExpr->array.get());
            auto index = std::get<int>(evaluate(indexExpr->index.get()));

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
        } else {
            std::stringstream ss;
            ss << "Unsupported expression type: " << typeid(*expr).name();
            throw std::runtime_error(ss.str());
        }

        std::stringstream ss;
        ss << "Unsupported expression type: " << typeid(*expr).name();
        throw std::runtime_error(ss.str());
    }

    bool isNumber(const std::string& s) {
        return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
    }

    bool isDouble(const std::string& s) {
        std::istringstream iss(s);
        double d;
        iss >> d;
        return iss.eof() && !iss.fail();
    }

    template <typename T>
    static void printValue(const T& value) {
        std::cout << value << std::endl;
    }

    template <typename T>
    static void printValue(const std::vector<T>& vec) {
        std::cout << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            std::cout << vec[i];
            if (i < vec.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
};