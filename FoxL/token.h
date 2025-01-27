#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// Enumeration for all possible token types
enum class TokenType {
    Keyword,
    Identifier,
    Number,
    Operator,
    Symbol,
    StringLiteral,
    EndOfFile,
    Unknown
};

// Structure representing a token
struct Token {
    TokenType type;       // The type of the token
    std::string value;    // The actual token string
    int line;             // Line number where the token appears

    // Constructor for initializing a token
    Token(TokenType type, std::string value, int line)
        : type(type), value(std::move(value)), line(line) {}
};

#endif