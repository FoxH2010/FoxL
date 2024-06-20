#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include <utility>

class ASTNode {
public:
    int line;
    virtual ~ASTNode() = default;
    virtual void print() const = 0;
    ASTNode(int line) : line(line) {}
};

class Expression : public ASTNode {
public:
    std::string name;
    using ASTNode::ASTNode;

    Expression(int line) : ASTNode(line), name("") {}
    Expression(std::string name, int line) : ASTNode(line), name(std::move(name)) {}
};

class Statement : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class WriteStatement : public Statement {
public:
    std::unique_ptr<Expression> messageExpr;

    explicit WriteStatement(std::unique_ptr<Expression> messageExpr, int line)
        : Statement(line), messageExpr(std::move(messageExpr)) {}

    void print() const override {
        std::cout << "WriteStatement(";
        messageExpr->print();
        std::cout << ", line: " << line << ")" << std::endl;
    }
};

class ReadExpression : public Expression {
public:
    std::unique_ptr<Expression> prompt;

    ReadExpression(std::unique_ptr<Expression> prompt, int line)
        : Expression(line), prompt(std::move(prompt)) {}

    void print() const override {
        std::cout << "ReadExpression(";
        if (prompt) {
            prompt->print();
        }
        std::cout << ", line: " << line << ")" << std::endl;
    }
};

class VariableDeclaration : public Statement {
public:
    std::string type;
    std::string name;
    std::unique_ptr<Expression> initializer;

    VariableDeclaration(std::string type, std::string name, std::unique_ptr<Expression> initializer, int line)
        : Statement(line), type(std::move(type)), name(std::move(name)), initializer(std::move(initializer)) {}

    void print() const override {
        std::cout << "VariableDeclaration(" << type << " " << name << ", line: " << line << ")" << std::endl;
        if (initializer) {
            initializer->print();
        }
    }
};

class NumberExpression : public Expression {
public:
    double value;

    NumberExpression(double value, int line) : Expression(line), value(value) {}

    void print() const override {
        std::cout << "NumberExpression(" << value << ", line: " << line << ")" << std::endl;
    }
};

class StringExpression : public Expression {
public:
    std::string value;

    StringExpression(std::string value, int line) : Expression(line), value(std::move(value)) {}

    void print() const override {
        std::cout << "StringExpression(" << value << ", line: " << line << ")" << std::endl;
    }
};

class BoolExpression : public Expression {
public:
    bool value;

    BoolExpression(bool value, int line) : Expression(line), value(value) {}

    void print() const override {
        std::cout << "BoolExpression(" << value << ", line: " << line << ")" << std::endl;
    }
};

class ArrayExpression : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;

    ArrayExpression(std::vector<std::unique_ptr<Expression>> elements, int line)
        : Expression(line), elements(std::move(elements)) {}

    void print() const override {
        std::cout << "ArrayExpression, line: " << line << std::endl;
        for (const auto& elem : elements) {
            elem->print();
        }
    }
};

class VariableExpression : public Expression {
public:
    VariableExpression(std::string name, int line) : Expression(std::move(name), line) {}

    void print() const override {
        std::cout << "VariableExpression(" << name << ", line: " << line << ")" << std::endl;
    }
};

class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;

    BinaryExpression(std::unique_ptr<Expression> left, std::string op, std::unique_ptr<Expression> right, int line)
        : Expression(line), left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    void print() const override {
        std::cout << "BinaryExpression(" << op << ", line: " << line << ")" << std::endl;
        left->print();
        right->print();
    }
};

class UnaryExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> operand;

    UnaryExpression(std::string op, std::unique_ptr<Expression> operand, int line)
        : Expression(line), op(std::move(op)), operand(std::move(operand)) {}

    void print() const override {
        std::cout << "UnaryExpression(" << op << ", line: " << line << ")" << std::endl;
        operand->print();
    }
};

class FunctionCallExpression : public Expression {
public:
    std::string functionName;
    std::vector<std::unique_ptr<Expression>> arguments;

    FunctionCallExpression(std::string functionName, std::vector<std::unique_ptr<Expression>> arguments, int line)
        : Expression(line), functionName(std::move(functionName)), arguments(std::move(arguments)) {}

    void print() const override {
        std::cout << "FunctionCallExpression(" << functionName << ", line: " << line << ")" << std::endl;
        for (const auto& arg : arguments) {
            arg->print();
        }
    }
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    ReturnStatement(std::unique_ptr<Expression> expression, int line) : Statement(line), expression(std::move(expression)) {}

    void print() const override {
        std::cout << "ReturnStatement, line: " << line << std::endl;
        expression->print();
    }
};

class ForStatement : public Statement {
public:
    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> increment;
    std::unique_ptr<Statement> body;

    ForStatement(std::unique_ptr<Statement> initializer, std::unique_ptr<Expression> condition,
                 std::unique_ptr<Statement> increment, std::unique_ptr<Statement> body, int line)
        : Statement(line), initializer(std::move(initializer)), condition(std::move(condition)),
          increment(std::move(increment)), body(std::move(body)) {}

    void print() const override {
        std::cout << "ForStatement, line: " << line << std::endl;
        if (initializer) initializer->print();
        if (condition) condition->print();
        if (increment) increment->print();
        if (body) body->print();
    }
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body, int line)
        : Statement(line), condition(std::move(condition)), body(std::move(body)) {}

    void print() const override {
        std::cout << "WhileStatement, line: " << line << std::endl;
        if (condition) condition->print();
        if (body) body->print();
    }
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    BlockStatement(std::vector<std::unique_ptr<Statement>> statements, int line)
        : Statement(line), statements(std::move(statements)) {}

    void print() const override {
        std::cout << "BlockStatement, line: " << line << std::endl;
        for (const auto& stmt : statements) {
            stmt->print();
        }
    }
};

class IncludeStatement : public Statement {
public:
    std::string fileName;

    IncludeStatement(std::string fileName, int line) : Statement(line), fileName(std::move(fileName)) {}

    void print() const override {
        std::cout << "IncludeStatement(" << fileName << ", line: " << line << ")" << std::endl;
    }
};

class IndexExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    IndexExpression(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index, int line)
        : Expression(line), array(std::move(array)), index(std::move(index)) {}

    void print() const override {
        std::cout << "IndexExpression(";
        array->print();
        std::cout << ", ";
        index->print();
        std::cout << ", line: " << line << ")" << std::endl;
    }
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::unique_ptr<Statement>> body;

    FunctionDeclaration(std::string name, std::vector<std::string> parameters, std::vector<std::unique_ptr<Statement>> body, int line)
        : Statement(line), name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}

    void print() const override {
        std::cout << "FunctionDeclaration(" << name << ", line: " << line << ")" << std::endl;
        for (const auto& param : parameters) {
            std::cout << "Param(" << param << ")" << std::endl;
        }
        for (const auto& stmt : body) {
            stmt->print();
        }
    }
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;

    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> thenBranch, std::unique_ptr<Statement> elseBranch, int line)
        : Statement(line), condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    void print() const override {
        std::cout << "IfStatement, line: " << line << std::endl;
        condition->print();
        thenBranch->print();
        if (elseBranch) {
            elseBranch->print();
        }
    }
};

class Parser {
public:
    explicit Parser(Lexer &lexer) : lexer(lexer), currentToken(lexer.getNextToken()) {}

    std::unique_ptr<ASTNode> parse() {
        return parseStatement();
    }

private:
    Lexer &lexer;
    Token currentToken;

    void advance() {
        currentToken = lexer.getNextToken();
    }

    std::unique_ptr<ASTNode> parseStatement() {
        if (currentToken.type == TokenType::EndOfFile) {
            return nullptr; // Handle end of file
        }

        if (currentToken.type == TokenType::Keyword) {
            if (currentToken.value == "write" && peekNextToken().value == "(") {
                return parseWriteStatement();
            } else if (currentToken.value == "func") {
                return parseFunctionDeclaration();
            } else if (currentToken.value == "if") {
                return parseIfStatement();
            } else if (currentToken.value == "return") {
                return parseReturnStatement();
            } else if (currentToken.value == "for") {
                return parseForStatement();
            } else if (currentToken.value == "while") {
                return parseWhileStatement();
            } else if (currentToken.value == "include" && peekNextToken().type == TokenType::StringLiteral) {
                return parseIncludeStatement();
            } else if (currentToken.value == "let" || currentToken.value == "const") {
                return parseLetDeclaration();
            } else {
                throw std::runtime_error("Unexpected keyword: " + currentToken.value + " at line " + std::to_string(currentToken.line));
            }
        } else if (currentToken.type == TokenType::Identifier) {
            return parseExpressionStatement();
        } else {
            throw std::runtime_error("Unexpected token: " + currentToken.value + " at line " + std::to_string(currentToken.line));
        }
    }

    std::unique_ptr<Statement> castToStatement(std::unique_ptr<ASTNode> node) {
        return std::unique_ptr<Statement>(dynamic_cast<Statement*>(node.release()));
    }

    std::unique_ptr<ASTNode> parseWriteStatement() {
        int line = currentToken.line;
        advance(); // consume 'write'
        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'write' at line " + std::to_string(line));
        }
        advance(); // consume '('

        auto messageExpr = parseExpression();

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after message or variable at line " + std::to_string(line));
        }
        advance(); // consume ')'

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        } else if (currentToken.value != "}" && currentToken.type != TokenType::EndOfFile) {
            throw std::runtime_error("Expected ';' after 'write' statement at line " + std::to_string(line));
        }

        return std::make_unique<WriteStatement>(std::move(messageExpr), line);
    }

    std::unique_ptr<ASTNode> parseVariableDeclaration() {
        int line = currentToken.line;
        std::string type = currentToken.value;
        advance(); // consume type

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected variable name in declaration at line " + std::to_string(line));
        }
        std::string name = currentToken.value;
        advance(); // consume variable name

        lexer.registerIdentifier(name); // Register the identifier in the lexer

        std::unique_ptr<Expression> initializer;
        if (currentToken.value == "=") {
            advance(); // consume '='
            initializer = parseExpression();
        }

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        } else if (currentToken.value != "}" && currentToken.type != TokenType::EndOfFile) {
            throw std::runtime_error("Expected ';' after variable declaration at line " + std::to_string(line));
        }

        return std::make_unique<VariableDeclaration>(type, name, std::move(initializer), line);
    }

    std::unique_ptr<ASTNode> parseLetDeclaration() {
        int line = currentToken.line;
        advance(); // consume 'let' or 'const'

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected variable name in let/const declaration at line " + std::to_string(line));
        }
        std::string name = currentToken.value;
        advance(); // consume variable name

        lexer.registerIdentifier(name); // Register the identifier in the lexer

        std::unique_ptr<Expression> initializer;
        if (currentToken.value == "=") {
            advance(); // consume '='
            initializer = parseExpression();
        }

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        } else if (currentToken.value != "}" && currentToken.type != TokenType::EndOfFile) {
            throw std::runtime_error("Expected ';' after let/const declaration at line " + std::to_string(line));
        }

        return std::make_unique<VariableDeclaration>("auto", name, std::move(initializer), line);
    }

    std::unique_ptr<Expression> parseExpression() {
        auto left = parsePrimary();

        while (currentToken.type == TokenType::Operator) {
            std::string op = currentToken.value;
            if (op == "++" || op == "--") {
                advance(); // consume operator
                return std::make_unique<UnaryExpression>(op, std::move(left), currentToken.line);
            }
            advance(); // consume operator
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpression>(std::move(left), op, std::move(right), currentToken.line);
        }

        return left;
    }

    std::unique_ptr<Expression> parsePrimary() {
        int line = currentToken.line;

        if (currentToken.type == TokenType::Number) {
            double value = std::stod(currentToken.value);
            advance(); // consume number
            return std::make_unique<NumberExpression>(value, line);
        }

        if (currentToken.type == TokenType::Identifier) {
            std::string name = currentToken.value;
            advance(); // consume identifier

            if (currentToken.type == TokenType::Symbol && currentToken.value == "[") {
                advance(); // consume '['
                auto indexExpr = parseExpression();
                if (currentToken.value != "]") {
                    throw std::runtime_error("Expected ']' after index at line " + std::to_string(currentToken.line));
                }
                advance(); // consume ']'
                return std::make_unique<IndexExpression>(std::make_unique<VariableExpression>(name, line), std::move(indexExpr), line);
            }

            if (currentToken.type == TokenType::Symbol && currentToken.value == "(") {
                // Function call parsing
                advance(); // consume '('
                std::vector<std::unique_ptr<Expression>> arguments;
                while (currentToken.type != TokenType::Symbol || currentToken.value != ")") {
                    arguments.push_back(parseExpression());
                    if (currentToken.type == TokenType::Symbol && currentToken.value == ",") {
                        advance(); // consume ','
                    } else if (currentToken.type == TokenType::Symbol && currentToken.value == ")") {
                        break;
                    } else {
                        throw std::runtime_error("Expected ',' or ')' in function call at line " + std::to_string(currentToken.line));
                    }
                }
                advance(); // consume ')'
                return std::make_unique<FunctionCallExpression>(name, std::move(arguments), line);
            }

            return std::make_unique<VariableExpression>(name, line);
        }

        if (currentToken.type == TokenType::StringLiteral) {
            std::string value = currentToken.value;
            advance(); // consume string literal
            return std::make_unique<StringExpression>(value, line);
        }

        if (currentToken.type == TokenType::Keyword && (currentToken.value == "true" || currentToken.value == "false")) {
            bool value = (currentToken.value == "true");
            advance(); // consume 'true' or 'false'
            return std::make_unique<BoolExpression>(value, line);
        }

        if (currentToken.type == TokenType::Symbol && currentToken.value == "[") {
            // Array or list parsing
            advance(); // consume '['
            std::vector<std::unique_ptr<Expression>> elements;
            while (currentToken.type != TokenType::Symbol || currentToken.value != "]") {
                elements.push_back(parseExpression());
                if (currentToken.type == TokenType::Symbol && currentToken.value == ",") {
                    advance(); // consume ','
                } else if (currentToken.type == TokenType::Symbol && currentToken.value == "]") {
                    break;
                } else {
                    throw std::runtime_error("Expected ',' or ']' in array or list at line " + std::to_string(currentToken.line));
                }
            }
            advance(); // consume ']'
            return std::make_unique<ArrayExpression>(std::move(elements), line);
        }

        if (currentToken.type == TokenType::Keyword && currentToken.value == "read") {
            advance(); // consume 'read'
            if (currentToken.value != "(") {
                throw std::runtime_error("Expected '(' after 'read' at line " + std::to_string(currentToken.line));
            }
            advance(); // consume '('
            std::unique_ptr<Expression> prompt = nullptr;
            if (currentToken.type != TokenType::Symbol || currentToken.value != ")") {
                prompt = parseExpression();
            }
            if (currentToken.value != ")") {
                throw std::runtime_error("Expected ')' after 'read' argument at line " + std::to_string(currentToken.line));
            }
            advance(); // consume ')'
            return std::make_unique<ReadExpression>(std::move(prompt), line);
        }

        throw std::runtime_error("Expected primary expression at line " + std::to_string(currentToken.line));
    }

    std::unique_ptr<ASTNode> parseFunctionDeclaration() {
        int line = currentToken.line;
        advance(); // consume 'func'

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected function name in declaration at line " + std::to_string(line));
        }
        std::string name = currentToken.value;
        advance(); // consume function name

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after function name at line " + std::to_string(line));
        }
        advance(); // consume '('

        std::vector<std::string> parameters;
        while (currentToken.value != ")") {
            if (currentToken.type != TokenType::Identifier) {
                throw std::runtime_error("Expected parameter name at line " + std::to_string(line));
            }
            parameters.push_back(currentToken.value);
            advance(); // consume parameter

            if (currentToken.value == ",") {
                advance(); // consume ','
            } else if (currentToken.value != ")") {
                throw std::runtime_error("Expected ',' or ')' after parameter at line " + std::to_string(line));
            }
        }
        advance(); // consume ')'

        if (currentToken.value != "{") {
            throw std::runtime_error("Expected '{' before function body at line " + std::to_string(line));
        }
        advance(); // consume '{'

        std::vector<std::unique_ptr<Statement>> body;
        while (currentToken.value != "}") {
            body.push_back(castToStatement(parseStatement()));
        }
        advance(); // consume '}'

        return std::make_unique<FunctionDeclaration>(name, std::move(parameters), std::move(body), line);
    }

    std::unique_ptr<ASTNode> parseIfStatement() {
        int line = currentToken.line;
        advance(); // consume 'if'

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'if' at line " + std::to_string(line));
        }
        advance(); // consume '('

        auto condition = parseExpression();

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after condition at line " + std::to_string(line));
        }
        advance(); // consume ')'

        auto thenBranch = castToStatement(parseBlock());
        std::unique_ptr<Statement> elseBranch = nullptr;

        if (currentToken.value == "else") {
            advance(); // consume 'else'
            elseBranch = castToStatement(parseBlock());
        }

        return std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line);
    }

    std::unique_ptr<Statement> parseReturnStatement() {
        int line = currentToken.line;
        advance(); // consume 'return'

        auto expression = parseExpression();

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        } else if (currentToken.value != "}" && currentToken.type != TokenType::EndOfFile) {
            throw std::runtime_error("Expected ';' after 'return' statement at line " + std::to_string(line));
        }

        return std::make_unique<ReturnStatement>(std::move(expression), line);
    }

    std::unique_ptr<Statement> parseForStatement() {
        int line = currentToken.line;
        advance(); // consume 'for'

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'for' at line " + std::to_string(line));
        }
        advance(); // consume '('

        auto initializer = castToStatement(parseStatement());

        if (currentToken.value != ";") {
            throw std::runtime_error("Expected ';' after initializer in 'for' statement at line " + std::to_string(line));
        }
        advance(); // consume ';'

        auto condition = parseExpression();

        if (currentToken.value != ";") {
            throw std::runtime_error("Expected ';' after condition in 'for' statement at line " + std::to_string(line));
        }
        advance(); // consume ';'

        auto increment = castToStatement(parseExpressionStatement());

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after increment in 'for' statement at line " + std::to_string(line));
        }
        advance(); // consume ')'

        auto body = castToStatement(parseBlock());

        return std::make_unique<ForStatement>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), line);
    }

    std::unique_ptr<Statement> parseWhileStatement() {
        int line = currentToken.line;
        advance(); // consume 'while'

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'while' at line " + std::to_string(line));
        }
        advance(); // consume '('

        auto condition = parseExpression();

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after condition at line " + std::to_string(line));
        }
        advance(); // consume ')'

        auto body = castToStatement(parseBlock());

        return std::make_unique<WhileStatement>(std::move(condition), std::move(body), line);
    }

    std::unique_ptr<Statement> parseIncludeStatement() {
        int line = currentToken.line;
        advance(); // consume 'include'

        if (currentToken.type != TokenType::StringLiteral) {
            throw std::runtime_error("Expected string literal after 'include' at line " + std::to_string(line));
        }

        std::string fileName = currentToken.value;
        advance(); // consume string literal

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        } else if (currentToken.value != "}" && currentToken.type != TokenType::EndOfFile) {
            throw std::runtime_error("Expected ';' after 'include' statement at line " + std::to_string(line));
        }

        return std::make_unique<IncludeStatement>(fileName, line);
    }

    std::unique_ptr<ASTNode> parseBlock() {
        if (currentToken.value != "{") {
            return parseStatement();
        }

        int line = currentToken.line;
        advance(); // consume '{'

        std::vector<std::unique_ptr<Statement>> statements;
        while (currentToken.value != "}") {
            statements.push_back(castToStatement(parseStatement()));
        }
        advance(); // consume '}'

        return std::make_unique<BlockStatement>(std::move(statements), line);
    }

    std::unique_ptr<ASTNode> parseExpressionStatement() {
        auto expr = parseExpression();

        if (currentToken.type == TokenType::EndOfFile || currentToken.value == ";") {
            if (currentToken.type != TokenType::EndOfFile) {
                advance(); // consume ';'
            }
            return expr;
        } else {
            throw std::runtime_error("Expected ';' after expression statement at line " + std::to_string(currentToken.line));
        }
    }

    Token peekNextToken() {
        size_t savedPosition = lexer.position;
        int savedLine = lexer.line;
        Token nextToken = lexer.getNextToken();
        lexer.position = savedPosition;
        lexer.line = savedLine;
        return nextToken;
    }
};
