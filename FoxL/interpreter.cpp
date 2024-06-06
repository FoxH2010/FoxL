#include <unordered_map>
#include <functional>
#include <cmath>
#include <stdexcept>
#include <variant>
#include <iostream>
#include <vector>

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
        if (auto writeStmt = dynamic_cast<const WriteStatement*>(statement)) {
            if (writeStmt->isVariable) {
                if (variables.find(writeStmt->message) != variables.end()) {
                    auto value = variables[writeStmt->message];
                    std::visit([](auto&& arg) { printValue(arg); }, value);
                } else {
                    throw std::runtime_error("Undefined variable: " + writeStmt->message);
                }
            } else {
                std::cout << writeStmt->message << std::endl;
            }
        } else if (auto varDecl = dynamic_cast<const VariableDeclaration*>(statement)) {
            if (varDecl->type == "auto") {
                if (varDecl->initializer) {
                    auto initExpr = varDecl->initializer.get();
                    if (auto numberExpr = dynamic_cast<NumberExpression*>(initExpr)) {
                        variables[varDecl->name] = static_cast<int>(numberExpr->value);
                    } else if (auto strExpr = dynamic_cast<StringExpression*>(initExpr)) {
                        variables[varDecl->name] = strExpr->value;
                    } else if (auto boolExpr = dynamic_cast<BoolExpression*>(initExpr)) {
                        variables[varDecl->name] = boolExpr->value;
                    } else if (auto arrayExpr = dynamic_cast<ArrayExpression*>(initExpr)) {
                        if (!arrayExpr->elements.empty() && dynamic_cast<NumberExpression*>(arrayExpr->elements[0].get())) {
                            std::vector<int> arrayValues;
                            for (const auto& elem : arrayExpr->elements) {
                                arrayValues.push_back(evaluateInt(elem.get()));
                            }
                            variables[varDecl->name] = arrayValues;
                        } else if (!arrayExpr->elements.empty() && dynamic_cast<StringExpression*>(arrayExpr->elements[0].get())) {
                            std::vector<std::string> arrayValues;
                            for (const auto& elem : arrayExpr->elements) {
                                arrayValues.push_back(evaluateString(elem.get()));
                            }
                            variables[varDecl->name] = arrayValues;
                        } else {
                            throw std::runtime_error("Unsupported initializer type for variable: " + varDecl->name);
                        }
                    } else if (auto readExpr = dynamic_cast<ReadExpression*>(initExpr)) {
                        std::string input;
                        std::getline(std::cin, input);
                        if (isNumber(input)) {
                            variables[varDecl->name] = std::stoi(input);
                        } else {
                            variables[varDecl->name] = input;
                        }
                    } else {
                        throw std::runtime_error("Unsupported initializer type for variable: " + varDecl->name);
                    }
                } else {
                    throw std::runtime_error("Variable '" + varDecl->name + "' has no initializer");
                }
            } else {
                if (varDecl->initializer) {
                    if (varDecl->type == "int") {
                        variables[varDecl->name] = evaluateInt(varDecl->initializer.get());
                    } else if (varDecl->type == "double") {
                        variables[varDecl->name] = evaluateDouble(varDecl->initializer.get());
                    } else if (varDecl->type == "str") {
                        variables[varDecl->name] = evaluateString(varDecl->initializer.get());
                    } else if (varDecl->type == "bool") {
                        variables[varDecl->name] = evaluateInt(varDecl->initializer.get()) != 0;
                    }
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
        } else if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(statement)) {
            // Handle function declarations
        } else if (auto ifStmt = dynamic_cast<const IfStatement*>(statement)) {
            if (evaluateInt(ifStmt->condition.get())) {
                execute(ifStmt->thenBranch.get());
            } else if (ifStmt->elseBranch) {
                execute(ifStmt->elseBranch.get());
            }
        } else if (auto forStmt = dynamic_cast<const ForStatement*>(statement)) {
            execute(forStmt->initializer.get());
            while (evaluateInt(forStmt->condition.get())) {
                execute(forStmt->body.get());
                execute(forStmt->increment.get());
            }
        } else if (auto returnStmt = dynamic_cast<const ReturnStatement*>(statement)) {
            evaluateInt(returnStmt->expression.get());
        } else if (auto blockStmt = dynamic_cast<const BlockStatement*>(statement)) {
            for (const auto& stmt : blockStmt->statements) {
                execute(stmt.get());
            }
        } else if (auto includeStmt = dynamic_cast<const IncludeStatement*>(statement)) {
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

    int evaluateInt(const Expression* expr) {
        if (auto numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
            return static_cast<int>(numberExpr->value);
        } else if (auto varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            if (variables.find(varExpr->name) != variables.end()) {
                auto value = variables[varExpr->name];
                if (std::holds_alternative<int>(value)) {
                    return std::get<int>(value);
                } else if (std::holds_alternative<double>(value)) {
                    return static_cast<int>(std::get<double>(value));
                } else if (std::holds_alternative<std::string>(value)) {
                    try {
                        return std::stoi(std::get<std::string>(value));
                    } catch (const std::invalid_argument&) {
                        throw std::runtime_error("Cannot convert string to int: " + std::get<std::string>(value));
                    }
                } else if (std::holds_alternative<bool>(value)) {
                    return std::get<bool>(value) ? 1 : 0;
                }
            } else {
                throw std::runtime_error("Undefined variable: " + varExpr->name);
            }
        } else if (auto binExpr = dynamic_cast<const BinaryExpression*>(expr)) {
            int left = evaluateInt(binExpr->left.get());
            int right = evaluateInt(binExpr->right.get());
            if (binExpr->op == "+") {
                return left + right;
            } else if (binExpr->op == "-") {
                return left - right;
            } else if (binExpr->op == "*") {
                return left * right;
            } else if (binExpr->op == "/") {
                return left / right;
            } else if (binExpr->op == "<") {
                return left < right;
            } else if (binExpr->op == ">") {
                return left > right;
            } else if (binExpr->op == "<=") {
                return left <= right;
            } else if (binExpr->op == ">=") {
                return left >= right;
            } else if (binExpr->op == "==") {
                return left == right;
            } else if (binExpr->op == "!=") {
                return left != right;
            } else if (binExpr->op == "**") {
                return static_cast<int>(std::pow(left, right));
            } else if (binExpr->op == "*/") {
                return static_cast<int>(std::sqrt(left));
            }
        } else if (auto funcCallExpr = dynamic_cast<const FunctionCallExpression*>(expr)) {
            std::vector<int> args;
            for (const auto& arg : funcCallExpr->arguments) {
                args.push_back(evaluateInt(arg.get()));
            }
            // Handle function calls
        } else if (auto readExpr = dynamic_cast<const ReadExpression*>(expr)) {
            std::string input;
            std::getline(std::cin, input);
            if (isNumber(input)) {
                return std::stoi(input);
            } else {
                throw std::runtime_error("Invalid input for integer conversion");
            }
        }
        return 0; // Default return for unknown expressions
    }

    double evaluateDouble(const Expression* expr) {
        if (auto numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
            return numberExpr->value;
        } else if (auto varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            if (variables.find(varExpr->name) != variables.end()) {
                auto value = variables[varExpr->name];
                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                } else if (std::holds_alternative<int>(value)) {
                    return static_cast<double>(std::get<int>(value));
                } else if (std::holds_alternative<std::string>(value)) {
                    try {
                        return std::stod(std::get<std::string>(value));
                    } catch (const std::invalid_argument&) {
                        throw std::runtime_error("Cannot convert string to double: " + std::get<std::string>(value));
                    }
                } else if (std::holds_alternative<bool>(value)) {
                    return std::get<bool>(value) ? 1.0 : 0.0;
                }
            } else {
                throw std::runtime_error("Undefined variable: " + varExpr->name);
            }
        } else if (auto binExpr = dynamic_cast<const BinaryExpression*>(expr)) {
            double left = evaluateDouble(binExpr->left.get());
            double right = evaluateDouble(binExpr->right.get());
            if (binExpr->op == "+") {
                return left + right;
            } else if (binExpr->op == "-") {
                return left - right;
            } else if (binExpr->op == "*") {
                return left * right;
            } else if (binExpr->op == "/") {
                return left / right;
            } else if (binExpr->op == "<") {
                return left < right;
            } else if (binExpr->op == ">") {
                return left > right;
            } else if (binExpr->op == "<=") {
                return left <= right;
            } else if (binExpr->op == ">=") {
                return left >= right;
            } else if (binExpr->op == "==") {
                return left == right;
            } else if (binExpr->op == "!=") {
                return left != right;
            } else if (binExpr->op == "**") {
                return std::pow(left, right);
            } else if (binExpr->op == "*/") {
                return std::sqrt(left);
            }
        }
        return 0.0; // Default return for unknown expressions
    }

    std::string evaluateString(const Expression* expr) {
        if (auto strExpr = dynamic_cast<const StringExpression*>(expr)) {
            return strExpr->value;
        } else if (auto varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            if (variables.find(varExpr->name) != variables.end()) {
                return std::get<std::string>(variables[varExpr->name]);
            } else {
                throw std::runtime_error("Undefined variable: " + varExpr->name);
            }
        }
        return ""; // Default return for unknown expressions
    }

    bool isNumber(const std::string& s) {
        return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
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
