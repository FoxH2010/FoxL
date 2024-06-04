#include "parser.cpp"
#include <unordered_map>
#include <functional>
#include <cmath>
#include <stdexcept>

class Interpreter {
public:
    void interpret(const std::vector<std::unique_ptr<Statement>>& statements) {
        for (const auto& statement : statements) {
            execute(statement.get());
        }
    }

private:
    std::unordered_map<std::string, int> intVariables;
    std::unordered_map<std::string, double> doubleVariables;
    std::unordered_map<std::string, std::string> strVariables;
    std::unordered_map<std::string, bool> boolVariables;
    std::unordered_map<std::string, std::vector<int>> intArrays;
    std::unordered_map<std::string, std::vector<int>> intLists;
    std::unordered_map<std::string, std::function<int(const std::vector<int>&)>> functions;

    void execute(const Statement* statement) {
        if (auto writeStmt = dynamic_cast<const WriteStatement*>(statement)) {
            if (writeStmt->isVariable) {
                std::cout << intVariables[writeStmt->message] << std::endl;
            } else {
                std::cout << writeStmt->message << std::endl;
            }
        } else if (auto varDecl = dynamic_cast<const VariableDeclaration*>(statement)) {
            if (varDecl->type == "int") {
                int value = 0;
                if (varDecl->initializer) {
                    value = evaluate(varDecl->initializer.get());
                }
                intVariables[varDecl->name] = value;
            } else if (varDecl->type == "double") {
                double value = 0.0;
                if (varDecl->initializer) {
                    value = evaluate(varDecl->initializer.get());
                }
                doubleVariables[varDecl->name] = value;
            } else if (varDecl->type == "str") {
                std::string value;
                if (varDecl->initializer) {
                    value = evaluateString(varDecl->initializer.get());
                }
                strVariables[varDecl->name] = value;
            } else if (varDecl->type == "bool") {
                bool value = false;
                if (varDecl->initializer) {
                    value = evaluate(varDecl->initializer.get()) != 0;
                }
                boolVariables[varDecl->name] = value;
            }
        } else if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(statement)) {
            functions[funcDecl->name] = [this, funcDecl](const std::vector<int>& args) -> int {
                std::unordered_map<std::string, int> oldIntVars = intVariables;
                for (size_t i = 0; i < funcDecl->parameters.size(); ++i) {
                    intVariables[funcDecl->parameters[i]] = args[i];
                }
                for (const auto& stmt : funcDecl->body) {
                    if (auto retStmt = dynamic_cast<ReturnStatement*>(stmt.get())) {
                        int result = evaluate(retStmt->expression.get());
                        intVariables = oldIntVars;
                        return result;
                    }
                    execute(stmt.get());
                }
                intVariables = oldIntVars;
                return 0; // Default return value if no return statement is found
            };
        } else if (auto ifStmt = dynamic_cast<const IfStatement*>(statement)) {
            if (evaluate(ifStmt->condition.get())) {
                execute(ifStmt->thenBranch.get());
            } else if (ifStmt->elseBranch) {
                execute(ifStmt->elseBranch.get());
            }
        } else if (auto forStmt = dynamic_cast<const ForStatement*>(statement)) {
            execute(forStmt->initializer.get());
            while (evaluate(forStmt->condition.get())) {
                execute(forStmt->body.get());
                execute(forStmt->increment.get());
            }
        } else if (auto returnStmt = dynamic_cast<const ReturnStatement*>(statement)) {
            evaluate(returnStmt->expression.get());
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

    int evaluate(const Expression* expr) {
        if (auto numberExpr = dynamic_cast<const NumberExpression*>(expr)) {
            return static_cast<int>(numberExpr->value);
        } else if (auto varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            return intVariables[varExpr->name];
        } else if (auto binExpr = dynamic_cast<const BinaryExpression*>(expr)) {
            int left = evaluate(binExpr->left.get());
            int right = evaluate(binExpr->right.get());
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
                args.push_back(evaluate(arg.get()));
            }
            return functions[funcCallExpr->functionName](args);
        } else if (auto readExpr = dynamic_cast<const ReadExpression*>(expr)) {
            std::string input;
            std::cin >> input;
            return std::stoi(input);
        }
        return 0; // Default return for unknown expressions
    }

    std::string evaluateString(const Expression* expr) {
        if (auto strExpr = dynamic_cast<const StringExpression*>(expr)) {
            return strExpr->value;
        } else if (auto varExpr = dynamic_cast<const VariableExpression*>(expr)) {
            return strVariables[varExpr->name];
        }
        return ""; // Default return for unknown expressions
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_name.foxl>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
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

        Interpreter interpreter;
        interpreter.interpret(statements);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
