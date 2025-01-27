#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_set>
#include <vector>
#include "token.h"
#include <algorithm>

// Lexer class
class Lexer {
public:
    explicit Lexer(const std::string &source);

    Token getNextToken();
    void registerIdentifier(const std::string &identifier);
    size_t position;
    int line;

private:
    std::string source;
    std::unordered_set<std::string> identifiers;

protected: // Change from private to protected
    // Utility functions
    void handleWhitespace();
    bool isCommentStart(char currentChar) const;
    void skipSingleLineComment();
    bool isIdentifierStart(char ch) const;
    bool isIdentifierPart(char ch) const;
    bool isStringStart(char ch) const;
    bool isOperator(char ch) const;
    bool isSymbol(char ch) const;
    bool isTwoCharOperator(char ch) const;
    char parseEscapeCharacter(char ch) const;
    Token lexIdentifierOrKeyword();
    Token lexNumber();
    Token lexOperator();
    Token lexSymbol();
    Token lexStringLiteral(char quoteType);
    bool isKeyword(const std::string &str) const;
};


#endif
