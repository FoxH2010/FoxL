#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>

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

struct Token {
    TokenType type;
    std::string value;

    Token(TokenType type, std::string value) : type(type), value(std::move(value)) {}
};

class Lexer {
public:
    explicit Lexer(const std::string &source) : source(source), position(0) {}

    Token getNextToken() {
        while (position < source.size()) {
            char currentChar = source[position];

            if (isspace(currentChar)) {
                ++position;
                continue;
            }

            if (currentChar == '/' && position + 1 < source.size() && source[position + 1] == '/') {
                skipSingleLineComment();
                continue;
            }

            if (isalpha(currentChar)) {
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

            if (currentChar == '\'' || currentChar == '"') {
                return lexStringLiteral(currentChar);
            }

            return Token(TokenType::Unknown, std::string(1, currentChar));
        }

        return Token(TokenType::EndOfFile, "");
    }

private:
    std::string source;
    size_t position;

    void skipSingleLineComment() {
        while (position < source.size() && source[position] != '\n') {
            ++position;
        }
        if (position < source.size() && source[position] == '\n') {
            ++position;
        }
    }

    Token lexIdentifierOrKeyword() {
        size_t start = position;
        while (position < source.size() && (isalnum(source[position]) || source[position] == '_')) {
            ++position;
        }
        std::string value = source.substr(start, position - start);

        if (isKeyword(value)) {
            return Token(TokenType::Keyword, value);
        }

        return Token(TokenType::Identifier, value);
    }

    Token lexNumber() {
        size_t start = position;
        bool hasDot = false;
        while (position < source.size() && (isdigit(source[position]) || (source[position] == '.' && !hasDot))) {
            if (source[position] == '.') hasDot = true;
            ++position;
        }
        return Token(TokenType::Number, source.substr(start, position - start));
    }

    Token lexOperator() {
        size_t start = position;
        char currentChar = source[position];

        if ((currentChar == '!' || currentChar == '=' || currentChar == '<' || currentChar == '>') &&
            position + 1 < source.size() && source[position + 1] == '=') {
            position += 2;
            return Token(TokenType::Operator, source.substr(start, 2));
        }

        if ((currentChar == '+' || currentChar == '-' || currentChar == '*' || currentChar == '/') &&
            position + 1 < source.size() && source[position + 1] == currentChar) {
            position += 2;
            return Token(TokenType::Operator, source.substr(start, 2));
        }

        return Token(TokenType::Operator, std::string(1, source[position++]));
    }

    Token lexSymbol() {
        return Token(TokenType::Symbol, std::string(1, source[position++]));
    }

    Token lexStringLiteral(char quoteType) {
        ++position; // consume the opening quote
        size_t start = position;
        std::string value;
        while (position < source.size() && source[position] != quoteType) {
            if (source[position] == '\\' && position + 1 < source.size()) {
                ++position;
                switch (source[position]) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '\\': value += '\\'; break;
                    case '\'': value += '\''; break;
                    case '"': value += '\"'; break;
                    default: value += source[position]; break;
                }
            } else {
                value += source[position];
            }
            ++position;
        }

        if (position >= source.size()) {
            throw std::runtime_error("Unterminated string literal");
        }

        ++position; // consume the closing quote
        return Token(TokenType::StringLiteral, value);
    }

    bool isKeyword(const std::string &str) const {
        static const std::vector<std::string> keywords = {
            "if", "else", "while", "return", "int", "float", "char", "write", "read", "func", "for", "str", "bool", "double", "include", "let"
        };

        for (const auto &keyword : keywords) {
            if (str == keyword) {
                return true;
            }
        }
        return false;
    }

    bool isOperator(char ch) const {
        static const std::string operators = "+-*/=!><";
        return operators.find(ch) != std::string::npos;
    }

    bool isSymbol(char ch) const {
        static const std::string symbols = "();{},[]";
        return symbols.find(ch) != std::string::npos;
    }
};
