#include "lexer.cpp"
#include "token.h"
#include "parser.cpp"
#include "interpreter.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

const std::string VERSION = "0.0.4";

void displayUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <file_name.foxl>\n";
    std::cout << "Options:\n";
    std::cout << "  --help      Display this help message\n";
    std::cout << "  --version   Display the version information\n";
}

void displayVersion() {
    std::cout << "FoxL Interpreter version: " << VERSION << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        displayUsage(argv[0]);
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--help") {
        displayUsage(argv[0]);
        return 0;
    } else if (arg1 == "--version") {
        displayVersion();
        return 0;
    }

    std::ifstream file(arg1);
    if (!file) {
        std::cerr << "Error: Could not open file " << arg1 << std::endl;
        return 1;
    }

    std::string input((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Instantiate Lexer, Parser, Environment, and Interpreter
    Lexer lexer(input);
    Parser parser(lexer);
    auto environment = std::make_shared<Environment>();
    Interpreter interpreter(environment);

    try {
        // Parse and interpret the input
        while (auto ast = parser.parse()) {
            // Ensure the parsed AST node is valid
            if (!ast) {
                throw std::runtime_error("Parsed AST is null.");
            }

            interpreter.interpret(ast);  // Interpret the current AST node
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
