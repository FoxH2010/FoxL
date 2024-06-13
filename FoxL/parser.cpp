#include <memory>
#include <vector>
#include <iostream>

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print() const = 0;
};

class Expression : public ASTNode {
};

class IndexExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    IndexExpression(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index)
        : array(std::move(array)), index(std::move(index)) {}

    void print() const override {
        std::cout << "IndexExpression(";
        array->print();
        std::cout << ", ";
        index->print();
        std::cout << ")" << std::endl;
    }
};

class Statement : public ASTNode {
};

class WriteStatement : public Statement {
public:
    std::unique_ptr<Expression> messageExpr;
    bool isVariable;

    explicit WriteStatement(std::unique_ptr<Expression> messageExpr)
        : messageExpr(std::move(messageExpr)), isVariable(false) {}

    void print() const override {
        std::cout << "WriteStatement(";
        messageExpr->print();
        std::cout << ")" << std::endl;
    }
};

class ReadExpression : public Expression {
public:
    std::unique_ptr<Expression> prompt;

    ReadExpression(std::unique_ptr<Expression> prompt = nullptr)
        : prompt(std::move(prompt)) {}

    void print() const override {
        std::cout << "ReadExpression(";
        if (prompt) {
            prompt->print();
        }
        std::cout << ")" << std::endl;
    }
};

class VariableDeclaration : public Statement {
public:
    std::string type;
    std::string name;
    std::unique_ptr<Expression> initializer;

    VariableDeclaration(std::string type, std::string name, std::unique_ptr<Expression> initializer)
        : type(std::move(type)), name(std::move(name)), initializer(std::move(initializer)) {}

    void print() const override {
        std::cout << "VariableDeclaration(" << type << " " << name << ")" << std::endl;
        if (initializer) {
            initializer->print();
        }
    }
};

class NumberExpression : public Expression {
public:
    double value;

    explicit NumberExpression(double value) : value(value) {}

    void print() const override {
        std::cout << "NumberExpression(" << value << ")" << std::endl;
    }
};

class StringExpression : public Expression {
public:
    std::string value;

    explicit StringExpression(std::string value) : value(std::move(value)) {}

    void print() const override {
        std::cout << "StringExpression(" << value << ")" << std::endl;
    }
};

class BoolExpression : public Expression {
public:
    bool value;

    explicit BoolExpression(bool value) : value(value) {}

    void print() const override {
        std::cout << "BoolExpression(" << value << ")" << std::endl;
    }
};

class ArrayExpression : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;

    explicit ArrayExpression(std::vector<std::unique_ptr<Expression>> elements)
        : elements(std::move(elements)) {}

    void print() const override {
        std::cout << "ArrayExpression" << std::endl;
        for (const auto& elem : elements) {
            elem->print();
        }
    }
};

class ListExpression : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;

    explicit ListExpression(std::vector<std::unique_ptr<Expression>> elements)
        : elements(std::move(elements)) {}

    void print() const override {
        std::cout << "ListExpression" << std::endl;
        for (const auto& elem : elements) {
            elem->print();
        }
    }
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::unique_ptr<Statement>> body;

    FunctionDeclaration(std::string name, std::vector<std::string> parameters, std::vector<std::unique_ptr<Statement>> body)
        : name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}

    void print() const override {
        std::cout << "FunctionDeclaration(" << name << ")" << std::endl;
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

    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> thenBranch, std::unique_ptr<Statement> elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    void print() const override {
        std::cout << "IfStatement" << std::endl;
        condition->print();
        thenBranch->print();
        if (elseBranch) {
            elseBranch->print();
        }
    }
};

class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;

    BinaryExpression(std::unique_ptr<Expression> left, std::string op, std::unique_ptr<Expression> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    void print() const override {
        std::cout << "BinaryExpression(" << op << ")" << std::endl;
        left->print();
        right->print();
    }
};

class VariableExpression : public Expression {
public:
    std::string name;

    explicit VariableExpression(std::string name) : name(std::move(name)) {}

    void print() const override {
        std::cout << "VariableExpression(" << name << ")" << std::endl;
    }
};

class FunctionCallExpression : public Expression {
public:
    std::string functionName;
    std::vector<std::unique_ptr<Expression>> arguments;

    FunctionCallExpression(std::string functionName, std::vector<std::unique_ptr<Expression>> arguments)
        : functionName(std::move(functionName)), arguments(std::move(arguments)) {}

    void print() const override {
        std::cout << "FunctionCallExpression(" << functionName << ")" << std::endl;
        for (const auto& arg : arguments) {
            arg->print();
        }
    }
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    explicit ReturnStatement(std::unique_ptr<Expression> expression) : expression(std::move(expression)) {}

    void print() const override {
        std::cout << "ReturnStatement" << std::endl;
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
                 std::unique_ptr<Statement> increment, std::unique_ptr<Statement> body)
        : initializer(std::move(initializer)), condition(std::move(condition)),
          increment(std::move(increment)), body(std::move(body)) {}

    void print() const override {
        std::cout << "ForStatement" << std::endl;
        if (initializer) initializer->print();
        if (condition) condition->print();
        if (increment) increment->print();
        if (body) body->print();
    }
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    explicit BlockStatement(std::vector<std::unique_ptr<Statement>> statements)
        : statements(std::move(statements)) {}

    void print() const override {
        std::cout << "BlockStatement" << std::endl;
        for (const auto& stmt : statements) {
            stmt->print();
        }
    }
};

class IncludeStatement : public Statement {
public:
    std::string fileName;

    explicit IncludeStatement(std::string fileName) : fileName(std::move(fileName)) {}

    void print() const override {
        std::cout << "IncludeStatement(" << fileName << ")" << std::endl;
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
            if (currentToken.value == "write") {
                return parseWriteStatement();
            } else if (currentToken.value == "func") {
                return parseFunctionDeclaration();
            } else if (currentToken.value == "if") {
                return parseIfStatement();
            } else if (currentToken.value == "return") {
               
                return parseReturnStatement();
            } else if (currentToken.value == "for") {
                return parseForStatement();
            } else if (currentToken.value == "include") {
                return parseIncludeStatement();
            } else if (currentToken.value == "let") {
                return parseLetDeclaration();
            } else {
                return parseVariableDeclaration();
            }
        } else if (currentToken.type == TokenType::Identifier) {
            return parseExpressionStatement();
        } else {
            throw std::runtime_error("Unexpected token: " + currentToken.value);
        }
    }

    std::unique_ptr<Statement> castToStatement(std::unique_ptr<ASTNode> node) {
        return std::unique_ptr<Statement>(dynamic_cast<Statement*>(node.release()));
    }

    std::unique_ptr<ASTNode> parseWriteStatement() {
        advance(); // consume 'write'
        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'write'");
        }
        advance(); // consume '('

        auto messageExpr = parseExpression();

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after message or variable");
        }
        advance(); // consume ')'

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        }

        return std::make_unique<WriteStatement>(std::move(messageExpr));
    }

    std::unique_ptr<ASTNode> parseVariableDeclaration() {
        std::string type = currentToken.value;
        advance(); // consume type

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected variable name in declaration");
        }
        std::string name = currentToken.value;
        advance(); // consume variable name

        std::unique_ptr<Expression> initializer;
        if (currentToken.value == "=") {
            advance(); // consume '='
            initializer = parseExpression();
        }

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        }

        return std::make_unique<VariableDeclaration>(type, name, std::move(initializer));
    }

    std::unique_ptr<ASTNode> parseLetDeclaration() {
        advance(); // consume 'let'

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected variable name in let declaration");
        }
        std::string name = currentToken.value;
        advance(); // consume variable name

        std::unique_ptr<Expression> initializer;
        if (currentToken.value == "=") {
            advance(); // consume '='
            initializer = parseExpression();
        }

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        }

        return std::make_unique<VariableDeclaration>("auto", name, std::move(initializer));
    }

    std::unique_ptr<Expression> parseExpression() {
        auto left = parsePrimary();

        while (currentToken.type == TokenType::Operator) {
            std::string op = currentToken.value;
            advance(); // consume operator
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpression>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<Expression> parsePrimary() {
        if (currentToken.type == TokenType::Number) {
            double value = std::stod(currentToken.value);
            advance(); // consume number
            return std::make_unique<NumberExpression>(value);
        }

        if (currentToken.type == TokenType::Identifier) {
            std::string name = currentToken.value;
            advance(); // consume identifier

            if (currentToken.type == TokenType::Symbol && currentToken.value == "[") {
                advance(); // consume '['
                auto indexExpr = parseExpression();
                if (currentToken.value != "]") {
                    throw std::runtime_error("Expected ']' after index");
                }
                advance(); // consume ']'
                return std::make_unique<IndexExpression>(std::make_unique<VariableExpression>(name), std::move(indexExpr));
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
                        throw std::runtime_error("Expected ',' or ')' in function call");
                    }
                }
                advance(); // consume ')'
                return std::make_unique<FunctionCallExpression>(name, std::move(arguments));
            }

            return std::make_unique<VariableExpression>(name);
        }

        if (currentToken.type == TokenType::StringLiteral) {
            std::string value = currentToken.value;
            advance(); // consume string literal
            return std::make_unique<StringExpression>(value);
        }

        if (currentToken.type == TokenType::Keyword && currentToken.value == "true") {
            advance(); // consume 'true'
            return std::make_unique<BoolExpression>(true);
        }

        if (currentToken.type == TokenType::Keyword && currentToken.value == "false") {
            advance(); // consume 'false'
            return std::make_unique<BoolExpression>(false);
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
                    throw std::runtime_error("Expected ',' or ']' in array or list");
                }
            }
            advance(); // consume ']'
            return std::make_unique<ArrayExpression>(std::move(elements));
        }

        if (currentToken.type == TokenType::Keyword && currentToken.value == "read") {
            advance(); // consume 'read'
            if (currentToken.value != "(") {
                throw std::runtime_error("Expected '(' after 'read'");
            }
            advance(); // consume '('
            std::unique_ptr<Expression> prompt = nullptr;
            if (currentToken.type != TokenType::Symbol || currentToken.value != ")") {
                prompt = parseExpression();
            }
            if (currentToken.value != ")") {
                throw std::runtime_error("Expected ')' after 'read' argument");
            }
            advance(); // consume ')'
            return std::make_unique<ReadExpression>(std::move(prompt));
        }

        throw std::runtime_error("Expected primary expression");
    }

    std::unique_ptr<ASTNode> parseFunctionDeclaration() {
        advance(); // consume 'func'

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected function name in declaration");
        }
        std::string name = currentToken.value;
        advance(); // consume function name

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after function name");
        }
        advance(); // consume '('

        std::vector<std::string> parameters;
        while (currentToken.value != ")") {
            if (currentToken.type != TokenType::Identifier) {
                throw std::runtime_error("Expected parameter name");
            }
            parameters.push_back(currentToken.value);
            advance(); // consume parameter

            if (currentToken.value == ",") {
                advance(); // consume ','
            } else if (currentToken.value != ")") {
                throw std::runtime_error("Expected ',' or ')' after parameter");
            }
        }
        advance(); // consume ')'

        if (currentToken.value != "{") {
            throw std::runtime_error("Expected '{' before function body");
        }
        advance(); // consume '{'

        std::vector<std::unique_ptr<Statement>> body;
        while (currentToken.value != "}") {
            body.push_back(castToStatement(parseStatement()));
        }
        advance(); // consume '}'

        return std::make_unique<FunctionDeclaration>(name, std::move(parameters), std::move(body));
    }

    std::unique_ptr<ASTNode> parseIfStatement() {
        advance(); // consume 'if'

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'if'");
        }
        advance(); // consume '('

        auto condition = parseExpression();

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after condition");
        }
        advance(); // consume ')'

        auto thenBranch = castToStatement(parseBlock());
        std::unique_ptr<Statement> elseBranch = nullptr;

        if (currentToken.value == "else") {
            advance(); // consume 'else'
            elseBranch = castToStatement(parseBlock());
        }

        return std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    std::unique_ptr<ASTNode> parseReturnStatement() {
        advance(); // consume 'return'

        auto expression = parseExpression();

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        }

        return std::make_unique<ReturnStatement>(std::move(expression));
    }

    std::unique_ptr<ASTNode> parseBlock() {
        if (currentToken.value != "{") {
            return parseStatement();
        }

        advance(); // consume '{'

        std::vector<std::unique_ptr<Statement>> statements;
        while (currentToken.value != "}") {
            statements.push_back(castToStatement(parseStatement()));
        }
        advance(); // consume '}'

        return std::make_unique<BlockStatement>(std::move(statements));
    }

    std::unique_ptr<ASTNode> parseExpressionStatement() {
        auto expr = parseExpression();

        if (currentToken.type == TokenType::EndOfFile || currentToken.value == ";") {
            if (currentToken.type != TokenType::EndOfFile) {
                advance(); // consume ';'
            }
            return expr;
        } else {
            throw std::runtime_error("Expected ';' after expression statement");
        }
    }

    std::unique_ptr<ASTNode> parseForStatement() {
        advance(); // consume 'for'

        if (currentToken.value != "(") {
            throw std::runtime_error("Expected '(' after 'for'");
        }
        advance(); // consume '('

        auto initializer = castToStatement(parseStatement());

        auto condition = parseExpression();
        if (currentToken.value != ";") {
            throw std::runtime_error("Expected ';' after condition in 'for' statement");
        }
        advance(); // consume ';'

        auto increment = castToStatement(parseExpressionStatement());

        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after increment in 'for' statement");
        }
        advance(); // consume ')'

        auto body = castToStatement(parseBlock());

        return std::make_unique<ForStatement>(std::move(initializer), std::move(condition), std::move(increment), std::move(body));
    }

    std::unique_ptr<ASTNode> parseIncludeStatement() {
        advance(); // consume 'include'

        if (currentToken.type != TokenType::StringLiteral) {
            throw std::runtime_error("Expected string literal after 'include'");
        }

        std::string fileName = currentToken.value;
        advance(); // consume string literal

        if (currentToken.value == ";") {
            advance(); // consume optional ';'
        }

        return std::make_unique<IncludeStatement>(fileName);
    }
};
