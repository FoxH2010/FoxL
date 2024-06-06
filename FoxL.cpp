#include "lexer.cpp"
#include "parser.cpp"
#include "interpreter.cpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_name.foxl>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
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
