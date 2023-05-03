#ifndef NODE_HH
#define NODE_HH
#define MAX_NUM_CHILDREN 3

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <stack>
#include <tuple>

enum DataType { DATATYPE_UNDEFINED, DATATYPE_STR, DATATYPE_INT, DATATYPE_BOOLEAN };

// Returns the name of the given data type.
static inline const char *type_string(enum DataType t) {
    static const char *names[] = {"Undefined", "String", "Integer", "Boolean"};
    return names[t % 4];
}

struct SemanticData {
    enum DataType type;
    union value_t {
        char* string_value;
        int int_value;
        bool boolean_value;
    } value;
};

enum NodeType {
    NODETYPE_PROGRAM,
    NODETYPE_MAINCLASS,
    NODETYPE_MAINMETHOD,
    NODETYPE_STATICVARDECLLIST,
    NODETYPE_STATICMETHODDECLLIST,
    NODETYPE_STATEMENTLIST,
    NODETYPE_INIT,
    NODETYPE_IDEXPLIST,
    NODETYPE_STATICVARDECL,
    NODETYPE_STATICMETHODDECL,
    NODETYPE_FORMALLIST,
    NODETYPE_TYPEIDLIST,
    NODETYPE_TYPEIDLIST_ID,
    NODETYPE_PRIMETYPE,

    NODETYPE_VARDECL,
    NODETYPE_PRINT,
    NODETYPE_PRINTLN,
    NODETYPE_PARSEINT,
    NODETYPE_STATEMENT,
    NODETYPE_IF,
    NODETYPE_WHILE,
    NODETYPE_ASSIGN,
    NODETYPE_RETURN,
    NODETYPE_METHODCALL,
    NODETYPE_TYPE,
    NODETYPE_EXP,
    NODETYPE_INDEX,
    NODETYPE_EXPLIST,
    NODETYPE_COMMAEXPLIST,
    NODETYPE_LEFTVALUE_ID,

    NODETYPE_EXP_POS,
    NODETYPE_EXP_NEG,
    NODETYPE_EXP_LOGIC,
    NODETYPE_EXP_COMP,
    NODETYPE_EXP_OPERATION,
    NODETYPE_EXP_PLUS,
    NODETYPE_EXP_TYPE,
    NODETYPE_EXP_LOGIC_NOT,
    NODETYPE_LEFTVALUE_ARR,
    NODETYPE_EXP_OPERATION_COMP,
    NODETYPE_TYPE_ONE_ARR,
    NODETYPE_TYPE_TWO_ARR,
    NODETYPE_EXP_ONE_ARR,
    NODETYPE_EXP_TWO_ARR,
    NODETYPE_EXP_LENGTH,
    NODETYPE_EXP_LEFT,
    NODETYPE_EXP_METHODCALL,
    NODETYPE_LEFTVALUE_ONE_ARR,
    NODETYPE_LEFTVALUE_TWO_ARR,
    NODETYPE_CALL_MAIN,
    NODETYPE_TYPE_ARR,
    NODETYPE_EXP_ARR,
    NODETYPE_BRACKETEXPLIST,
    NODETYPE_STATEMENT_METHODCALL,
};

enum InstructionType {
    LDR_LITERAL,
    LDR_FROM_MEM,
    MOV_LEFTVAL,
    ADD_MINUS_MUL,
    MOV_CMP,
    AND_OR,
    LSL,
    STR_ASSIGN_VARDECL,
    NO_NEED_INDENT, // for if, while, do, then, else
    NEG,
    POS,
    LOGIC_NOT,
    EXP_LENGTH,
    IF_WHILE_CMP,
    ONE_OPERAND,
    TWO_OPERAND,
    THREE_OPERAND,
    METHOD_CALL,
    BRANCH,
    PRINTLN,
    LDR_INT_FORMAT,
    COMPARE_TWO_REG,
    LDR_VAR,
    OTHERS,
};

struct Node {
    std::string name;
    std::string offset;
    int assigned_register;
    std::set<Node*> neighbors;
    int degree;
    bool is_LVout_of_method;
    Node() {
        is_LVout_of_method = false;
        assigned_register = -1;
    }
};

struct method_scope {
    bool callee;
    int symbolic_register_counter;
    std::set<int> callee_save_register_remain;
    std::set<int> callee_save_register_in_use;
    std::set<int> register_remain;
    std::set<int> register_in_use;
    std::stack<Node *> coloring_stack;
    std::unordered_map<std::string, Node*> node_table;
    std::vector<int> return_instruction_id;
    method_scope() {
        callee = true;
        callee_save_register_remain = {4, 5, 6, 7, 8, 9, 10, 11, 12};
        register_remain = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    }
};

struct instruction {
    struct ASTNode * root;
    std::string instr;

    // use to assign register
    int reg_number;
    int is_left; // destination register
    int operand1_number;
    int operand2_number;

    int instruction_id;
    int block_id;
    int argument_number;
    int message_number;
    int branch_number;

    
    bool is_if;
    bool is_then;
    bool is_else;
    bool is_while;
    bool is_do;
    bool is_operation;
    bool is_return;
    struct SymbolTableEntry * entry;
    enum InstructionType instrType;

    std::set<std::string> use;
    std::set<std::string> def;
    std::vector<int> predecessor;
    std::vector<int> successor;
    std::set<std::string> LVout;
    std::set<std::string> LVin;

    instruction() {
        root = NULL;
        is_left = -1;
        is_if = false;
        is_then = false;
        is_else = false;
        is_while = false;
        is_do = false;
        is_operation = false;
        is_return = false;
        argument_number = 0;
        instrType = OTHERS;
        instr = " ";
    }
};

struct ASTNode {
    struct ASTNode* children[MAX_NUM_CHILDREN];
    struct ASTNode* parent;
    struct SymbolTable* symbol_table;
    struct method_scope* method_scope;
    struct instruction instr;
    
    bool has_curly_braces;
    int num_children;
    int line_number;
    bool is_static;
    int child_index;
    
    enum NodeType node_type;
    struct SemanticData data;
};


// Creates a new node with 0 children on the heap using `malloc()`.
struct ASTNode* new_node(enum NodeType t, int line_number);
// Adds the given children to the parent node. Returns -1 if the capacity is full.
int add_child(struct ASTNode* parent, struct ASTNode* child);
void assign_parent(struct ASTNode* parent, struct ASTNode* child);
// Sets the data of the node to the given value and the corresponding type.

void set_string_value(struct ASTNode* node, char* s);
void set_int_value(struct ASTNode* node, int i);
void set_boolean_value(struct ASTNode* node, bool b);
void traverse_and_fill_table(struct ASTNode * root);
void traverse_and_check_type(struct ASTNode * root);

void print_tree_with_lines(struct ASTNode* node , int indent);
#endif
