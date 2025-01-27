#include "ast_nodes.h"
#include <memory>

// Helper function to clone expressions
std::unique_ptr<Expression> cloneExpression(const Expression *expr) {
    if (const auto *numExpr = dynamic_cast<const NumberExpression *>(expr)) {
        return std::make_unique<NumberExpression>(numExpr->value, numExpr->line);
    } else if (const auto *strExpr = dynamic_cast<const StringExpression *>(expr)) {
        return std::make_unique<StringExpression>(strExpr->value, strExpr->line);
    } else if (const auto *boolExpr = dynamic_cast<const BoolExpression *>(expr)) {
        return std::make_unique<BoolExpression>(boolExpr->value, boolExpr->line);
    } else if (const auto *varExpr = dynamic_cast<const VariableExpression *>(expr)) {
        return std::make_unique<VariableExpression>(varExpr->name, varExpr->line);
    } else if (const auto *binExpr = dynamic_cast<const BinaryExpression *>(expr)) {
        return std::make_unique<BinaryExpression>(
            binExpr->op,
            cloneExpression(binExpr->left.get()),
            cloneExpression(binExpr->right.get()),
            binExpr->line
        );
    } else if (const auto *unaryExpr = dynamic_cast<const UnaryExpression *>(expr)) {
        return std::make_unique<UnaryExpression>(
            unaryExpr->op,
            cloneExpression(unaryExpr->operand.get()),
            unaryExpr->line
        );
    } else if (const auto *funcCallExpr = dynamic_cast<const FunctionCallExpression *>(expr)) {
        std::vector<std::unique_ptr<Expression>> clonedArgs;
        for (const auto &arg : funcCallExpr->arguments) {
            clonedArgs.push_back(cloneExpression(arg.get()));
        }
        return std::make_unique<FunctionCallExpression>(
            funcCallExpr->functionName, std::move(clonedArgs), funcCallExpr->line
        );
    } else if (const auto *arrayExpr = dynamic_cast<const ArrayExpression *>(expr)) {
        std::vector<std::unique_ptr<Expression>> clonedElements;
        for (const auto &element : arrayExpr->elements) {
            clonedElements.push_back(cloneExpression(element.get()));
        }
        return std::make_unique<ArrayExpression>(std::move(clonedElements), arrayExpr->line);
    }
    throw std::runtime_error("Unsupported expression type for cloning.");
}

// Helper function to clone statements
std::unique_ptr<Statement> cloneStatement(const Statement *stmt) {
    if (const auto *writeStmt = dynamic_cast<const WriteStatement *>(stmt)) {
        return std::make_unique<WriteStatement>(cloneExpression(writeStmt->messageExpr.get()), writeStmt->line);
    } else if (const auto *blockStmt = dynamic_cast<const BlockStatement *>(stmt)) {
        std::vector<std::unique_ptr<Statement>> clonedStatements;
        for (const auto &innerStmt : blockStmt->statements) {
            clonedStatements.push_back(cloneStatement(innerStmt.get()));
        }
        return std::make_unique<BlockStatement>(std::move(clonedStatements), blockStmt->line);
    } else if (const auto *ifStmt = dynamic_cast<const IfStatement *>(stmt)) {
        return std::make_unique<IfStatement>(
            cloneExpression(ifStmt->condition.get()),
            cloneStatement(ifStmt->thenBranch.get()),
            ifStmt->elseBranch ? cloneStatement(ifStmt->elseBranch.get()) : nullptr,
            ifStmt->line
        );
    } else if (const auto *whileStmt = dynamic_cast<const WhileStatement *>(stmt)) {
        return std::make_unique<WhileStatement>(
            cloneExpression(whileStmt->condition.get()),
            cloneStatement(whileStmt->body.get()),
            whileStmt->line
        );
    } else if (const auto *forStmt = dynamic_cast<const ForStatement *>(stmt)) {
        return std::make_unique<ForStatement>(
            cloneStatement(forStmt->initializer.get()),
            cloneExpression(forStmt->condition.get()),
            cloneStatement(forStmt->increment.get()),
            cloneStatement(forStmt->body.get()),
            forStmt->line
        );
    } else if (const auto *forEachStmt = dynamic_cast<const ForEachStatement *>(stmt)) {
        return std::make_unique<ForEachStatement>(
            forEachStmt->variable,
            cloneExpression(forEachStmt->iterable.get()),
            cloneStatement(forEachStmt->body.get()),
            forEachStmt->line
        );
    } else if (const auto *returnStmt = dynamic_cast<const ReturnStatement *>(stmt)) {
        return std::make_unique<ReturnStatement>(
            returnStmt->returnValue ? cloneExpression(returnStmt->returnValue.get()) : nullptr,
            returnStmt->line
        );
    } else if (const auto *varDecl = dynamic_cast<const VariableDeclaration *>(stmt)) {
        return std::make_unique<VariableDeclaration>(
            varDecl->type,
            varDecl->name,
            varDecl->initializer ? cloneExpression(varDecl->initializer.get()) : nullptr,
            varDecl->line
        );
    } else if (const auto *funcDecl = dynamic_cast<const FunctionDeclaration *>(stmt)) {
        std::vector<std::string> clonedParams = funcDecl->parameters;
        return std::make_unique<FunctionDeclaration>(
            funcDecl->name,
            std::move(clonedParams),
            cloneStatement(funcDecl->body.get()),
            funcDecl->line
        );
    }
    throw std::runtime_error("Unsupported statement type for cloning.");
}
