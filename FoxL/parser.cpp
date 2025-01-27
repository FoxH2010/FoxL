#include "parser.h"
#include <iostream>
#include <unordered_map>

Parser::Parser(Lexer &lexer) : lexer(lexer), currentToken(lexer.getNextToken()) {}

void Parser::advance() {
    currentToken = lexer.getNextToken();
}

Token Parser::peekNextToken() {
    size_t savedPosition = lexer.position;
    int savedLine = lexer.line;
    Token nextToken = lexer.getNextToken();
    lexer.position = savedPosition;
    lexer.line = savedLine;
    return nextToken;
}

std::unordered_map<std::string, int> precedenceMap = {
    {"+", 1},
    {"-", 1},
    {"*", 2},
    {"/", 2},
    {"%", 2},
    {"^", 3} // For exponentiation
};

std::unique_ptr<ASTNode> Parser::parse() {
    return parseStatement();
}

std::unique_ptr<Statement> Parser::parseVariableReassignment() {
    int line = currentToken.line;

    // Expect an identifier (variable name)
    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected variable name at line " + std::to_string(line));
    }

    std::string variableName = currentToken.value;
    advance(); // Consume the variable name

    // Expect an assignment operator (e.g., =, +=, -=)
    if (currentToken.type != TokenType::Operator) {
        throw std::runtime_error("Expected assignment operator after variable at line " + std::to_string(line));
    }

    std::string op = currentToken.value;
    advance(); // Consume the operator

    // Parse the right-hand side expression
    auto valueExpr = parseExpression();

    // Ensure the statement ends with a semicolon
    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after reassignment at line " + std::to_string(line));
    }
    advance(); // Consume the semicolon

    // Return a new VariableReassignmentStatement
    return std::make_unique<VariableReassignmentStatement>(variableName, op, std::move(valueExpr), line);
}

std::unique_ptr<ASTNode> Parser::parseStatement() {

    int line = currentToken.line;

    if (currentToken.type == TokenType::EndOfFile) {
        return nullptr;
    }

    if (currentToken.type == TokenType::Identifier) {
        std::string varName = currentToken.value;
        advance(); // Consume identifier

        if (currentToken.type == TokenType::Operator) {
            std::string op = currentToken.value;
            advance(); // Consume operator

            // Parse the right-hand side as an expression
            auto rightExpr = parseExpression();

            // Ensure the statement ends with a semicolon
            if (currentToken.value != ";") {
                throw std::runtime_error("Expected ';' after variable reassignment at line " + std::to_string(line));
            }
            advance(); // Consume the semicolon

            // Return a BinaryExpression wrapped in an ExpressionStatement
            return std::make_unique<ExpressionStatement>(
                std::make_unique<BinaryExpression>(
                    op,
                    std::make_unique<VariableExpression>(varName, line),
                    std::move(rightExpr),
                    line
                ),
                line
            );
        } else {
            throw std::runtime_error("Unexpected token after variable name at line " + std::to_string(line));
        }
    }

    if (currentToken.type == TokenType::Keyword) {
        if (currentToken.value == "write") {
            return parseWriteStatement();
        } else if (currentToken.value == "read") {
            return parseReadStatement();
        } else if (currentToken.value == "let") {
            return parseVariableDeclaration();
        } else if (currentToken.value == "const") {
            return parseConstantDeclaration();
        } else if (currentToken.value == "if") {
            return parseIfStatement();
        } else if (currentToken.value == "for") {
            return parseForStatement();
        } else if (currentToken.value == "while") {
            return parseWhileStatement();
        } else if (currentToken.value == "include") {
            return parseIncludeStatement();
        } else if (currentToken.value == "class") {
            return parseClassDeclaration();
        } else if (currentToken.value == "function") {
            return parseFunctionDeclaration();
        } else if (currentToken.value == "return") {
            return parseReturnStatement();
        }
    }

    // Handle stand-alone expressions or function calls
    if (currentToken.type == TokenType::Identifier) {
        auto expr = parseExpression();

        // Ensure the statement ends with a semicolon
        if (currentToken.value != ";") {
            throw std::runtime_error("Expected ';' after expression at line " + std::to_string(line));
        }
        advance(); // Consume the semicolon
        return std::make_unique<ExpressionStatement>(std::move(expr), line);
    }

    throw std::runtime_error("Unexpected token: " + currentToken.value + " at line " + std::to_string(line));
}

std::unique_ptr<Statement> Parser::parseWriteStatement() {
    int line = currentToken.line;
    advance(); // Consume 'write'

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after 'write' at line " + std::to_string(line));
    }
    advance(); // Consume '('

    auto messageExpr = parseExpression();

    if (currentToken.value != ")") {
        throw std::runtime_error("Expected ')' after expression at line " + std::to_string(line));
    }
    advance(); // Consume ')'

    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after 'write' statement at line " + std::to_string(line));
    }
    advance(); // Consume ';'

    return std::make_unique<WriteStatement>(std::move(messageExpr), line);
}

std::unique_ptr<Statement> Parser::parseReadStatement() {
    int line = currentToken.line;
    advance(); // Consume 'read'

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after 'read' at line " + std::to_string(line));
    }
    advance(); // Consume '('

    std::unique_ptr<Expression> prompt = nullptr;
    if (currentToken.type == TokenType::StringLiteral) {
        prompt = parseExpression(); // Parse optional prompt
    }

    if (currentToken.value != ")") {
        throw std::runtime_error("Expected ')' after 'read' at line " + std::to_string(line));
    }
    advance(); // Consume ')'

    // Check for variable assignment
    if (currentToken.type == TokenType::Identifier) {
        std::string variableName = currentToken.value;
        advance(); // Consume variable name

        if (currentToken.value != ";") {
            throw std::runtime_error("Expected ';' after variable assignment in 'read' statement.");
        }
        advance(); // Consume ';'

        return std::make_unique<ReadStatement>(std::make_unique<VariableExpression>(variableName, line), std::move(prompt), line);
    }

    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after 'read' statement.");
    }
    advance(); // Consume ';'

    return std::make_unique<ReadStatement>(nullptr, std::move(prompt), line);
}

std::unique_ptr<Expression> Parser::parseReadExpression() {
    int line = currentToken.line;
    advance(); // Consume 'read'

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after 'read' at line " + std::to_string(line));
    }
    advance(); // Consume '('

    std::unique_ptr<Expression> prompt = nullptr;
    if (currentToken.type == TokenType::StringLiteral) {
        prompt = parseExpression(); // Parse optional prompt
    }

    if (currentToken.value != ")") {
        throw std::runtime_error("Expected ')' after 'read' at line " + std::to_string(line));
    }
    advance(); // Consume ')'

    return std::make_unique<ReadExpression>(std::move(prompt), line);
}

std::unique_ptr<Expression> Parser::parseExpressionStatement() {
    int line = currentToken.line;
    auto expression = parseExpression();

    // Handle reassignment (e.g., x = 5 or x += 2)
    if (currentToken.type == TokenType::Operator && 
       (currentToken.value == "=" || currentToken.value == "+=" || currentToken.value == "-=" || 
        currentToken.value == "*=" || currentToken.value == "/=" || currentToken.value == "%=")) {
        std::string op = currentToken.value;
        advance(); // Consume operator
        auto right = parseExpression(); // Parse the right-hand side
        return std::make_unique<BinaryExpression>(op, std::move(expression), std::move(right), line);
    }

    // Ensure we handle semicolons properly
    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after expression at line " + std::to_string(line));
    }
    advance(); // Consume ';'
    return expression;
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    int line = currentToken.line;
    advance(); // Consume 'if'

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after 'if' at line " + std::to_string(line));
    }
    advance(); // Consume '('

    auto condition = parseExpression();

    if (currentToken.value != ")") {
        throw std::runtime_error("Expected ')' after condition at line " + std::to_string(line));
    }
    advance(); // Consume ')'

    auto thenBranch = parseBlockStatement();

    std::unique_ptr<Statement> elseBranch = nullptr;
    if (currentToken.value == "else") {
        advance(); // Consume 'else'
        elseBranch = parseBlockStatement();
    }

    return std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line);
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    int line = currentToken.line;
    advance(); // Consume 'while'

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after 'while' at line " + std::to_string(line));
    }
    advance(); // Consume '('

    auto condition = parseExpression();

    if (currentToken.value != ")") {
        throw std::runtime_error("Expected ')' after condition at line " + std::to_string(line));
    }
    advance(); // Consume ')'

    auto body = parseBlockStatement();
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body), line);
}

std::unique_ptr<Statement> Parser::parseForStatement() {
    int line = currentToken.line;
    advance(); // Consume 'for'

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after 'for' at line " + std::to_string(line));
    }
    advance(); // Consume '('

    std::unique_ptr<Statement> initializer;
    if (currentToken.type == TokenType::Keyword && currentToken.value == "let") {
        initializer = parseVariableDeclaration(); // Parse variable declaration
    } else {
        auto expression = parseExpression(); // Parse a general expression
        initializer = std::make_unique<ExpressionStatement>(std::move(expression), line); // Wrap in ExpressionStatement
    }

    auto condition = parseExpression(); // Parse loop condition

    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after condition in 'for' statement at line " + std::to_string(line));
    }
    advance(); // Consume ';'

    auto incrementExpression = parseExpression(); // Parse loop increment
    auto increment = std::make_unique<ExpressionStatement>(std::move(incrementExpression), line); // Wrap in ExpressionStatement

    if (currentToken.value != ")") {
        throw std::runtime_error("Expected ')' after increment in 'for' statement at line " + std::to_string(line));
    }
    advance(); // Consume ')'

    auto body = parseBlockStatement(); // Parse loop body
    return std::make_unique<ForStatement>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), line);
}

std::unique_ptr<Statement> Parser::parseIncludeStatement() {
    int line = currentToken.line;
    advance(); // Consume 'include'

    std::unique_ptr<Expression> target = nullptr;

    // Check if the next token is an identifier (e.g., className_or_functionName_or)
    if (currentToken.type == TokenType::Identifier) {
        std::string targetName = currentToken.value;
        advance(); // Consume identifier

        // Check for optional dot (method or member access)
        while (currentToken.value == ".") {
            advance(); // Consume '.'
            if (currentToken.type != TokenType::Identifier) {
                throw std::runtime_error("Expected identifier after '.' in include statement at line " + std::to_string(line));
            }
            targetName += "." + currentToken.value;
            advance(); // Consume identifier
        }

        // Create a VariableExpression for the target
        target = std::make_unique<VariableExpression>(targetName, line);
    }

    // Look for the 'from' keyword
    if (currentToken.value != "from") {
        throw std::runtime_error("Expected 'from' in include statement at line " + std::to_string(line));
    }
    advance(); // Consume 'from'

    // Expect a string literal (filename)
    if (currentToken.type != TokenType::StringLiteral) {
        throw std::runtime_error("Expected string literal after 'from' in include statement at line " + std::to_string(line));
    }
    std::string fileName = currentToken.value;
    advance(); // Consume string literal

    // Consume optional semicolon
    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after include statement at line " + std::to_string(line));
    }
    advance(); // Consume ';'

    // Return an IncludeStatement with an optional target
    return std::make_unique<IncludeStatement>(fileName, line, std::move(target));
}

int Parser::getPrecedence(const std::string &op) {
    if (op == "||") return 1; // Logical OR
    if (op == "&&") return 2; // Logical AND
    if (op == "==" || op == "!=") return 3; // Equality
    if (op == "<" || op == "<=" || op == ">" || op == ">=") return 4; // Relational
    if (op == "+" || op == "-") return 5; // Addition/Subtraction
    if (op == "*" || op == "/") return 6; // Multiplication/Division
    if (op == "++" || op == "--") return 7; // Increment/Decrement
    return 0; // Default: lowest precedence
}

std::unique_ptr<Expression> Parser::parseExpression(int minPrecedence) {
    if (currentToken.type == TokenType::Keyword && currentToken.value == "let") {
        throw std::runtime_error("Unexpected 'let' in expression. Did you mean to declare a variable?");
    }

    // Handle specific keywords like 'read'
    if (currentToken.type == TokenType::Keyword && currentToken.value == "read") {
        return parseReadExpression();
    }

    // Parse the primary part of the expression
    auto left = parsePrimary();

    // Parse binary operators
    while (currentToken.type == TokenType::Operator && getPrecedence(currentToken.value) >= minPrecedence) {
        std::string op = currentToken.value;
        int precedence = getPrecedence(op);
        advance(); // Consume operator

        auto right = parseExpression(precedence + 1); // Parse the right-hand side
        left = std::make_unique<BinaryExpression>(op, std::move(left), std::move(right), currentToken.line);
    }

    // Handle function calls (like `factorial(n - 1)`)
    if (currentToken.value == "(") {
        advance(); // Consume '('
        std::vector<std::unique_ptr<Expression>> arguments;

        while (currentToken.type != TokenType::Symbol || currentToken.value != ")") {
            arguments.push_back(parseExpression());
            if (currentToken.value == ",") {
                advance(); // Consume ','
            } else if (currentToken.value == ")") {
                break; // Exit loop if ')' is encountered
            } else {
                throw std::runtime_error("Expected ',' or ')' in function call at line " +
                                         std::to_string(currentToken.line));
            }
        }
        advance(); // Consume ')'

        // Wrap the function call as an expression
        auto functionName = dynamic_cast<VariableExpression *>(left.get());
        if (!functionName) {
            throw std::runtime_error("Invalid function name in call at line " + std::to_string(currentToken.line));
        }

        // Create the function call expression
        left = std::make_unique<FunctionCallExpression>(functionName->name, std::move(arguments), currentToken.line);
    }

    // Ensure we do NOT consume semicolons here; leave them for the higher-level parser

    return left;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    int line = currentToken.line;

    if (currentToken.type == TokenType::Identifier) {
        std::string name = currentToken.value;
        advance(); // Consume identifier

        // Check for postfix operators (e.g., i++, i--)
        if (currentToken.value == "++" || currentToken.value == "--") {
            std::string op = currentToken.value;
            advance(); // Consume postfix operator
            return std::make_unique<UnaryExpression>(op, std::make_unique<VariableExpression>(name, line), line);
        }

        // Otherwise, treat as a variable
        return std::make_unique<VariableExpression>(name, line);
    }

    if (currentToken.type == TokenType::Number) {
        double value = std::stod(currentToken.value);
        advance(); // Consume number
        return std::make_unique<NumberExpression>(value, line);
    }

    if (currentToken.type == TokenType::StringLiteral) {
        std::string value = currentToken.value;
        advance(); // Consume string literal
        return std::make_unique<StringExpression>(value, line);
    }

    if (currentToken.value == "(") {
        advance(); // Consume '('
        auto expr = parseExpression();
        if (currentToken.value != ")") {
            throw std::runtime_error("Expected ')' after expression at line " + std::to_string(line));
        }
        advance(); // Consume ')'
        return expr;
    }

    throw std::runtime_error("Unexpected token in expression at line " + std::to_string(line));
}

std::unique_ptr<ClassDeclaration> Parser::parseClassDeclaration() {
    int line = currentToken.line;
    advance(); // Consume 'class'

    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected class name after 'class' at line " + std::to_string(line));
    }

    std::string className = currentToken.value;
    advance(); // Consume class name

    if (currentToken.value != "{") {
        throw std::runtime_error("Expected '{' after class name at line " + std::to_string(line));
    }
    advance(); // Consume '{'

    std::vector<std::unique_ptr<ClassMember>> members;
    while (currentToken.value != "}") {
        std::string modifier = "private"; // Default modifier
        if (currentToken.value == "public" || currentToken.value == "private" || currentToken.value == "protected") {
            modifier = currentToken.value;
            advance(); // Consume modifier
        }

        if (peekNextToken().value == "(") {
            members.push_back(parseMethodDeclaration(std::move(modifier)));
        } else {
            members.push_back(parseFieldDeclaration(std::move(modifier)));
        }
    }

    advance(); // Consume '}'
    return std::make_unique<ClassDeclaration>(className, std::move(members), line);
}

std::unique_ptr<FieldDeclaration> Parser::parseFieldDeclaration(const std::string& modifier) {
    int line = currentToken.line;
    std::string type = currentToken.value;
    advance(); // Consume type

    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected field name at line " + std::to_string(line));
    }

    std::string fieldName = currentToken.value;
    advance(); // Consume field name

    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after field declaration at line " + std::to_string(line));
    }
    advance(); // Consume ';'

    return std::make_unique<FieldDeclaration>(modifier, type, fieldName, line);
}

std::unique_ptr<MethodDeclaration> Parser::parseMethodDeclaration(const std::string& modifier) {
    int line = currentToken.line;
    std::string methodName = currentToken.value;
    advance(); // Consume method name

    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after method name at line " + std::to_string(line));
    }
    advance(); // Consume '('

    std::vector<std::string> parameters;
    while (currentToken.value != ")") {
        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected parameter name at line " + std::to_string(line));
        }
        parameters.push_back(currentToken.value);
        advance();

        if (currentToken.value == ",") {
            advance(); // Consume ','
        } else if (currentToken.value != ")") {
            throw std::runtime_error("Expected ',' or ')' in parameter list at line " + std::to_string(line));
        }
    }
    advance(); // Consume ')'

    auto body = parseBlockStatement();
    return std::make_unique<MethodDeclaration>(modifier, methodName, std::move(parameters), std::move(body), line);
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration() {
    int line = currentToken.line;
    advance(); // Consume 'let'

    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected variable name at line " + std::to_string(line));
    }

    std::string name = currentToken.value;
    advance(); // Consume variable name

    std::unique_ptr<Expression> initializer = nullptr;
    if (currentToken.value == "=") {
        advance(); // Consume '='
        initializer = parseExpression();
    }

    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after variable declaration at line " + std::to_string(line));
    }
    advance(); // Consume ';'

    return std::make_unique<VariableDeclaration>("auto", name, std::move(initializer), line);
}

std::unique_ptr<Statement> Parser::parseConstantDeclaration() {
    int line = currentToken.line;
    advance(); // Consume 'const'

    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected constant name at line " + std::to_string(line));
    }

    std::string name = currentToken.value;
    advance(); // Consume constant name

    if (currentToken.value != "=") {
        throw std::runtime_error("Expected '=' after constant name at line " + std::to_string(line));
    }
    advance(); // Consume '='

    auto initializer = parseExpression();

    if (currentToken.value != ";") {
        throw std::runtime_error("Expected ';' after constant declaration at line " + std::to_string(line));
    }
    advance(); // Consume ';'

    return std::make_unique<VariableDeclaration>("const", name, std::move(initializer), line);
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    int line = currentToken.line;
    advance(); // Consume 'return'

    std::unique_ptr<Expression> returnValue = nullptr;

    if (currentToken.value != ";" && currentToken.value != "}" && currentToken.type != TokenType::EndOfFile) {
        returnValue = parseExpression(); // Parse the return expression
    }

    // Look for semicolon or end of block
    if (currentToken.value == ";") {
        advance(); // Consume semicolon
    } else if (currentToken.value == "}" || currentToken.type == TokenType::EndOfFile) {
        // Implicit end of return statement at block boundary or EOF
    } else {
        throw std::runtime_error("Expected ';' after return statement at line " + std::to_string(line));
    }

    return std::make_unique<ReturnStatement>(std::move(returnValue), line);
}

std::unique_ptr<Statement> Parser::parseBlockStatement() {
    if (currentToken.value != "{") {
        // Single statement without braces
        auto singleStatement = std::unique_ptr<Statement>(dynamic_cast<Statement*>(parseStatement().release()));
        if (!singleStatement) {
            throw std::runtime_error("Invalid statement inside block at line " + std::to_string(currentToken.line));
        }
        return singleStatement;
    }
    advance(); // Consume '{'

    std::vector<std::unique_ptr<Statement>> statements;
    while (currentToken.value != "}") {
        // Dynamically cast each parsed statement and ensure validity
        auto stmt = std::unique_ptr<Statement>(dynamic_cast<Statement*>(parseStatement().release()));
        if (!stmt) {
            throw std::runtime_error("Invalid statement in block at line " + std::to_string(currentToken.line));
        }
        statements.push_back(std::move(stmt));
    }
    advance(); // Consume '}'

    return std::make_unique<BlockStatement>(std::move(statements), currentToken.line);
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    int line = currentToken.line;

    // Expect and consume 'function' keyword
    if (currentToken.value != "function") {
        throw std::runtime_error("Expected 'function' keyword at line " + std::to_string(line));
    }
    advance(); // Consume 'function'

    // Expect and consume function name
    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected function name after 'function' at line " + std::to_string(line));
    }
    std::string functionName = currentToken.value;
    if (functionName.empty()) {
        throw std::runtime_error("Function name is missing.");
    }
    advance(); // Consume function name

    // Expect and consume '('
    if (currentToken.value != "(") {
        throw std::runtime_error("Expected '(' after function name at line " + std::to_string(line));
    }
    advance(); // Consume '('

    // Parse parameters
    std::vector<std::string> parameters;
    while (currentToken.value != ")") {
        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected parameter name in function declaration at line " + std::to_string(line));
        }
        parameters.push_back(currentToken.value);
        advance(); // Consume parameter name

        // Consume ',' if more parameters follow
        if (currentToken.value == ",") {
            advance(); // Consume ','
        } else if (currentToken.value != ")") {
            throw std::runtime_error("Expected ',' or ')' in parameter list at line " + std::to_string(line));
        }
    }
    advance(); // Consume ')'

    // Parse function body
    auto body = parseBlockStatement();

    // Return a new FunctionDeclaration node
    return std::make_unique<FunctionDeclaration>(functionName, std::move(parameters), std::move(body), line);
}

std::unique_ptr<FunctionCallExpression> Parser::parseFunctionCallExpression(const std::string &functionName) {
    int line = currentToken.line;
    advance(); // Consume '(' after function name

    std::vector<std::unique_ptr<Expression>> arguments;
    while (currentToken.value != ")") {
        arguments.push_back(parseExpression());

        if (currentToken.value == ",") {
            advance(); // Consume ','
        } else if (currentToken.value != ")") {
            throw std::runtime_error("Expected ',' or ')' in argument list at line " + std::to_string(line));
        }
    }
    advance(); // Consume ')'

    return std::make_unique<FunctionCallExpression>(functionName, std::move(arguments), line);
}
