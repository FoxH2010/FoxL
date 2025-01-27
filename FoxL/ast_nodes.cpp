#include "ast_nodes.h"
#include <iostream>

void WriteStatement::print() const {
    std::cout << "WriteStatement(";
    if (messageExpr) {
        messageExpr->print();
    }
    std::cout << ", line: " << line << ")" << std::endl;
}

void ReadExpression::print() const {
    std::cout << "ReadExpression(";
    if (prompt) {
        prompt->print();
    }
    std::cout << ", line: " << line << ")" << std::endl;
}

void VariableDeclaration::print() const {
    std::cout << "VariableDeclaration(" << type << " " << name << ", line: " << line << ")" << std::endl;
    if (initializer) {
        initializer->print();
    }
}

void NumberExpression::print() const {
    std::cout << "NumberExpression(" << value << ", line: " << line << ")" << std::endl;
}

void StringExpression::print() const {
    std::cout << "StringExpression(" << value << ", line: " << line << ")" << std::endl;
}

void BoolExpression::print() const {
    std::cout << "BoolExpression(" << std::boolalpha << value << ", line: " << line << ")" << std::endl;
}

void ArrayExpression::print() const {
    std::cout << "ArrayExpression, line: " << line << std::endl;
    for (const auto &elem : elements) {
        elem->print();
    }
}

void VariableExpression::print() const {
    std::cout << "VariableExpression(" << name << ", line: " << line << ")" << std::endl;
}

void BinaryExpression::print() const {
    std::cout << "BinaryExpression(" << op << ", line: " << line << ")" << std::endl;
    std::cout << "Left: ";
    if (left) left->print();
    std::cout << "Right: ";
    if (right) right->print();
    if (rightElse) {
        std::cout << "Right Else: ";
        rightElse->print();
    }
}

void UnaryExpression::print() const {
    std::cout << "UnaryExpression(" << op << ", line: " << line << ")" << std::endl;
    if (operand) operand->print();
}

void FunctionCallExpression::print() const {
    std::cout << "FunctionCallExpression(" << functionName << ", line: " << line << ")" << std::endl;
    for (const auto &arg : arguments) {
        arg->print();
    }
}

void IndexExpression::print() const {
    std::cout << "IndexExpression(line: " << line << ")" << std::endl;
    std::cout << "Array: ";
    if (array) array->print();
    std::cout << "Index: ";
    if (index) index->print();
}

void FunctionDeclaration::print() const {
    std::cout << "FunctionDeclaration(" << name << ", line: " << line << ")" << std::endl;
    std::cout << "Parameters: ";
    for (const auto &param : parameters) {
        std::cout << param << " ";
    }
    std::cout << std::endl;
    if (body) {
        body->print();
    }
}

void FieldDeclaration::print() const {
    std::cout << modifier << " FieldDeclaration(" << type << " " << name << ", line: " << line << ")" << std::endl;
}

void MethodDeclaration::print() const {
    std::cout << modifier << " MethodDeclaration(" << name << ", line: " << line << ")" << std::endl;
    std::cout << "Parameters: ";
    for (const auto &param : parameters) {
        std::cout << param << " ";
    }
    std::cout << std::endl;
    if (body) {
        body->print();
    }
}

void ClassDeclaration::print() const {
    std::cout << "ClassDeclaration(" << name << ", line: " << line << ")" << std::endl;
    for (const auto &member : members) {
        member->print();
    }
}

void IfStatement::print() const {
    std::cout << "IfStatement(line: " << line << ")" << std::endl;
    std::cout << "Condition: ";
    if (condition) condition->print();
    std::cout << "Then Branch: ";
    if (thenBranch) thenBranch->print();
    if (elseBranch) {
        std::cout << "Else Branch: ";
        elseBranch->print();
    }
}

void WhileStatement::print() const {
    std::cout << "WhileStatement(line: " << line << ")" << std::endl;
    std::cout << "Condition: ";
    if (condition) condition->print();
    std::cout << "Body: ";
    if (body) body->print();
}

ReadStatement::ReadStatement(std::unique_ptr<Expression> variable, 
                             std::unique_ptr<Expression> prompt, 
                             int line)
    : Statement(line), variable(std::move(variable)), prompt(std::move(prompt)) {}

void ReadStatement::print() const {
    std::cout << "ReadStatement(line: " << line << ")" << std::endl;

    if (prompt) {
        std::cout << "Prompt: ";
        prompt->print();
    }

    if (variable) {
        std::cout << "Variable: ";
        variable->print();
    }
}

void IncludeStatement::print() const {
    std::cout << "IncludeStatement(File: " << fileName << ", line: " << line << ")" << std::endl;
    if (target) {
        std::cout << "Target: ";
        target->print();
    }
}

void ReturnStatement::print() const {
    std::cout << "ReturnStatement(line: " << line << ")" << std::endl;
    if (returnValue) {
        std::cout << "Return Value: ";
        returnValue->print();
    }
}

void ForEachStatement::print() const {
    std::cout << "ForEachStatement(line: " << line << ")" << std::endl;
    std::cout << "Variable: " << variable << std::endl;
    std::cout << "Iterable: ";
    if (iterable) iterable->print();
    std::cout << "Body: ";
    if (body) body->print();
}

void BlockStatement::print() const {
    std::cout << "BlockStatement(line: " << line << ")" << std::endl;
    for (const auto &statement : statements) {
        statement->print();
    }
}

void ExpressionStatement::print() const {
    std::cout << "ExpressionStatement(line: " << line << ")" << std::endl;
    if (expression) {
        expression->print();
    }
}