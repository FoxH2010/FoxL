#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <algorithm> // For std::find

Lexer::Lexer(const std::string &source)
    : source(source), position(0), line(1) {}

Token Lexer::getNextToken() {
    while (position < source.size()) {
        char currentChar = source[position];

        if (isspace(currentChar)) {
            handleWhitespace();
            continue;
        }

        if (isCommentStart(currentChar)) {
            skipSingleLineComment();
            continue;
        }

        if (isIdentifierStart(currentChar)) {
            return lexIdentifierOrKeyword();
        }

        if (isdigit(currentChar)) {
            return lexNumber();
        }

        if (isOperator(currentChar)) {
            return lexOperator();
        }

        if (isSymbol(currentChar)) {
            return lexSymbol();
        }

        if (isStringStart(currentChar)) {
            return lexStringLiteral(currentChar);
        }

        throw std::runtime_error("Unknown character at line " + std::to_string(line));
    }

    return Token(TokenType::EndOfFile, "", line);
}

void Lexer::registerIdentifier(const std::string &identifier) {
    identifiers.insert(identifier);
}

void Lexer::handleWhitespace() {
    while (position < source.size() && isspace(source[position])) {
        if (source[position] == '\n') {
            ++line;
        }
        ++position;
    }
}

bool Lexer::isCommentStart(char currentChar) const {
    return currentChar == '/' && position + 1 < source.size() && source[position + 1] == '/';
}

void Lexer::skipSingleLineComment() {
    while (position < source.size() && source[position] != '\n') {
        ++position;
    }
    if (position < source.size() && source[position] == '\n') {
        ++line;
        ++position;
    }
}

bool Lexer::isIdentifierStart(char ch) const {
    return isalpha(ch) || (ch & 0x80); // Support for extended ASCII
}

bool Lexer::isIdentifierPart(char ch) const {
    return isalnum(ch) || ch == '_' || (ch & 0x80);
}

bool Lexer::isStringStart(char ch) const {
    return ch == '"' || ch == '\'';
}

bool Lexer::isOperator(char ch) const {
    static const std::string operators = "+-*/%=&|<>!^";
    return operators.find(ch) != std::string::npos;
}

bool Lexer::isSymbol(char ch) const {
    static const std::string symbols = ";(){}[],:.@";
    return symbols.find(ch) != std::string::npos;
}

bool Lexer::isTwoCharOperator(char ch) const {
    static const std::vector<std::string> twoCharOperators = {
        "==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>"
    };
    return std::find(twoCharOperators.begin(), twoCharOperators.end(), source.substr(position, 2)) != twoCharOperators.end();
}

char Lexer::parseEscapeCharacter(char ch) const {
    switch (ch) {
        case 'n': return '\n';
        case 't': return '\t';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        default: return ch;
    }
}

Token Lexer::lexIdentifierOrKeyword() {
    size_t start = position;
    while (position < source.size() && isIdentifierPart(source[position])) {
        ++position;
    }
    std::string value = source.substr(start, position - start);

    if (isKeyword(value)) {
        return Token(TokenType::Keyword, value, line);
    }

    return Token(TokenType::Identifier, value, line);
}

Token Lexer::lexNumber() {
    size_t start = position;
    bool hasDot = false;

    while (position < source.size() && (isdigit(source[position]) || (source[position] == '.' && !hasDot))) {
        if (source[position] == '.') hasDot = true;
        ++position;
    }

    return Token(TokenType::Number, source.substr(start, position - start), line);
}

Token Lexer::lexOperator() {
    size_t start = position;

    if (isTwoCharOperator(source[position])) {
        position += 2;
        return Token(TokenType::Operator, source.substr(start, 2), line);
    }

    return Token(TokenType::Operator, std::string(1, source[position++]), line);
}

Token Lexer::lexSymbol() {
    return Token(TokenType::Symbol, std::string(1, source[position++]), line);
}

Token Lexer::lexStringLiteral(char quoteType) {
    ++position; // Consume opening quote
    std::string value;

    while (position < source.size() && source[position] != quoteType) {
        if (source[position] == '\\') {
            ++position;
            value += parseEscapeCharacter(source[position]);
        } else {
            value += source[position];
        }
        ++position;
    }

    if (position >= source.size()) {
        throw std::runtime_error("Unterminated string literal at line " + std::to_string(line));
    }
    ++position; // Consume closing quote

    return Token(TokenType::StringLiteral, value, line);
}

bool Lexer::isKeyword(const std::string &str) const {
    static const std::vector<std::string> keywords = {
        "if", "else", "while", "return", "write", "read", "for", "include", "let", "const",
        "function", "class", "public", "private", "protected", "in", "from"
    };
    return std::find(keywords.begin(), keywords.end(), str) != keywords.end();
}
