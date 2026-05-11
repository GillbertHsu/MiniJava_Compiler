#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "ast.h"
#include "symtab.h"
#include <vector>
#include <string>
#include <memory>

class TypeChecker {
public:
    explicit TypeChecker(MethodTable& methods);

    void buildSymbolTables(ASTNode* root);
    bool checkTypes(ASTNode* root);
    bool hasErrors() const { return !errorLines_.empty(); }
    void reportErrors() const;

private:
    MethodTable& methods_;
    std::vector<std::unique_ptr<SymbolTable>> scopes_;
    std::vector<int> errorLines_;

    SymbolTable* createScope(ASTNode* node, SymbolTable* parent);
    void fillTable(ASTNode* root, SymbolTable* currentScope);
    void checkNode(ASTNode* root);
    void addError(int line);

    // Type resolution
    DataType findType(ASTNode* typeNode);
    DataType findExpType(ASTNode* expr);
    ASTNode* unwrapExp(ASTNode* node);
    SymbolInfo* findLeftValueInfo(ASTNode* node);
    int countParams(ASTNode* formalList);

    // Check helpers
    bool checkExpPlus(ASTNode* node);
    bool checkOperationAndComp(ASTNode* node);
    bool checkExpComp(ASTNode* node);
    bool checkExpLogic(ASTNode* node);
    bool checkExpLogicNot(ASTNode* node);
    bool checkExpPosNeg(ASTNode* node);
    bool checkIfWhile(ASTNode* node);
    bool checkVarDecl(ASTNode* node);
    bool checkAssign(ASTNode* node);
    bool checkExpLength(ASTNode* node);
    bool checkLeftValueArr(ASTNode* node);
    bool checkMethodCall(ASTNode* node);
};

#endif
