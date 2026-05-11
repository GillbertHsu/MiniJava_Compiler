#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

enum class DataType { UNDEFINED, STRING, INT, BOOLEAN };

inline const char* dataTypeStr(DataType t) {
    switch (t) {
        case DataType::STRING:  return "String";
        case DataType::INT:     return "Integer";
        case DataType::BOOLEAN: return "Boolean";
        default:                return "Undefined";
    }
}

enum class NodeType {
    PROGRAM, MAINCLASS, MAINMETHOD,
    STATIC_VAR_DECL_LIST, STATIC_METHOD_DECL_LIST, STATEMENT_LIST,
    INIT, ID_EXP_LIST,
    STATIC_VAR_DECL, STATIC_METHOD_DECL,
    FORMAL_LIST, TYPE_ID_LIST, TYPE_ID_LIST_ID,
    PRIME_TYPE, TYPE, TYPE_ONE_ARR, TYPE_TWO_ARR,
    VAR_DECL, STATEMENT,
    PRINT, PRINTLN, PARSE_INT,
    IF, WHILE, ASSIGN, RETURN,
    METHOD_CALL, CALL_MAIN, STATEMENT_METHOD_CALL,
    EXP, EXP_LIST, COMMA_EXP_LIST,
    EXP_PLUS, EXP_OPERATION, EXP_OPERATION_COMP,
    EXP_LOGIC, EXP_COMP,
    EXP_POS, EXP_NEG, EXP_LOGIC_NOT,
    EXP_TYPE, EXP_LEFT,
    EXP_ONE_ARR, EXP_TWO_ARR, EXP_LENGTH,
    EXP_METHOD_CALL,
    LEFTVALUE_ID, LEFTVALUE_ONE_ARR, LEFTVALUE_TWO_ARR,
    BRACKET_EXP_LIST,
};

class SymbolTable;

struct ASTNode {
    NodeType node_type;
    int line = 0;

    DataType data_type = DataType::UNDEFINED;
    std::string str_val;
    int int_val = 0;
    bool bool_val = false;

    std::vector<std::unique_ptr<ASTNode>> children;
    ASTNode* parent = nullptr;

    SymbolTable* scope = nullptr;
    bool is_static = false;
    bool has_curly_braces = false;
};

inline std::unique_ptr<ASTNode> makeNode(NodeType type, int line) {
    auto n = std::make_unique<ASTNode>();
    n->node_type = type;
    n->line = line;
    return n;
}

inline void addChild(ASTNode* parent, std::unique_ptr<ASTNode> child) {
    if (!child) return;
    child->parent = parent;
    parent->children.push_back(std::move(child));
}

#endif
