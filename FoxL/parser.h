#ifndef PARSER_H
#define PARSER_H

#include "ast_nodes.cpp"
#include "lexer.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

// The Parser class converts tokens into an Abstract Syntax Tree (AST)
class Parser {
public:
    explicit Parser(Lexer &lexer);

    // Parse the entire input and return the root node of the AST
    std::unique_ptr<ASTNode> parse();

private:
    Lexer &lexer;
    Token currentToken;

    // Helper function to advance to the next token
    void advance();

    // Peek the next token without consuming it
    Token peekNextToken();

    // Parse a statement or expression
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<Expression> parseExpressionStatement();

    // Specialized parsing functions for various types of statements
    std::unique_ptr<Statement> parseReadStatement();
    std::unique_ptr<Statement> parseVariableDeclaration();
    std::unique_ptr<Statement> parseVariableReassignment();
    std::unique_ptr<Statement> parseConstantDeclaration();
    std::unique_ptr<Statement> parseWriteStatement();
    std::unique_ptr<Statement> parseIncludeStatement();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseForStatement();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseWhileStatement();

    // Parse a block of statements (e.g., in a function or control flow)
    std::unique_ptr<Statement> parseBlockStatement();

    // Specialized parsing for class members and declarations
    std::unique_ptr<ClassDeclaration> parseClassDeclaration();
    std::unique_ptr<FieldDeclaration> parseFieldDeclaration(const std::string& modifier);
    std::unique_ptr<MethodDeclaration> parseMethodDeclaration(const std::string& modifier);
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration();
    std::unique_ptr<FunctionCallExpression> parseFunctionCallExpression(const std::string& functionName);

    // Parse expressions and primary constructs
    std::unique_ptr<Expression> parseExpression(int minPrecedence = 0);
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parseReadExpression();

    // Operator precedence map
    std::unordered_map<std::string, int> precedenceMap;
    int getPrecedence(const std::string& op);
};

#endif
