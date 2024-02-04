#include <iostream>
#include <string>
#include <vector>
#include <cctype>

struct Token {
    enum class Type {
        Identifier,    // identifiers, such as variable names
        Integer,       // integer literals
        Plus,          // arithmetic operators
        Minus,
        Star,
        Slash,
        LessThan,
        GreaterThan,
        Equal,
        NotEqual,
        Semicolon,     // punctuation symbols
        LeftParenthesis,
        RightParenthesis,
        If,            // keywords
        Else,
        While,
        LeftBrace,
        RightBrace,
        Assign,
        For,
        Function,
        Write,
        Input,
        String,        // string literals
        SingleQuotedString,
        Error,
    } type;
    std::string lexeme;  // the actual text of the token
    int value;             // optional value for integer literals
};

// checks if a character can start an identifier (alphabetical character or underscore)
bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// checks if a character can be part of an identifier (alphabetical character, underscore, or digit)
bool is_ident_part(char c) {
    return is_ident_start(c) || std::isdigit(static_cast<unsigned char>(c));
}

// checks if a character can start a number (digit)
bool is_number_start(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

// checks if a character can be part of a number (digit)
bool is_number_part(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

// checks if a character is an escaped character (backslash)
bool is_escaped_char(char c) {
    return c == '\\';
}

// lexer function that breaks down a string into tokens
std::vector<Token> lex(const std::string& input) {
    std::vector<Token> tokens;
    int pos = 0;
    while (pos < input.size()) {
        char c = input[pos];
        if (std::isspace(static_cast<unsigned char>(c))) { // skip whitespace
            ++pos;
            continue;
        }
        if (is_ident_start(c)) { // check for identifiers
            std::string ident;
            while (pos < input.size() && is_ident_part(input[pos])) {
                ident += input[pos++];
            }
            Token token;
            if (ident == "if") {
                token.type = Token::Type::If;
            } else if (ident == "else") {
                token.type = Token::Type::Else;
            } else if (ident == "while") {
                token.type = Token::Type::While;
            } else if (ident == "for") {
                token.type = Token::Type::For;
            } else if (ident == "function") {
                token.type = Token::Type::Function;
            } else if (ident == "write") {
                token.type = Token::Type::Write;
            } else if (ident == "input") {
                token.type = Token::Type::Input;
            } else {
                token.type = Token::Type::Identifier;
            }
            token.lexeme = ident;
            tokens.push_back(token);
        } else if (is_number_start(c)) { // check for numbers
            std::string number;
            while (pos < input.size() && is_number_part(input[pos])) {
                number += input[pos++];
            }
            Token token;
            token.type = Token::Type::Integer;
            token.lexeme = number;
            token.value = std::stoi(number);
            tokens.push_back(token);
        } else if (c == '+') { // punctuation symbols
            tokens.push_back({Token::Type::Plus});
            ++pos;
        } else if (c == '-') {
            tokens.push_back({Token::Type::Minus});
            ++pos;
        } else if (c == '*') {
            tokens.push_back({Token::Type::Star});
            ++pos;
        } else if (c == '/') {
            tokens.push_back({Token::Type::Slash});
            ++pos;
        } else if (c == '<') {
            tokens.push_back({Token::Type::LessThan});
            ++pos;
        } else if (c == '>') {
            tokens.push_back({Token::Type::GreaterThan});
            ++pos;
        } else if (c == '=') {
            if (pos + 1 < input.size() && input[pos + 1] == '=') {
                tokens.push_back({Token::Type::Equal});
                pos += 2;
            } else {
                tokens.push_back({Token::Type::Assign});
                ++pos;
            }
        } else if (c == '!') {
            if (pos + 1 < input.size() && input[pos + 1] == '=') {
                tokens.push_back({Token::Type::NotEqual});
                pos += 2;
            } else {
                tokens.push_back({Token::Type::Error});
                ++pos;
            }
        } else if (c == ';') {
            tokens.push_back({Token::Type::Semicolon});
            ++pos;
        } else if (c == '(') {
            tokens.push_back({Token::Type::LeftParenthesis});
            ++pos;
        } else if (c == ')') {
            tokens.push_back({Token::Type::RightParenthesis});
            ++pos;
        } else if (c == '{') {
            tokens.push_back({Token::Type::LeftBrace});
            ++pos;
        } else if (c == '}') {
            tokens.push_back({Token::Type::RightBrace});
            ++pos;
        } else if (c == '\'') { // string literals
            if (pos + 1 < input.size() && input[pos + 1] == '\'') {
                tokens.push_back({Token::Type::SingleQuotedString, "''"});
                pos += 2;
            } else {
                std::string singleQuotedString;
                bool escaped = false;
                while (pos < input.size()) {
                    if (escaped) {
                        singleQuotedString += input[pos++];
                        escaped = false;
                    } else if (input[pos] == '\\' && pos + 1 < input.size()) {
                        escaped = true;
                        ++pos;
                    } else if (input[pos] == '\\' && pos + 1 == input.size()) {
                        break;
                    } else {
                        singleQuotedString += input[pos++];
                    }
                }
                if (pos < input.size()) {
                    tokens.push_back({Token::Type::SingleQuotedString, singleQuotedString});
                } else {
                    tokens.push_back({Token::Type::Error});
                }
            }
        } else if (c == '\"') { // string literals
            if (pos + 1 < input.size() && input[pos + 1] == '\"') {
                tokens.push_back({Token::Type::String, "''"});
                pos += 2;
            } else {
                std::string string;
                bool escaped = false;
                while (pos < input.size()) {
                    if (escaped) {
                        string += input[pos++];
                        escaped = false;
                    } else if (input[pos] == '\\' && pos + 1 < input.size()) {
                        escaped = true;
                        ++pos;
                    } else if (input[pos] == '\"' && pos + 1 == input.size()) {
                        break;
                    } else {
                        string += input[pos++];
                    }
                }
                if (pos < input.size()) {
                    tokens.push_back({Token::Type::String, string});
                } else {
                    tokens.push_back({Token::Type::Error});
                }
            }
        } else { // any other character is an error
            tokens.push_back({Token::Type::Error});
            ++pos;
        }
    }
    return tokens;
}

int main() {
    std::string input = "function foo() { write(\"Hello, World!\"); }";
    std::vector<Token> tokens = lex(input);
    for (const Token& token : tokens) {
        switch (token.type) {
            case Token::Type::Identifier:
                std::cout << "Identifier: " << token.lexeme << '\n';
                break;
            case Token::Type::Integer:
                std::cout << "Integer: " << token.lexeme << " (" << token.value << ")\n";
                break;
            case Token::Type::Plus:
                std::cout << "Plus\n";
                break;
            case Token::Type::Minus:
                std::cout << "Minus\n";
                break;
            case Token::Type::Star:
                std::cout << "Star\n";
                break;
            case Token::Type::Slash:
                std::cout << "Slash\n";
                break;
            case Token::Type::LessThan:
                std::cout << "LessThan\n";
                break;
            case Token::Type::GreaterThan:
                std::cout << "GreaterThan\n";
                break;
            case Token::Type::Equal:
                std::cout << "Equal\n";
                break;
            case Token::Type::NotEqual:
                std::cout << "NotEqual\n";
                break;
            case Token::Type::Semicolon:
                std::cout << "Semicolon\n";
                break;
            case Token::Type::LeftParenthesis:
                std::cout << "LeftParenthesis\n";
                break;
            case Token::Type::RightParenthesis:
                std::cout << "RightParenthesis\n";
                break;
            case Token::Type::If:
                std::cout << "If\n";
                break;
            case Token::Type::Else:
                std::cout << "Else\n";
                break;
            case Token::Type::While:
                std::cout << "While\n";
                break;
            case Token::Type::LeftBrace:
                std::cout << "LeftBrace\n";
                break;
            case Token::Type::RightBrace:
                std::cout << "RightBrace\n";
                break;
            case Token::Type::Assign:
                std::cout << "Assign\n";
                break;
            case Token::Type::For:
                std::cout << "For\n";
                break;
            case Token::Type::Function:
                std::cout << "Function\n";
                break;
            case Token::Type::Write:
                std::cout << "Write\n";
                break;
            case Token::Type::Input:
                std::cout << "Input\n";
                break;
            case Token::Type::String:
                std::cout << "String: " << token.lexeme << '\n';
                break;
            case Token::Type::SingleQuotedString:
                std::cout << "SingleQuotedString: " << token.lexeme << '\n';
                break;
            case Token::Type::Error:
                std::cout << "Error\n";
                break;
        }
    }
    return 0;
}