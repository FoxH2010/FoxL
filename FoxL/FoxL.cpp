#include "lexer.cpp"
#include "parser.cpp"
#include "interpreter.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

const std::string VERSION = "0.0.3";

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

    Lexer lexer(input);
    Parser parser(lexer);

    try {
        std::vector<std::unique_ptr<Statement>> statements;
        while (auto ast = parser.parse()) {
            if (auto stmt = dynamic_cast<Statement*>(ast.get())) {
                statements.push_back(std::unique_ptr<Statement>(stmt));
                ast.release();
            }
        }

        Interpreter interpreter;
        interpreter.interpret(statements);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
