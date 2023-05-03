#ifndef TYPE_CHECK_HH
#define TYPE_CHECK_HH

#include "node.hh"
#include <string>

struct SymbolTableEntry {
    const char * id;
    enum DataType type;
    bool is_method;
    int number_of_parameters;
    int dim_of_array;
    bool is_static;
    int offset_number; // real offset(4*x)
    bool is_arg;
    int entry_in_table;
    struct SymbolTable * correspond_scope;

    SymbolTableEntry() {
        is_arg = false;
    }
    
};

struct SymbolTable {
    struct SymbolTableEntry * entries[50];
    struct SymbolTable * parent;
    int num_entries;
};

struct String {
    char * string;
    struct String * next;
};

struct TypeList {
    enum DataType type;
    struct SymbolTableEntry * info;
    struct ASTNode * node;
    struct TypeList * next;
};
// Adds an entry to the symbol table.
void add_to_symbol_table(char* id, struct ASTNode * root, enum DataType type);

// Looks for an entry in the symbol table with the given name.
// Returns NULL if there is no such entry.
struct SymbolTableEntry * find_symbol(struct SymbolTable* symbol_table, char * id, enum DataType type, bool is_static, bool is_assign);
enum DataType FindType(struct ASTNode * root);
struct SymbolTableEntry * find_method(char * id);
struct SymbolTableEntry * findLeftValueID(struct ASTNode* root);
void checkProgram(struct ASTNode* program);
void checkMain(struct ASTNode* mainClass);
void traverse_and_fill_table(struct ASTNode * root);
bool has_error();


struct String* concatenateLists(struct String* head1, struct String* head2);
void add_to_error_array(int line_number);
bool is_in_array(int* array, int num, int size);
struct ASTNode* exploreNodeTypEXP(struct ASTNode* root);
struct ASTNode* exploreNodeTypEXP_Parent(struct ASTNode* root);
enum DataType FindExpType (struct ASTNode * root);
bool checkExpPlus(struct ASTNode* root);
bool checkOperationAndComp(struct ASTNode* root);
bool checkExpComp(struct ASTNode * root);
bool checkExpLogic(struct ASTNode * root);
bool checkExpLogicNot(struct ASTNode * root);
bool checkExpPosNeg(struct ASTNode * root);
bool checkIfWhile(struct ASTNode * root);
bool checkVarDecl(struct ASTNode * root);
bool checkAssign(struct ASTNode * root);
bool checkExpLength(struct ASTNode * root);
bool checkParseInt(struct ASTNode * root);
bool checkExpMethodCall(struct ASTNode * root);
bool checkLeftValueArr(struct ASTNode * root);

struct TypeList * exploreExpCommaListType(struct ASTNode * root);
std::string enum_string(enum NodeType type);




extern int num_errors;
extern int num_entries;


#endif
