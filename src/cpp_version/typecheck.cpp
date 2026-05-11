#include "typecheck.h"
#include <cstdio>
#include <algorithm>
#include <cstring>

TypeChecker::TypeChecker(MethodTable& methods) : methods_(methods) {}

SymbolTable* TypeChecker::createScope(ASTNode* node, SymbolTable* parent) {
    scopes_.push_back(std::make_unique<SymbolTable>(parent));
    auto* scope = scopes_.back().get();
    node->scope = scope;
    return scope;
}

void TypeChecker::addError(int line) {
    errorLines_.push_back(line);
}

void TypeChecker::reportErrors() const {
    auto sorted = errorLines_;
    std::sort(sorted.begin(), sorted.end());
    for (int line : sorted)
        fprintf(stderr, "Type violation in line %d\n", line);
}

DataType TypeChecker::findType(ASTNode* typeNode) {
    if (!typeNode) return DataType::UNDEFINED;
    if (typeNode->node_type == NodeType::PRIME_TYPE)
        return typeNode->data_type;
    if (!typeNode->children.empty())
        return findType(typeNode->children[0].get());
    return DataType::UNDEFINED;
}

ASTNode* TypeChecker::unwrapExp(ASTNode* node) {
    if (!node) return nullptr;
    if (node->node_type == NodeType::EXP && !node->children.empty())
        return unwrapExp(node->children[0].get());
    return node;
}

int TypeChecker::countParams(ASTNode* formalList) {
    if (!formalList) return 0;
    int count = 1;
    // The second child of FORMAL_LIST is the next param (chained)
    if (formalList->children.size() > 1 && formalList->children[1])
        count += countParams(formalList->children[1].get());
    return count;
}

SymbolInfo* TypeChecker::findLeftValueInfo(ASTNode* node) {
    if (!node) return nullptr;
    if (node->node_type == NodeType::LEFTVALUE_ID ||
        node->node_type == NodeType::LEFTVALUE_ONE_ARR ||
        node->node_type == NodeType::LEFTVALUE_TWO_ARR) {
        if (node->scope) {
            auto* info = node->scope->lookup(node->str_val);
            if (info) return info;
        }
        return methods_.lookup(node->str_val);
    }
    if (node->node_type == NodeType::METHOD_CALL ||
        node->node_type == NodeType::CALL_MAIN) {
        return methods_.lookup(node->str_val);
    }
    if (node->node_type == NodeType::VAR_DECL ||
        node->node_type == NodeType::ID_EXP_LIST) {
        if (node->scope)
            return node->scope->lookupLocal(node->str_val);
    }
    if (node->node_type == NodeType::EXP_METHOD_CALL && !node->children.empty())
        return findLeftValueInfo(node->children[0].get());
    return nullptr;
}

// ============================================================================
// Phase 1: Build Symbol Tables
// ============================================================================
void TypeChecker::buildSymbolTables(ASTNode* root) {
    if (!root) return;
    fillTable(root, nullptr);
}

void TypeChecker::fillTable(ASTNode* root, SymbolTable* currentScope) {
    if (!root) return;

    // Create a new scope for braced blocks
    if (root->has_curly_braces) {
        currentScope = createScope(root, currentScope);
    } else {
        root->scope = currentScope;
    }

    // Handle main method: add "main" to method table and "args" to scope
    if (root->node_type == NodeType::MAINMETHOD) {
        SymbolInfo mainInfo;
        mainInfo.name = "main";
        mainInfo.type = DataType::UNDEFINED;
        mainInfo.is_method = true;
        mainInfo.num_params = 1;
        mainInfo.method_scope = currentScope;
        methods_.insert("main", mainInfo);

        SymbolInfo argsInfo;
        argsInfo.name = "args";
        argsInfo.type = DataType::STRING;
        argsInfo.array_dim = 1;
        argsInfo.is_arg = true;
        currentScope->insert("args", argsInfo);
    }

    // Handle variable declarations
    if (root->node_type == NodeType::VAR_DECL) {
        DataType type = DataType::UNDEFINED;
        int dim = 0;
        if (!root->children.empty()) {
            auto* typeNode = root->children[0].get();
            type = findType(typeNode);
            if (typeNode->node_type == NodeType::TYPE_ONE_ARR) dim = 1;
            else if (typeNode->node_type == NodeType::TYPE_TWO_ARR) dim = 2;
        }

        // Check if already declared in current scope
        if (currentScope && currentScope->lookupLocal(root->str_val)) {
            addError(root->line);
        } else if (currentScope) {
            SymbolInfo info;
            info.name = root->str_val;
            info.type = type;
            info.array_dim = dim;
            info.is_static = root->is_static;
            currentScope->insert(root->str_val, info);
        }

        // Handle additional IDs in the same declaration (idExpList)
        for (size_t i = 1; i < root->children.size(); i++) {
            auto* child = root->children[i].get();
            if (child->node_type == NodeType::ID_EXP_LIST) {
                child->scope = currentScope;
                if (currentScope && currentScope->lookupLocal(child->str_val)) {
                    addError(child->line);
                } else if (currentScope) {
                    SymbolInfo info2;
                    info2.name = child->str_val;
                    info2.type = type;
                    info2.array_dim = dim;
                    info2.is_static = root->is_static;
                    currentScope->insert(child->str_val, info2);
                }
            }
        }
    }

    // Handle static method declarations
    if (root->node_type == NodeType::STATIC_METHOD_DECL) {
        DataType retType = DataType::UNDEFINED;
        int retDim = 0;
        if (!root->children.empty()) {
            auto* typeNode = root->children[0].get();
            retType = findType(typeNode);
            if (typeNode->node_type == NodeType::TYPE_ONE_ARR) retDim = 1;
            else if (typeNode->node_type == NodeType::TYPE_TWO_ARR) retDim = 2;
        }

        if (methods_.lookup(root->str_val)) {
            addError(root->line);
        } else {
            SymbolInfo methodInfo;
            methodInfo.name = root->str_val;
            methodInfo.type = retType;
            methodInfo.is_method = true;
            methodInfo.array_dim = retDim;
            // Count parameters
            if (root->children.size() > 1 && root->children[1])
                methodInfo.num_params = countParams(root->children[1].get());
            methodInfo.method_scope = currentScope;
            methods_.insert(root->str_val, methodInfo);
        }
    }

    // Handle assignments: check that variable exists
    if (root->node_type == NodeType::ASSIGN && !root->children.empty()) {
        auto* leftVal = root->children[0].get();
        if (leftVal && currentScope) {
            leftVal->scope = currentScope;
            if (!currentScope->lookup(leftVal->str_val) &&
                !methods_.lookup(leftVal->str_val)) {
                addError(root->line);
            }
        }
    }

    // Handle formal parameters
    if (root->node_type == NodeType::FORMAL_LIST ||
        root->node_type == NodeType::TYPE_ID_LIST) {
        DataType pType = DataType::UNDEFINED;
        int pDim = 0;
        if (!root->children.empty()) {
            auto* typeNode = root->children[0].get();
            pType = findType(typeNode);
            if (typeNode->node_type == NodeType::TYPE_ONE_ARR) pDim = 1;
            else if (typeNode->node_type == NodeType::TYPE_TWO_ARR) pDim = 2;
        }
        if (currentScope) {
            if (currentScope->lookupLocal(root->str_val)) {
                addError(root->line);
            } else {
                SymbolInfo pInfo;
                pInfo.name = root->str_val;
                pInfo.type = pType;
                pInfo.array_dim = pDim;
                pInfo.is_arg = true;
                currentScope->insert(root->str_val, pInfo);
            }
        }
    }

    // Recurse into children
    for (auto& child : root->children) {
        if (child) fillTable(child.get(), currentScope);
    }
}

// ============================================================================
// Phase 2: Type Checking
// ============================================================================
DataType TypeChecker::findExpType(ASTNode* root) {
    if (!root) return DataType::UNDEFINED;
    root = unwrapExp(root);
    if (!root) return DataType::UNDEFINED;

    switch (root->node_type) {
        case NodeType::EXP_TYPE:
            return root->data_type;
        case NodeType::EXP_ONE_ARR:
        case NodeType::EXP_TWO_ARR:
            if (!root->children.empty())
                return findType(root->children[0].get());
            return DataType::UNDEFINED;
        case NodeType::EXP_LENGTH:
            return DataType::INT;
        case NodeType::EXP_LEFT:
            if (!root->children.empty()) {
                auto* info = findLeftValueInfo(root->children[0].get());
                if (info) return info->type;
            }
            return DataType::UNDEFINED;
        case NodeType::EXP_METHOD_CALL:
            if (!root->children.empty()) {
                if (root->children[0]->node_type == NodeType::PARSE_INT)
                    return DataType::INT;
                auto* info = findLeftValueInfo(root->children[0].get());
                if (info) return info->type;
            }
            return DataType::UNDEFINED;
        case NodeType::EXP_PLUS: {
            ASTNode* left = unwrapExp(root->children.size() > 0 ? root->children[0].get() : nullptr);
            return findExpType(left);
        }
        case NodeType::EXP_OPERATION:
            return DataType::INT;
        case NodeType::EXP_OPERATION_COMP:
        case NodeType::EXP_COMP:
        case NodeType::EXP_LOGIC:
            return DataType::BOOLEAN;
        case NodeType::EXP_LOGIC_NOT:
        case NodeType::EXP_POS:
        case NodeType::EXP_NEG:
            if (!root->children.empty())
                return findExpType(root->children[0].get());
            return DataType::UNDEFINED;
        default:
            if (!root->children.empty())
                return findExpType(root->children[0].get());
            return DataType::UNDEFINED;
    }
}

bool TypeChecker::checkTypes(ASTNode* root) {
    if (!root) return true;

    // Recurse first (bottom-up)
    for (auto& child : root->children)
        checkNode(child.get());

    return !hasErrors();
}

void TypeChecker::checkNode(ASTNode* root) {
    if (!root) return;
    for (auto& child : root->children)
        checkNode(child.get());

    switch (root->node_type) {
        case NodeType::EXP_PLUS:
            if (!checkExpPlus(root)) addError(root->line);
            break;
        case NodeType::EXP_OPERATION:
        case NodeType::EXP_OPERATION_COMP:
            if (!checkOperationAndComp(root)) addError(root->line);
            break;
        case NodeType::EXP_LOGIC:
            if (!checkExpLogic(root)) addError(root->line);
            break;
        case NodeType::EXP_COMP:
            if (!checkExpComp(root)) addError(root->line);
            break;
        case NodeType::EXP_LOGIC_NOT:
            if (!checkExpLogicNot(root)) addError(root->line);
            break;
        case NodeType::EXP_POS:
        case NodeType::EXP_NEG:
            if (!checkExpPosNeg(root)) addError(root->line);
            break;
        case NodeType::IF:
        case NodeType::WHILE:
            if (!checkIfWhile(root)) addError(root->line);
            break;
        case NodeType::VAR_DECL:
            if (!checkVarDecl(root)) addError(root->line);
            break;
        case NodeType::ASSIGN:
            if (!checkAssign(root)) addError(root->line);
            break;
        case NodeType::EXP_LENGTH:
            if (!checkExpLength(root)) addError(root->line);
            break;
        case NodeType::LEFTVALUE_ONE_ARR:
        case NodeType::LEFTVALUE_TWO_ARR:
            if (!checkLeftValueArr(root)) addError(root->line);
            break;
        case NodeType::EXP_METHOD_CALL:
        case NodeType::STATEMENT_METHOD_CALL:
            if (!checkMethodCall(root)) addError(root->line);
            break;
        default:
            break;
    }
}

bool TypeChecker::checkExpPlus(ASTNode* node) {
    if (node->children.size() < 2) return false;
    ASTNode* left = unwrapExp(node->children[0].get());
    ASTNode* right = unwrapExp(node->children[1].get());
    DataType lt = findExpType(left);
    DataType rt = findExpType(right);
    if (lt == DataType::UNDEFINED || rt == DataType::UNDEFINED) return true;
    return (lt == DataType::INT && rt == DataType::INT) ||
           (lt == DataType::STRING && rt == DataType::STRING);
}

bool TypeChecker::checkOperationAndComp(ASTNode* node) {
    if (node->children.size() < 2) return false;
    ASTNode* left = unwrapExp(node->children[0].get());
    ASTNode* right = unwrapExp(node->children[1].get());
    DataType lt = findExpType(left);
    DataType rt = findExpType(right);
    if (lt == DataType::UNDEFINED || rt == DataType::UNDEFINED) return true;
    return lt == DataType::INT && rt == DataType::INT;
}

bool TypeChecker::checkExpComp(ASTNode* node) {
    if (node->children.size() < 2) return false;
    ASTNode* left = unwrapExp(node->children[0].get());
    ASTNode* right = unwrapExp(node->children[1].get());
    DataType lt = findExpType(left);
    DataType rt = findExpType(right);
    if (lt == DataType::UNDEFINED || rt == DataType::UNDEFINED) return true;
    return (lt == DataType::INT && rt == DataType::INT) ||
           (lt == DataType::BOOLEAN && rt == DataType::BOOLEAN);
}

bool TypeChecker::checkExpLogic(ASTNode* node) {
    if (node->children.size() < 2) return false;
    ASTNode* left = unwrapExp(node->children[0].get());
    ASTNode* right = unwrapExp(node->children[1].get());
    DataType lt = findExpType(left);
    DataType rt = findExpType(right);
    if (lt == DataType::UNDEFINED || rt == DataType::UNDEFINED) return true;
    return lt == DataType::BOOLEAN && rt == DataType::BOOLEAN;
}

bool TypeChecker::checkExpLogicNot(ASTNode* node) {
    if (node->children.empty()) return false;
    ASTNode* child = unwrapExp(node->children[0].get());
    DataType t = findExpType(child);
    if (t == DataType::UNDEFINED) return true;
    return t == DataType::BOOLEAN;
}

bool TypeChecker::checkExpPosNeg(ASTNode* node) {
    if (node->children.empty()) return false;
    ASTNode* child = unwrapExp(node->children[0].get());
    DataType t = findExpType(child);
    if (t == DataType::UNDEFINED) return true;
    return t == DataType::INT;
}

bool TypeChecker::checkIfWhile(ASTNode* node) {
    if (node->children.empty()) return false;
    ASTNode* cond = unwrapExp(node->children[0].get());
    DataType t = findExpType(cond);
    if (t == DataType::UNDEFINED) return true;
    return t == DataType::BOOLEAN || t == DataType::INT;
}

bool TypeChecker::checkVarDecl(ASTNode* node) {
    // If there's an initializer, check type compatibility
    if (node->children.size() < 2) return true;
    auto* initNode = node->children[1].get();
    if (initNode->node_type != NodeType::INIT) return true;
    if (initNode->children.empty()) return true;

    DataType declType = findType(node->children[0].get());
    DataType initType = findExpType(initNode->children[0].get());
    if (declType == DataType::UNDEFINED || initType == DataType::UNDEFINED) return true;
    return declType == initType;
}

bool TypeChecker::checkAssign(ASTNode* node) {
    if (node->children.size() < 2) return false;
    auto* left = node->children[0].get();
    auto* right = unwrapExp(node->children[1].get());
    auto* info = findLeftValueInfo(left);
    if (!info) return false;

    DataType lt = info->type;
    DataType rt = findExpType(right);
    if (lt == DataType::UNDEFINED || rt == DataType::UNDEFINED) return true;
    return lt == rt;
}

bool TypeChecker::checkExpLength(ASTNode* node) {
    if (node->children.empty()) return false;
    auto* child = node->children[0].get();
    auto* info = findLeftValueInfo(child);
    if (!info) return false;
    return info->array_dim > 0;
}

bool TypeChecker::checkLeftValueArr(ASTNode* node) {
    auto* info = findLeftValueInfo(node);
    if (!info) return false;
    if (node->node_type == NodeType::LEFTVALUE_ONE_ARR)
        return info->array_dim >= 1;
    if (node->node_type == NodeType::LEFTVALUE_TWO_ARR)
        return info->array_dim >= 2;
    return true;
}

bool TypeChecker::checkMethodCall(ASTNode* node) {
    if (node->children.empty()) return true;
    auto* callNode = node->children[0].get();
    if (callNode->node_type == NodeType::PARSE_INT) return true;
    auto* info = methods_.lookup(callNode->str_val);
    if (!info) return false;
    return true;
}
