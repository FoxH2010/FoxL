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
#include <filesystem>

class ReturnException : public std::runtime_error {
public:
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> value;

    ReturnException(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& value)
        : std::runtime_error("Return"), value(value) {}
};

class Interpreter {
public:
    Interpreter(const std::string& scriptFileName) {
        std::string baseFileName = scriptFileName.substr(0, scriptFileName.find_last_of('.'));
        dataFileName = baseFileName + ".FoxLData.foxl";
        loadVariablesFromFile();
    }

    ~Interpreter() {
        saveVariablesToFile();
        std::filesystem::remove(dataFileName);
    }

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
    std::string dataFileName;

    void execute(const Statement* statement);

    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluate(const Expression* expr);

    void loadVariablesFromFile() {
        std::ifstream inFile(dataFileName);
        if (inFile) {
            std::string line;
            while (std::getline(inFile, line)) {
                std::istringstream iss(line);
                std::string type, name;
                iss >> type >> name;

                if (type == "variable" || type == "constant") {
                    bool isConstant = (type == "constant");
                    std::string valueStr;
                    std::getline(iss, valueStr);
                    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> value;

                    if (valueStr[0] == '[') {
                        if (valueStr.find("\"") != std::string::npos) {
                            std::vector<std::string> strArray;
                            valueStr.erase(0, 1);
                            valueStr.pop_back();
                            std::istringstream arrayStream(valueStr);
                            std::string element;
                            while (std::getline(arrayStream, element, ',')) {
                                element.erase(0, 1);
                                element.pop_back();
                                strArray.push_back(element);
                            }
                            value = strArray;
                        } else { // Integer array
                            std::vector<int> intArray;
                            valueStr.erase(0, 1);
                            valueStr.pop_back();
                            std::istringstream arrayStream(valueStr);
                            std::string element;
                            while (std::getline(arrayStream, element, ',')) {
                                intArray.push_back(std::stoi(element));
                            }
                            value = intArray;
                        }
                    } else if (valueStr == "true" || valueStr == "false") {
                        value = (valueStr == "true");
                    } else if (valueStr.find('.') != std::string::npos) {
                        value = std::stod(valueStr);
                    } else {
                        value = std::stoi(valueStr);
                    }

                    variables[name] = { value, isConstant };
                }
            }
        }
    }

    void saveVariablesToFile() {
        std::ofstream outFile(dataFileName);
        for (const auto& [name, variable] : variables) {
            outFile << (variable.isConstant ? "constant" : "variable") << " " << name << " ";
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, int>) {
                    outFile << arg;
                } else if constexpr (std::is_same_v<T, double>) {
                    outFile << arg;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    outFile << arg;
                } else if constexpr (std::is_same_v<T, bool>) {
                    outFile << (arg ? "true" : "false");
                } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                    outFile << "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        outFile << arg[i];
                        if (i < arg.size() - 1) {
                            outFile << ",";
                        }
                    }
                    outFile << "]";
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    outFile << "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        outFile << "\"" << arg[i] << "\"";
                        if (i < arg.size() - 1) {
                            outFile << ",";
                        }
                    }
                    outFile << "]";
                }
            }, variable.value);
            outFile << std::endl;
        }
    }

    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateNumberExpression(const NumberExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateStringExpression(const StringExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateBoolExpression(const BoolExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateVariableExpression(const VariableExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateBinaryExpression(const BinaryExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateReadExpression(const ReadExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateIndexExpression(const IndexExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateArrayExpression(const ArrayExpression* expr);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> evaluateFunctionCallExpression(const FunctionCallExpression* expr);

    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleAssignment(const BinaryExpression* expr, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleArrayAssignment(const IndexExpression* indexExpr, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleAddition(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleSubtraction(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleMultiplication(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleDivision(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleLessThan(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleGreaterThan(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleLessThanOrEqual(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);
    std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> handleGreaterThanOrEqual(const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& left, const std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>>& right);

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

            std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> value;
            if (varDecl->initializer) {
                value = evaluate(varDecl->initializer.get());
            } else {
                value = 0;
            }
            variables[varDecl->name] = { value, varDecl->type == "const" };

            std::ofstream outFile(dataFileName, std::ios_base::app);
            outFile << (varDecl->type == "const" ? "constant" : "variable") << " " << varDecl->name << " ";
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, int>) {
                    outFile << arg;
                } else if constexpr (std::is_same_v<T, double>) {
                    outFile << arg;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    outFile << arg;
                } else if constexpr (std::is_same_v<T, bool>) {
                    outFile << (arg ? "true" : "false");
                } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                    outFile << "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        outFile << arg[i];
                        if (i < arg.size() - 1) {
                            outFile << ",";
                        }
                    }
                    outFile << "]";
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    outFile << "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        outFile << "\"" << arg[i] << "\"";
                        if (i < arg.size() - 1) {
                            outFile << ",";
                        }
                    }
                    outFile << "]";
                }
            }, value);
            outFile << std::endl;
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
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Error executing statement: " << e.what() << std::endl;
    }
}

std::variant<int, double, std::string, bool, std::vector<int>, std::vector<std::string>> Interpreter::evaluate(const Expression* expr) {
    if (const auto* numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
        return static_cast<int>(numberExpr->value); // Convert double to int for consistency
    } else if (const auto* strExpr = dynamic_cast<const StringExpression*>(expr)) {
        return strExpr->value;
    } else if (const auto* boolExpr = dynamic_cast<const BoolExpression*>(expr)) {
        return boolExpr->value;
    } else if (const auto* varExpr = dynamic_cast<const VariableExpression*>(expr)) {
        if (variables.find(varExpr->name) != variables.end()) {
            return variables[varExpr->name].value;
        } else {
            throw std::runtime_error("Undefined variable: " + varExpr->name);
        }
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

    std::stringstream ss;
    ss << "Unsupported expression type: " << typeid(*expr).name() << " at line " << std::to_string(expr->line);
    throw std::runtime_error(ss.str());
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
    }
    throw std::runtime_error("Unsupported statement type");
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
    }
    throw std::runtime_error("Unsupported expression type");
}
