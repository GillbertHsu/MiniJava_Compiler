#include "typecheck.hh"
#include "node.hh"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int num_errors = 0;
int num_entries = 0;
int error_array[100];
struct SymbolTableEntry * method_symbol_table[50];


std::string enum_string(enum NodeType type) {
    if (type == NODETYPE_PROGRAM)return "NODETYPE_PROGRAM"; 
    if (type == NODETYPE_MAINCLASS)return "NODETYPE_MAINCLASS";
    if (type == NODETYPE_MAINMETHOD)return "NODETYPE_MAINMETHOD";
    if (type == NODETYPE_STATICVARDECLLIST)return "NODETYPE_STATICVARDECLLIST";
    if (type == NODETYPE_STATICMETHODDECLLIST)return "NODETYPE_STATICMETHODDECLLIST";
    if (type == NODETYPE_STATEMENTLIST)return "NODETYPE_STATEMENTLIST";
    if (type == NODETYPE_INIT)return "NODETYPE_INIT";
    if (type == NODETYPE_IDEXPLIST) return "NODETYPE_IDEXPLIST";
    if (type == NODETYPE_STATICVARDECL)return "NODETYPE_STATICVARDECL";
    if (type == NODETYPE_STATICMETHODDECL)return "NODETYPE_STATICMETHODDECL";
    if (type == NODETYPE_FORMALLIST)return "NODETYPE_FORMALLIST";
    if (type == NODETYPE_TYPEIDLIST)return "NODETYPE_TYPEIDLIST";
    if (type == NODETYPE_PRIMETYPE)return "NODETYPE_PRIMETYPE";
    if (type == NODETYPE_VARDECL)return "NODETYPE_VARDECL";
    if (type == NODETYPE_PRINT)return "NODETYPE_PRINT";
    if (type == NODETYPE_PRINTLN)return "NODETYPE_PRINTLN";
    if (type == NODETYPE_STATEMENT)return "NODETYPE_STATEMENT";
    if (type == NODETYPE_IF)return "NODETYPE_IF";
    if (type == NODETYPE_WHILE)return "NODETYPE_WHILE";
    if (type == NODETYPE_ASSIGN)return "NODETYPE_ASSIGN";
    if (type == NODETYPE_RETURN)return "NODETYPE_RETURN";
    if (type == NODETYPE_METHODCALL)return "NODETYPE_METHODCALL";
    if (type == NODETYPE_TYPE)return "NODETYPE_TYPE";
    if (type == NODETYPE_EXP)return "NODETYPE_EXP";
    if (type == NODETYPE_INDEX)return "NODETYPE_INDEX";
    if (type == NODETYPE_EXPLIST)return "NODETYPE_EXPLIST";
    if (type == NODETYPE_COMMAEXPLIST)return "NODETYPE_COMMAEXPLIST";
    if (type == NODETYPE_LEFTVALUE_ARR)return "NODETYPE_LEFTVALUE_ARR";
    if (type == NODETYPE_LEFTVALUE_ID)return "NODETYPE_LEFTVALUE_ID";
    if (type == NODETYPE_TYPEIDLIST_ID)return "NODETYPE_TYPEIDLIST_ID";
    if (type == NODETYPE_EXP_POS)return "NODETYPE_EXP_POS";
    if (type == NODETYPE_EXP_NEG)return "NODETYPE_EXP_NEG";
    if (type == NODETYPE_EXP_LOGIC)return "NODETYPE_EXP_LOGIC";
    if (type == NODETYPE_EXP_COMP)return  "NODETYPE_EXP_COMP";
    if (type == NODETYPE_EXP_OPERATION)return "NODETYPE_EXP_OPERATION";
    if (type == NODETYPE_EXP_PLUS)return "NODETYPE_EXP_PLUS";
    if (type == NODETYPE_EXP_TYPE)return "NODETYPE_EXP_TYPE";
    if (type == NODETYPE_EXP_LOGIC_NOT)return "NODETYPE_EXP_LOGIC_NOT";
    if (type == NODETYPE_LEFTVALUE_ARR)return "NODETYPE_LEFTVALUE_ARR";
    if (type == NODETYPE_EXP_OPERATION_COMP)return "NODETYPE_EXP_OPERATION_COMP";
    if (type == NODETYPE_TYPE_ONE_ARR)return "NODETYPE_TYPE_ONE_ARR";
    if (type == NODETYPE_TYPE_TWO_ARR)return "NODETYPE_TYPE_TWO_ARR"; 
    if (type == NODETYPE_EXP_ONE_ARR)return "NODETYPE_EXP_ONE_ARR"; 
    if (type == NODETYPE_EXP_TWO_ARR)return "NODETYPE_EXP_TWO_ARR"; 
    if (type == NODETYPE_EXP_LENGTH)return "NODETYPE_EXP_LENGTH";
    if (type == NODETYPE_EXP_LEFT)return "NODETYPE_EXP_LEFT";
    if (type == NODETYPE_EXP_METHODCALL)return "NODETYPE_EXP_METHODCALL";
    if (type == NODETYPE_LEFTVALUE_ONE_ARR)return "NODETYPE_LEFTVALUE_ONE_ARR";
    if (type == NODETYPE_LEFTVALUE_TWO_ARR )return "NODETYPE_LEFTVALUE_TWO_ARR";
    if (type == NODETYPE_CALL_MAIN )return  "NODETYPE_CALL_MAIN";
    if (type == NODETYPE_PARSEINT )return "NODETYPE_PARSEINT";
    if (type == NODETYPE_TYPE_ARR)return "NODETYPE_TYPE_ARR";
    if (type == NODETYPE_EXP_ARR)return "NODETYPE_EXP_ARR";
    if (type == NODETYPE_STATEMENT_METHODCALL)return "NODETYPE_STATEMENT_METHODCALL";
    return "NODETYPE_UNKNOWN";
}

int compare(const void * a, const void * b) {
    return (*(int*)a - *(int*)b);
}

static void report_type_violation() {
    qsort(error_array, num_errors, sizeof(int), compare);
    for (int i = 0 ; i < num_errors ; i++) {
        fprintf(stderr, "Type violation in line %d\n", error_array[i]);
    }
}

void checkProgram(struct ASTNode * program){
    report_type_violation();
}

bool has_error() {
    return num_errors > 0;
}

char* concat_str(char* s1, char* s2) {
    strcat(s1, s2);
    return s1;
}

enum DataType FindType(struct ASTNode * root) {
    if (root == NULL) return DATATYPE_UNDEFINED;
    if (root -> node_type == NODETYPE_PRIMETYPE) {
        return root -> data.type;
    }
    FindType(root->children[0]);
}

struct SymbolTableEntry * find_method(char * id) {
    for (int i = 0 ; i < num_entries ; i++) {
        if (strcmp(method_symbol_table[i]->id, id) == 0) {
            return method_symbol_table[i];
        }
    }
    return NULL;
}

char * FindID(struct ASTNode * root) {
    if (root == NULL) return NULL;
    if (root -> node_type == NODETYPE_LEFTVALUE_ID || root -> node_type == NODETYPE_LEFTVALUE_ONE_ARR || root -> node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
        return root -> data.value.string_value;
    }
    FindID(root->children[0]);
}

void find_argument_numbers(struct ASTNode * root, struct SymbolTableEntry * entry ) {
    if (root == NULL) return;
    if (root->node_type == NODETYPE_TYPEIDLIST || root->node_type == NODETYPE_FORMALLIST) {
        entry->number_of_parameters++;
    }
    find_argument_numbers(root->children[1], entry);
}

struct String * explore_idExpList(struct ASTNode * root) {
    if (root == NULL) return NULL;
    if (root->node_type == NODETYPE_IDEXPLIST) {
        struct String * id_list = (struct String *) malloc(sizeof(struct String));
        id_list->string = root->data.value.string_value;
        struct String * rest = explore_idExpList(root->children[0]);
        if (rest == NULL) return id_list;
        return concatenateLists(id_list, rest);
    }
    return NULL;
}

void traverse_and_fill_table(struct ASTNode * root) {
    if (root == NULL) return;

    if (root->has_curly_braces) {
        root->symbol_table = (struct SymbolTable *) malloc(sizeof(struct SymbolTable));
        root->symbol_table->num_entries = 0;
        if (root->node_type == NODETYPE_STATEMENTLIST || root->node_type == NODETYPE_STATEMENT ) {
            root->method_scope = root->parent->method_scope;
        } else {
            root->method_scope = new method_scope();
            root->method_scope->symbolic_register_counter = 0;
        }
        if (root->parent != NULL) {
            root->symbol_table->parent = root->parent->symbol_table;
        }
        if (root->node_type == NODETYPE_MAINMETHOD) {
            method_symbol_table[num_entries] = (struct SymbolTableEntry *) malloc(sizeof(struct SymbolTableEntry));
            std::string id = "main";
            method_symbol_table[num_entries]->id = id.c_str();
            method_symbol_table[num_entries]->type = DATATYPE_UNDEFINED;
            method_symbol_table[num_entries]->number_of_parameters = 1;
            num_entries++;
            
            struct SymbolTableEntry* entry = (struct SymbolTableEntry *) malloc(sizeof(struct SymbolTableEntry));
            struct SymbolTable * table = root->symbol_table;
            root->method_scope->symbolic_register_counter = 1;
            root->method_scope->callee = false;
            entry->id = "args";
            entry->type = DATATYPE_STR;
            entry->is_static = false;
            entry->is_method = false;
            entry->dim_of_array = 1;
            table->entries[table->num_entries] = entry;
            table->num_entries++;
        }
    } else if (root->parent != NULL) {
        root->symbol_table = root->parent->symbol_table;
        root->method_scope = root->parent->method_scope;
    }
    if (root->node_type == NODETYPE_VARDECL) {
        struct String * id_list = (struct String *) malloc(sizeof(struct String));
        id_list->string = root->data.value.string_value;
        if (root->children[root->num_children-1]->node_type == NODETYPE_IDEXPLIST) {
           struct String * rest = explore_idExpList(root->children[root->num_children-1]);
           id_list = concatenateLists(id_list, rest);
        }
        if (id_list->string != NULL) {
            enum DataType type = FindType(root->children[0]);
            while (id_list != NULL) {
                if (find_symbol(root->symbol_table, id_list->string, type, root->is_static, false) != NULL) {
                    add_to_error_array(root->line_number);
                } else {
                    add_to_symbol_table(id_list->string, root, type);
                }
                id_list = id_list->next;
            }
        }
        
    } else if (root->node_type == NODETYPE_STATICMETHODDECL) {
        char * id = root->data.value.string_value;
        enum DataType type = FindType(root->children[0]);
        if (find_method(id) != NULL) {
            add_to_error_array(root->line_number);
        } else {
            method_symbol_table[num_entries] = (struct SymbolTableEntry *) malloc(sizeof(struct SymbolTableEntry));
            method_symbol_table[num_entries]->id = id;
            method_symbol_table[num_entries]->type = type;
            method_symbol_table[num_entries]->number_of_parameters = 0;
            method_symbol_table[num_entries]->is_method = true;
            method_symbol_table[num_entries]->dim_of_array = 0;
            if (root->children[0]->node_type == NODETYPE_TYPE_ONE_ARR) {
                method_symbol_table[num_entries]->dim_of_array = 1;
            } else if (root->children[0]->node_type == NODETYPE_TYPE_TWO_ARR) {
                method_symbol_table[num_entries]->dim_of_array = 2;
            }
            find_argument_numbers(root->children[1], method_symbol_table[num_entries]);
            method_symbol_table[num_entries]->correspond_scope = root->symbol_table;
            struct SymbolTable * table = root->symbol_table;
            root->method_scope->symbolic_register_counter = method_symbol_table[num_entries]->number_of_parameters;
            num_entries++;
        }
    } else if (root->node_type == NODETYPE_ASSIGN) {
        // Maybe need to change symbol table for future project
        char * id = FindID(root->children[0]);
        enum DataType type = DATATYPE_UNDEFINED;
        if (find_symbol(root->symbol_table, id, type , false, true) == NULL) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_FORMALLIST) {
        char * id = root->data.value.string_value;
        enum DataType type = FindType(root->children[0]);
        if (find_symbol(root->symbol_table, id, type, false, false) != NULL) {
            add_to_error_array(root->line_number);
        } else {
            add_to_symbol_table(id, root, type);
            struct SymbolTableEntry * entry = find_symbol(root->symbol_table, id, type, false, false);
            entry->offset_number = (root->symbol_table->num_entries - 1)*4;
            entry->is_arg = true;
        }
    } else if (root->node_type == NODETYPE_TYPEIDLIST) {
        char * id = root->data.value.string_value;
        enum DataType type = FindType(root->children[0]);
        if (find_symbol(root->symbol_table, id, type, false, false) != NULL) {
            add_to_error_array(root->line_number);
        } else {
            add_to_symbol_table(id, root, type);
            struct SymbolTableEntry * entry = find_symbol(root->symbol_table, id, type, false, false);
            entry->offset_number = (root->symbol_table->num_entries - 1)*4;
            entry->is_arg = true;
        }
    }
    for (int i = 0; i < root->num_children; i++) {
        traverse_and_fill_table(root->children[i]);
    }
}

struct ASTNode* exploreNodeTypEXP(struct ASTNode* root) {
    if (root->node_type != NODETYPE_EXP){
        return root;
    }
    exploreNodeTypEXP(root->children[0]);
    
}
struct ASTNode* exploreNodeTypEXP_Parent(struct ASTNode* root) {
    if (root->node_type != NODETYPE_EXP){
        return root;
    }
    exploreNodeTypEXP_Parent(root->parent);
    
}

enum DataType FindExpType (struct ASTNode * root) {
    if (root == NULL) return DATATYPE_UNDEFINED;
    //FindExpType(root->children[0]);
    if (root->node_type == NODETYPE_EXP_TYPE) {
        return root->data.type;
    } else if (root->node_type == NODETYPE_EXP_ONE_ARR || root->node_type == NODETYPE_EXP_TWO_ARR) {
        return FindType(root->children[0]);
    } else if (root->node_type == NODETYPE_EXP_OPERATION_COMP || root->node_type == NODETYPE_EXP_LOGIC) {
        if (checkOperationAndComp(root) == false) {
            return DATATYPE_UNDEFINED;
        } else {
            return DATATYPE_BOOLEAN;
        }
    } else if (root->node_type == NODETYPE_EXP_COMP) {
        if (checkExpComp(root) == false) {
            return DATATYPE_UNDEFINED;
        } else {
            return DATATYPE_BOOLEAN;
        }
    } else if (root->node_type == NODETYPE_EXP_LENGTH) {
        return DATATYPE_INT;
    } else if (root->node_type == NODETYPE_EXP_LEFT) {
        if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR || root->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            if (checkLeftValueArr(root->children[0]) == false) {
                return DATATYPE_UNDEFINED;
            } else {
                struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
                return entry->type;
            }
        }
        struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
        return entry->type;
    } else if (root->node_type == NODETYPE_EXP_METHODCALL) {
        if (root->children[0]->node_type == NODETYPE_PARSEINT) {
            return DATATYPE_INT;
        }
        struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
        return entry->type;
    } else if (root->node_type == NODETYPE_EXP_PLUS) {
        if (checkExpPlus(root) == false) { 
            return DATATYPE_UNDEFINED;
        } else {
            struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
            return FindExpType(left);
        }
    } else if (root->node_type == NODETYPE_EXP_OPERATION || root->node_type ==  NODETYPE_EXP_OPERATION_COMP) {
        if (checkOperationAndComp(root) == false) {
            return DATATYPE_UNDEFINED;
        } else {
            struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
            return FindExpType(left);
        }
    } else if (root->node_type == NODETYPE_EXP_LOGIC_NOT) {
        if (checkExpLogicNot(root) == false) {
            return DATATYPE_UNDEFINED;
        } else {
            struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
            return FindExpType(left);
        }
    } else if (root->node_type == NODETYPE_EXP_LOGIC) {
        if (checkExpLogic(root) == false) {
            return DATATYPE_UNDEFINED;
        } else {
            struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
            return FindExpType(left);
        }
    } else if (root->node_type == NODETYPE_EXP_POS || root->node_type == NODETYPE_EXP_NEG) {
        if (checkExpPosNeg(root) == false) {
            return DATATYPE_UNDEFINED;
        } else {
            struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
            return FindExpType(left);
        }
    } 
    FindExpType(root->children[0]);
}

void checkIDExpList(struct ASTNode * root, enum DataType type, int dim_of_array) {
    // Everything here need to be checked
    if (root == NULL) return;
    if (root->node_type != NODETYPE_EXP_TYPE && root->node_type != NODETYPE_EXP_ONE_ARR 
        && root->node_type != NODETYPE_EXP_TWO_ARR && root->node_type != NODETYPE_LEFTVALUE_ID 
        && root->node_type != NODETYPE_LEFTVALUE_ONE_ARR && root->node_type != NODETYPE_LEFTVALUE_TWO_ARR
        && root->node_type != NODETYPE_EXP_METHODCALL && root->node_type != NODETYPE_EXP_PLUS
        && root->node_type != NODETYPE_EXP_OPERATION && root->node_type != NODETYPE_EXP_OPERATION_COMP
        && root->node_type != NODETYPE_EXP_LOGIC_NOT && root->node_type != NODETYPE_EXP_LOGIC
        && root->node_type != NODETYPE_EXP_POS && root->node_type != NODETYPE_EXP_NEG) {
        checkIDExpList(root->children[0], type, dim_of_array);
        if (root->num_children == 2)checkIDExpList(root->children[1], type, dim_of_array);
    } else if (root->node_type == NODETYPE_EXP_TYPE) {
        if ((FindExpType(root) != type && FindExpType(root) != DATATYPE_UNDEFINED) || dim_of_array != 0) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_ONE_ARR) {
        if ((FindExpType(root) != type && FindExpType(root) != DATATYPE_UNDEFINED) || dim_of_array != 1) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_TWO_ARR) {
        if ((FindExpType(root) != type && FindExpType(root) != DATATYPE_UNDEFINED) || dim_of_array != 2) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_LEFTVALUE_ID) {
        struct SymbolTableEntry * entry = findLeftValueID(root);
        if (entry->type != type || entry->dim_of_array != dim_of_array) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_METHODCALL) {
        if (root->children[0]->node_type == NODETYPE_PARSEINT && (type != DATATYPE_INT || dim_of_array != 0)) {
            add_to_error_array(root->line_number);
        } else {
            struct SymbolTableEntry * entry = findLeftValueID(root);
            if (entry->type != type || entry->dim_of_array != dim_of_array || root->children[0]->node_type == NODETYPE_CALL_MAIN) {
                add_to_error_array(root->line_number);
            }
        }
    } else if (root->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
        struct SymbolTableEntry * entry = findLeftValueID(root);
        if (entry->type != type || entry->dim_of_array-1 != dim_of_array) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
        struct SymbolTableEntry * entry = findLeftValueID(root);
        if (entry->type != type || entry->dim_of_array-2 != dim_of_array) {
            add_to_error_array(root->line_number);
        }
    } else {
        if (FindExpType(root) != type && FindExpType(root) != DATATYPE_UNDEFINED) {
            add_to_error_array(root->line_number);
        }
    }
}

void traverse_and_check_type(struct ASTNode * root) { 
    // need to use exploreNodeTypEXP
    if (root == NULL) return;
    for (int i = 0; i < root->num_children; i++) {
        traverse_and_check_type(root->children[i]);
    }
    if (root->node_type == NODETYPE_EXP_PLUS) {
        if (checkExpPlus(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_OPERATION || root->node_type == NODETYPE_EXP_OPERATION_COMP) {
        if (checkOperationAndComp(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_LOGIC) {
        if (checkExpLogic(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_COMP) {
        if (checkExpComp(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_LOGIC_NOT) {
        if (checkExpLogicNot(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_POS || root->node_type == NODETYPE_EXP_NEG ) {
        if (checkExpPosNeg(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_IF || root->node_type == NODETYPE_WHILE) {
        if (checkIfWhile(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_VARDECL) {
        if (checkVarDecl(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_LENGTH) {
        if (checkExpLength(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_LEFTVALUE_ONE_ARR || root->node_type == NODETYPE_LEFTVALUE_TWO_ARR) { // forget to check if it is int index 
        // if call array id, its dim must be valid
        if (checkLeftValueArr(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_ASSIGN) {
        if (checkAssign(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_EXP_ONE_ARR) { // check if array new array [x] x is int. Deal with array index to be int
        struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
        if (FindExpType(right) != DATATYPE_INT || right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR) {
            add_to_error_array(root->line_number);
        } else if (right->node_type == NODETYPE_EXP_LEFT) {
            struct SymbolTableEntry * entry = findLeftValueID(right->children[0]);
            if (entry != NULL && entry->dim_of_array != 0) {
                add_to_error_array(root->line_number);
            }
        }
    } else if (root->node_type == NODETYPE_EXP_TWO_ARR) {
        struct ASTNode * right1 = exploreNodeTypEXP(root->children[1]);
        struct ASTNode * right2 = exploreNodeTypEXP(root->children[2]);
        if (FindExpType(right1) != DATATYPE_INT || right1->node_type == NODETYPE_EXP_ONE_ARR || right1->node_type == NODETYPE_EXP_TWO_ARR 
        || FindExpType(right2) != DATATYPE_INT || right2->node_type == NODETYPE_EXP_ONE_ARR || right2->node_type == NODETYPE_EXP_TWO_ARR) {
            add_to_error_array(root->line_number);
        } else if (right1->node_type == NODETYPE_EXP_LEFT) {
            struct SymbolTableEntry * entry = findLeftValueID(right1->children[0]);
            if (entry != NULL && entry->dim_of_array != 0) {
                add_to_error_array(root->line_number);
            }
        } else if (right2->node_type == NODETYPE_EXP_LEFT) {
            struct SymbolTableEntry * entry = findLeftValueID(right2->children[0]);
            if (entry != NULL && entry->dim_of_array != 0) {
                add_to_error_array(root->line_number);
            }
        }
    } else if (root->node_type == NODETYPE_EXP_METHODCALL || root->node_type == NODETYPE_STATEMENT_METHODCALL) {
        if (checkExpMethodCall(root) == false) {
            add_to_error_array(root->line_number);
        }
    } else if (root->node_type == NODETYPE_PRINTLN || root->node_type == NODETYPE_PRINT ) {
        struct ASTNode * right = exploreNodeTypEXP(root->children[0]);
        if (right->node_type == NODETYPE_EXP_LEFT) {
            struct SymbolTableEntry * entry = findLeftValueID(right->children[0]);
            if (entry == NULL) {
                add_to_error_array(root->line_number);
            }
        }
    }
}

struct SymbolTableEntry * find_symbol(struct SymbolTable* symbol_table, char * id, enum DataType type, bool is_static, bool is_assign) { 
    if (symbol_table == NULL) return NULL;
    for (int i = 0; i < symbol_table->num_entries; i++) {
        if (is_assign) {
            if (symbol_table->entries[i]->is_method == false && strcmp(symbol_table->entries[i]->id, id) == 0) {
                return symbol_table->entries[i];
            }
        } else {
            if (is_static) {
                if (symbol_table->entries[i]->is_method == false && symbol_table->entries[i]->is_static == true && strcmp(symbol_table->entries[i]->id, id) == 0) {
                    if (symbol_table->entries[i]->type == DATATYPE_UNDEFINED) {
                        symbol_table->entries[i]->type = type;
                    } else if (symbol_table->entries[i]->type != type) {
                        symbol_table->entries[i]->type = DATATYPE_UNDEFINED;
                    }
                    return symbol_table->entries[i];
                }
            } else {
                if (symbol_table->entries[i]->is_method == false && symbol_table->entries[i]->is_static == false && strcmp(symbol_table->entries[i]->id, id) == 0) {
                    if (symbol_table->entries[i]->type == DATATYPE_UNDEFINED) {
                        symbol_table->entries[i]->type = type;
                    } else if (symbol_table->entries[i]->type != type) {
                        symbol_table->entries[i]->type = DATATYPE_UNDEFINED;
                    }
                    return symbol_table->entries[i];
                }
            }
        }
    }
    find_symbol(symbol_table->parent, id, type ,is_static, is_assign);
}
    
void add_to_symbol_table(char* id, struct ASTNode * root, enum DataType type) {
    struct SymbolTableEntry* entry = (struct SymbolTableEntry*) malloc(sizeof(struct SymbolTableEntry));
    struct SymbolTable * table = root->symbol_table;
    bool is_method = root->node_type == NODETYPE_STATICMETHODDECL ? true : false;
    entry->dim_of_array = 0;
    if (root->node_type == NODETYPE_VARDECL && root->children[0]->node_type == NODETYPE_TYPE_ONE_ARR) {
        entry->dim_of_array = 1;
    } else if (root->node_type == NODETYPE_VARDECL && root->children[0]->node_type == NODETYPE_TYPE_TWO_ARR) {
        entry->dim_of_array = 2;
    } else if (root->node_type == NODETYPE_FORMALLIST && root->children[0]->node_type == NODETYPE_TYPE_ONE_ARR) {
        entry->dim_of_array = 1;
    } else if (root->node_type == NODETYPE_FORMALLIST && root->children[0]->node_type == NODETYPE_TYPE_TWO_ARR) {
        entry->dim_of_array = 2;
    } else if (root->node_type == NODETYPE_TYPEIDLIST && root->children[0]->node_type == NODETYPE_TYPE_ONE_ARR) {
        entry->dim_of_array = 1;
    } else if (root->node_type == NODETYPE_TYPEIDLIST && root->children[0]->node_type == NODETYPE_TYPE_TWO_ARR) {
        entry->dim_of_array = 2;
    }
    entry->id = id;
    entry->type = type;
    entry->is_method = is_method;
    if (root->is_static == true) {
        entry->is_static = true;
    }
    table->entries[table->num_entries] = entry;
    entry->entry_in_table = table->num_entries;
    table->num_entries++;
}

void print_tree_with_lines(struct ASTNode* root , int indent) {
  if (root != NULL) {
        printf("%*s%s\n", indent, "", enum_string(root->node_type).c_str());
        for (int i = 0; i < root->num_children; i++) {
            if (root->children[i] != NULL) {
                printf("%*s", indent, "");
                printf("|___");
                print_tree_with_lines(root->children[i], indent + 4);
            }
        }
    }
}


// void print_tree_with_lines(struct ASTNode* root , int indent) {
//     if (root != NULL) {
//         for (int i = 0; i < root->num_children; i++) {
//             print_tree_with_lines(root->children[i], indent + 4);
//         }
//         if (root->node_type == NODETYPE_EXP_TYPE) {
//             printf("%d    ", root->data.value.int_value);
//         }
//         printf("%s\n", enum_string(root->node_type).c_str());
//     }
// }

struct String* concatenateLists(struct String* head1, struct String* head2) {
    struct String* temp = head1;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = head2;
    return head1;
}

void add_to_error_array(int line_number) {
    error_array[num_errors] = line_number;
    num_errors++;
}

struct SymbolTableEntry * findParentScope(struct SymbolTable* symbol_table, char * id) {
    if (symbol_table == NULL) return NULL;
    for (int i = 0 ; i < symbol_table-> num_entries; i++) {
        if (strcmp(symbol_table->entries[i]->id, id) == 0) {
            return symbol_table->entries[i];
        }
    }
    findParentScope(symbol_table->parent, id);
}

struct SymbolTableEntry * findLeftValueID(struct ASTNode* root) {
    if (root == NULL) return NULL;
    if (root->node_type == NODETYPE_LEFTVALUE_ID || root->node_type == NODETYPE_LEFTVALUE_ONE_ARR || root->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
        for (int i = 0 ; i < root->symbol_table-> num_entries; i++) {
            if (strcmp(root->symbol_table->entries[i]->id, root->data.value.string_value) == 0) {
                return root->symbol_table->entries[i];
            }
        }
        struct SymbolTableEntry * parent = findParentScope(root->symbol_table->parent, root->data.value.string_value);
        if (parent == NULL) {
            for (int i = 0; i < num_entries; i++) {
                if (strcmp(method_symbol_table[i]->id, root->data.value.string_value) == 0) {
                    return method_symbol_table[i];
                }
            }   
        } else {
            return parent;
        }
    } else if (root->node_type == NODETYPE_METHODCALL || root->node_type == NODETYPE_CALL_MAIN) {
        for (int i = 0; i < num_entries; i++) {
            if (strcmp(method_symbol_table[i]->id, root->data.value.string_value) == 0) {
                return method_symbol_table[i];
            }
        }
    } else if (root->node_type == NODETYPE_VARDECL || root->node_type == NODETYPE_IDEXPLIST) {
        for (int i = 0 ; i < root->symbol_table-> num_entries; i++) {
            if (strcmp(root->symbol_table->entries[i]->id, root->data.value.string_value) == 0) {
                return root->symbol_table->entries[i];
            }
        }
    }
    return NULL;
}


bool checkExpPlus(struct ASTNode* root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(left) == DATATYPE_UNDEFINED)return true;
    }
    if (right->num_children > 0) {
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->is_method)return false;
        if (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(right) == DATATYPE_UNDEFINED)return true;
    }
    // check if left and right of + is int or string, and cannot be new array
    if ((!(FindExpType(left) == DATATYPE_INT && FindExpType(right) == DATATYPE_INT) 
        && !(FindExpType(left) == DATATYPE_STR && FindExpType(right) == DATATYPE_STR) 
        && !(FindExpType(left) == DATATYPE_UNDEFINED || FindExpType(right) == DATATYPE_UNDEFINED))|| 
        (left->node_type == NODETYPE_EXP_ONE_ARR || left->node_type == NODETYPE_EXP_TWO_ARR ||
        right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR)) { 
        return false;
    } else if (left->num_children > 0 ) {
        // check if array id is correct, ex: int[] a = new int[1]; a[1] = 1; can't be a[1][1] = 1;
        if ((left->node_type != NODETYPE_EXP_LENGTH && left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && findLeftValueID(left->children[0])->dim_of_array != 1)
            || (left->node_type != NODETYPE_EXP_LENGTH && left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && findLeftValueID(left->children[0])->dim_of_array != 2) 
            || (left->node_type != NODETYPE_EXP_LENGTH && left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->dim_of_array != 0)
            || (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0])->dim_of_array != 0)) {
                return false;
        }
    } else if (right->num_children > 0 ) {
        if ((right->node_type != NODETYPE_EXP_LENGTH && right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && findLeftValueID(right->children[0])->dim_of_array != 1)
            || (right->node_type != NODETYPE_EXP_LENGTH && right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && findLeftValueID(right->children[0])->dim_of_array != 2) 
            || (right->node_type != NODETYPE_EXP_LENGTH && right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->dim_of_array != 0)
            || (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0])->dim_of_array != 0)) {
                return false;
            }
    }
    return true;
}

bool checkOperationAndComp(struct ASTNode* root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(left) == DATATYPE_UNDEFINED)return true;

    }
    if (right->num_children > 0) {
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->is_method)return false;
        if (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(right) == DATATYPE_UNDEFINED)return true;
    }
    // other operation except +, check if left and right is int, and cannot be array
    if ((!(FindExpType(left) == DATATYPE_INT && FindExpType(right) == DATATYPE_INT 
        && (FindExpType(left) != DATATYPE_UNDEFINED | FindExpType(right) != DATATYPE_UNDEFINED)))|| 
        (left->node_type == NODETYPE_EXP_ONE_ARR || left->node_type == NODETYPE_EXP_TWO_ARR ||
        right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR)) {
            return false;
    } else if (left->num_children > 0 ) {
        // when come to here, left and right must be int, so check if array dim is correct
        if ((left->node_type != NODETYPE_EXP_LENGTH && left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && findLeftValueID(left->children[0])->dim_of_array != 1)
            || (left->node_type != NODETYPE_EXP_LENGTH && left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && findLeftValueID(left->children[0])->dim_of_array != 2) 
            || (left->node_type != NODETYPE_EXP_LENGTH && left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->dim_of_array != 0)
            || (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0])->dim_of_array != 0)) {
                return false;
        }
    } else if (right->num_children > 0 ) {
        if ((right->node_type != NODETYPE_EXP_LENGTH && right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && findLeftValueID(right->children[0])->dim_of_array != 1)
            || (right->node_type != NODETYPE_EXP_LENGTH && right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && findLeftValueID(right->children[0])->dim_of_array != 2) 
            || (right->node_type != NODETYPE_EXP_LENGTH && right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->dim_of_array != 0)
            || (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0])->dim_of_array != 0)) {
                return false;
        }
    }
    return true;
}

bool checkExpComp(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(left) == DATATYPE_UNDEFINED)return true;
    }
    if (right->num_children > 0) {
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->is_method)return false;
        if (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(right) == DATATYPE_UNDEFINED)return true;
    }
    // check if left and right is int or bool, but can be new array with same dim. LIterals are dealt here
    if ((!(FindExpType(left) == DATATYPE_INT && FindExpType(right) == DATATYPE_INT)
         &&!(FindExpType(left) == DATATYPE_BOOLEAN && FindExpType(right) == DATATYPE_BOOLEAN)
         && !(FindExpType(left) == DATATYPE_UNDEFINED || FindExpType(right) == DATATYPE_UNDEFINED))||
         (left->node_type == NODETYPE_EXP_ONE_ARR && right->node_type == NODETYPE_EXP_TWO_ARR)||
         (left->node_type == NODETYPE_EXP_TWO_ARR && right->node_type == NODETYPE_EXP_ONE_ARR)) {
            return false;
        // variable vs variable            
    } else if (left->num_children > 0 && right->num_children > 0) {
        // when come to here, left and right must be int or bool, so check if array dim is correct
        // 1. when both two side are variable, check if dim is same
        if ((left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && right->children[0]->node_type == NODETYPE_LEFTVALUE_ID)
            && findLeftValueID(left->children[0])->dim_of_array != findLeftValueID(right->children[0])->dim_of_array) {
                return false;
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && right->children[0]->node_type != NODETYPE_LEFTVALUE_ID
                   && left->node_type != NODETYPE_EXP_LENGTH && right->node_type != NODETYPE_EXP_LENGTH) {
            // when left is variable, right is array var, check if dim is same
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if ((right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && dim_of_left != 1) ||
                (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && dim_of_left != 2) ||
                (right->node_type == NODETYPE_EXP_ONE_ARR && dim_of_left != 1) ||
                (right->node_type == NODETYPE_EXP_TWO_ARR && dim_of_left != 2)) {
                    return false;
            } else if (right->node_type == NODETYPE_EXP_METHODCALL) {
                if (right->children[0]->node_type == NODETYPE_CALL_MAIN) {
                    return false;
                } else if (right->children[0]->node_type == NODETYPE_PARSEINT && dim_of_left != 0) {
                    return false;
                } else if (right->children[0]->node_type == NODETYPE_METHODCALL && dim_of_left != findLeftValueID(right->children[0])->dim_of_array) {
                    return false;
                }
            }
        } else if (left->children[0]->node_type != NODETYPE_LEFTVALUE_ID && right->children[0]->node_type == NODETYPE_LEFTVALUE_ID
                   && left->node_type != NODETYPE_EXP_LENGTH && right->node_type != NODETYPE_EXP_LENGTH) {
            // when right is variable, left is array var, check if dim is same
            int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
            if ((left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && dim_of_right != 1) ||
                (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && dim_of_right != 2) ||
                (left->node_type == NODETYPE_EXP_ONE_ARR && dim_of_right != 1) ||
                (left->node_type == NODETYPE_EXP_TWO_ARR && dim_of_right != 2)) {
                return false;
            } else if (left->node_type == NODETYPE_EXP_METHODCALL) {
                if (left->children[0]->node_type == NODETYPE_CALL_MAIN) {
                    return false;
                } else if (left->children[0]->node_type == NODETYPE_PARSEINT && dim_of_right != 0) {
                    return false;
                } else if (left->children[0]->node_type == NODETYPE_METHODCALL && dim_of_right != findLeftValueID(left->children[0])->dim_of_array) {
                    return false;
                }
            }
        } else if (left->children[0]->node_type != NODETYPE_LEFTVALUE_ID && right->children[0]->node_type != NODETYPE_LEFTVALUE_ID
                   && left->node_type != NODETYPE_EXP_LENGTH && right->node_type != NODETYPE_EXP_LENGTH) {
            // when both two side are array var, check if dim is same
            if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
                int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                if (dim_of_left != dim_of_right) {
                    return false;
                }
            } else if ((left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR)||
                       (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR)) {
                int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
                int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                if (dim_of_left == dim_of_right) {
                    return false;
                }
            } else if (left->node_type == NODETYPE_EXP_METHODCALL && right->node_type == NODETYPE_EXP_METHODCALL) {
                if (left->children[0]->node_type == NODETYPE_CALL_MAIN && right->children[0]->node_type == NODETYPE_CALL_MAIN) {
                    return false;
                } else if (left->children[0]->node_type == NODETYPE_METHODCALL && right->children[0]->node_type == NODETYPE_METHODCALL) {
                    int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
                    int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                    if (dim_of_left != dim_of_right) {
                        return false;
                    }
                }
            }
        }
        // variable vs literal
    } else if (left->num_children > 0 && right->num_children == 0 && left->node_type != NODETYPE_EXP_LENGTH) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 0) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 1) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 2) {
                return false;
            }
        } else if (left->node_type == NODETYPE_EXP_METHODCALL) {
            if (left->children[0]->node_type == NODETYPE_CALL_MAIN) {
                return false;
            } else if (left->children[0]->node_type == NODETYPE_METHODCALL) {
                int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
                if (dim_of_left != 0) {
                    return false;
                }
            }
        }
        // literal vs variable
    } else if (left->num_children == 0 && right->num_children > 0 && right->node_type != NODETYPE_EXP_LENGTH) {
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
            if (dim_of_right != 0) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
            if (dim_of_right != 1) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
            if (dim_of_right != 2) {
                return false;
            }
        } else if (right->node_type == NODETYPE_EXP_METHODCALL) {
            if (right->children[0]->node_type == NODETYPE_CALL_MAIN) {
                return false;
            } else if (right->children[0]->node_type == NODETYPE_METHODCALL) {
                int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                if (dim_of_right != 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool checkExpLogic(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(left) == DATATYPE_UNDEFINED)return true;
    }
    if (right->num_children > 0) {
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->is_method)return false;
        if (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(right) == DATATYPE_UNDEFINED)return true;
    }
    // check if left and right is boolean, and cannot be new array
    if ((!(FindExpType(left) == DATATYPE_BOOLEAN && FindExpType(right) == DATATYPE_BOOLEAN)
        && !(FindExpType(left) == DATATYPE_UNDEFINED || FindExpType(right) == DATATYPE_UNDEFINED)) ||
        (left->node_type == NODETYPE_EXP_ONE_ARR || left->node_type == NODETYPE_EXP_TWO_ARR ||
        right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR)) {
        return false;
    } else if (left->num_children > 0 ) {
        // when come to here, left and right must be int, so check if array dim is correct
        if ((left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && findLeftValueID(left->children[0])->dim_of_array != 1)
            || (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && findLeftValueID(left->children[0])->dim_of_array != 2) 
            || (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->dim_of_array != 0)
            || (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0])->dim_of_array != 0)) {
                return false;
        }
    } else if (right->num_children > 0 ) {
        if ((right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && findLeftValueID(right->children[0])->dim_of_array != 1)
            || (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && findLeftValueID(right->children[0])->dim_of_array != 2) 
            || (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->dim_of_array != 0)
            || (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0])->dim_of_array != 0)) {
                return false;
        }
    }
    return true;
}

bool checkExpLogicNot(struct ASTNode * root) {
    struct ASTNode *left = exploreNodeTypEXP(root->children[0]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(left) == DATATYPE_UNDEFINED)return true;
    }
    if (((FindExpType(left) != DATATYPE_BOOLEAN 
        && FindExpType(left) != DATATYPE_UNDEFINED))||
        (left->node_type == NODETYPE_EXP_ONE_ARR)||
        (left->node_type == NODETYPE_EXP_TWO_ARR)) {
        return false;
    } else if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 0) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 1) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 2) {
                return false;
            }
        } else if (left->node_type == NODETYPE_EXP_METHODCALL) {
            if (left->children[0]->node_type == NODETYPE_CALL_MAIN) {
                return false;
            } else if (left->children[0]->node_type == NODETYPE_METHODCALL) {
                int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
                if (dim_of_left != 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool checkExpPosNeg(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(left) == DATATYPE_UNDEFINED)return true;
        
    }
    if ((!(FindExpType(left) == DATATYPE_INT)
        && FindExpType(left) != DATATYPE_UNDEFINED)||
        (left->node_type == NODETYPE_EXP_ONE_ARR)||
        (left->node_type == NODETYPE_EXP_TWO_ARR)) {
            return false;
    } else if (left->num_children > 0 && left->node_type != NODETYPE_EXP_LENGTH) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 0) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 1) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 2) {
                return false;
            }
        } else if (left->node_type == NODETYPE_EXP_METHODCALL) {
            if (left->children[0]->node_type == NODETYPE_CALL_MAIN) {
                return false;
            } else if (left->children[0]->node_type == NODETYPE_METHODCALL) {
                struct SymbolTableEntry * entry = findLeftValueID(left->children[0]);
                int dim_of_left = entry->dim_of_array;
                if (dim_of_left != 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool checkIfWhile(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
    if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0]) == NULL)return false;
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(left->children[0])->is_method)return false;
        if (left->node_type == NODETYPE_EXP_METHODCALL && left->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(left->children[0]) == NULL)return false;
    }
    if ((FindExpType(left) != DATATYPE_BOOLEAN &&
          FindExpType(left) != DATATYPE_UNDEFINED) ||
        (left->node_type == NODETYPE_EXP_ONE_ARR)||
        (left->node_type == NODETYPE_EXP_TWO_ARR)) {
            return false;
    } else if (left->num_children > 0) {
        if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 0) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 1) {
                return false;
            }
        } else if (left->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
            if (dim_of_left != 2) {
                return false;
            }
        } else if (left->node_type == NODETYPE_EXP_METHODCALL) {
            if (left->children[0]->node_type == NODETYPE_CALL_MAIN) {
                return false;
            } else if (left->children[0]->node_type == NODETYPE_METHODCALL) {
                int dim_of_left = findLeftValueID(left->children[0])->dim_of_array;
                if (dim_of_left != 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool checkVarDecl(struct ASTNode * root) {
    if (root->num_children > 1) {
        struct ASTNode * right = exploreNodeTypEXP(root->children[1]->children[0]);
        if (right->num_children > 0) {
            if (right->node_type == NODETYPE_EXP_LEFT && findLeftValueID(right->children[0]) == NULL)return false;
            if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->is_method)return false;
            if (right->node_type == NODETYPE_EXP_METHODCALL && right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)return false;
            if (right->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(right) == DATATYPE_UNDEFINED) {
                return true;
            }
        }
        if (right->num_children > 0) {
            if ((right->node_type == NODETYPE_EXP_LEFT && findLeftValueID(right->children[0]) == NULL) ||
            (right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)) {
                struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
                return false;
            }
        }
        if (root->children[0]->node_type == NODETYPE_TYPE_ONE_ARR) {
            if (root->children[root->num_children-1]->node_type == NODETYPE_IDEXPLIST) {
                enum DataType type = FindType(root->children[0]);
                checkIDExpList(root->children[root->num_children-2], type , 1);
                checkIDExpList(root->children[root->num_children-1], type , 1);
            } else {
                // check if the dim of method call is 1 and match the type
                if (right->node_type == NODETYPE_EXP_LENGTH)return false;
                if (right->node_type == NODETYPE_EXP_METHODCALL) {
                    if (right->children[0]->node_type == NODETYPE_CALL_MAIN ||
                        right->children[0]->node_type == NODETYPE_PARSEINT ||
                        !(FindType(root->children[0]) == FindExpType(right)) ||
                        findLeftValueID(right->children[0])->dim_of_array != 1) {
                            return false;
                    } 
                    // check if new type [] is dim 1 and match the type 
                } else if ((!(FindType(root->children[0]) == FindExpType(right)))
                            && (FindExpType(right) != DATATYPE_UNDEFINED) ||
                            (right->node_type == NODETYPE_EXP_TWO_ARR || right->node_type == NODETYPE_EXP_TYPE)) {
                            return false;
                } else if (right->num_children > 0) {
                    // check if the dim of left value is 1 and match the type
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                        if (dim_of_right != 1) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                        int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                        if (dim_of_right != 2) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        return false;
                    }
                }
            }
        } else if (root->children[0]->node_type == NODETYPE_TYPE_TWO_ARR) {
            if (root->children[root->num_children-1]->node_type == NODETYPE_IDEXPLIST) {
                enum DataType type = FindType(root->children[0]);
                checkIDExpList(root->children[root->num_children-2], type , 2);
                checkIDExpList(root->children[root->num_children-1], type , 2);
            } else {
                // check if the dim of method call is 2 and match the type
                if (right->node_type == NODETYPE_EXP_LENGTH)return false;
                if (right->node_type == NODETYPE_EXP_METHODCALL) {
                    if (right->children[0]->node_type == NODETYPE_CALL_MAIN || 
                        right->children[0]->node_type == NODETYPE_PARSEINT ||
                        !(FindType(root->children[0]) == FindExpType(right)) ||
                        findLeftValueID(right->children[0])->dim_of_array != 2) {
                                return false;
                    }
                     // check if new type [] is dim 1 and match the type 
                } else if (((FindType(root->children[0]) != FindExpType(right)) && (FindExpType(right) != DATATYPE_UNDEFINED))||
                            (right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TYPE)) {
                            return false;
                } else if (right->num_children > 0) {
                    // check if the dim of left value is 2 and match the type
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                        if (dim_of_right != 2) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR ||
                                right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        return false;
                    }
                }
            }
        } else {
            if (root->children[root->num_children-1]->node_type == NODETYPE_IDEXPLIST) {
                enum DataType type = FindType(root->children[0]);
                checkIDExpList(root->children[root->num_children-2], type , 0);
                checkIDExpList(root->children[root->num_children-1], type , 0);
            } else {
                if (right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_METHODCALL) {
                    if (right->children[0]->node_type == NODETYPE_CALL_MAIN || (FindType(root->children[0]) != FindExpType(right) 
                    && (FindExpType(right) != DATATYPE_UNDEFINED))) {
                        return false;
                    } else if (right->children[0]->node_type != NODETYPE_PARSEINT && findLeftValueID(right->children[0])->dim_of_array != 0) {
                        return false;
                    }
                } else {
                    if ((FindType(root->children[0]) != FindExpType(right)
                        && (FindExpType(right) != DATATYPE_UNDEFINED))) {
                            return false;
                    } else if (right->num_children > 0) {
                        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && right->node_type != NODETYPE_EXP_LENGTH) {
                            int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                            if (dim_of_right != 0 ) {
                                return false;
                            }
                        } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && right->node_type != NODETYPE_EXP_LENGTH) {
                            int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                            if (dim_of_right != 1) {
                                return false;
                            }
                        } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && right->node_type != NODETYPE_EXP_LENGTH) {
                             int dim_of_right = findLeftValueID(right->children[0])->dim_of_array;
                             if (dim_of_right != 2) {
                                return false;
                             }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool checkAssign(struct ASTNode * root) {
    // still need to deal with method
    struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
    struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (right->num_children > 0) {
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && findLeftValueID(right->children[0])->is_method)return false;
        if (right->node_type == NODETYPE_EXP_METHODCALL && right->children[0]->node_type == NODETYPE_METHODCALL && findLeftValueID(right->children[0]) == NULL)return false;
        if (right->children[0]->node_type != NODETYPE_CALL_MAIN && FindExpType(right) == DATATYPE_UNDEFINED) {
                return true;
        }
    }
    if (entry == NULL) return false;
    if (right->node_type == NODETYPE_EXP_LEFT) {
        // scope check for assign
        struct SymbolTableEntry * var = findLeftValueID(right->children[0]);
        if (var == NULL) return false;
    } else if (right->node_type == NODETYPE_EXP_METHODCALL && right->children[0]->node_type != NODETYPE_PARSEINT) {
        struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
        if (method == NULL) return false;
    }
    if (entry != NULL) {
        enum DataType type = FindExpType(right);
        if ((entry->type != type && type != DATATYPE_UNDEFINED)) return false;
        if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            if (entry->dim_of_array == 0) {
                // Deal with main
                if (right->node_type == NODETYPE_EXP_METHODCALL && right->children[0]->node_type != NODETYPE_PARSEINT) {
                    struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
                    if (right->children[0]->node_type == NODETYPE_CALL_MAIN || method->dim_of_array != 0) {
                        return false;
                    } 
                } else if (right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_LEFT) {
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && right->node_type != NODETYPE_EXP_LENGTH) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 0 ) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && right->node_type != NODETYPE_EXP_LENGTH) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 1) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && right->node_type != NODETYPE_EXP_LENGTH) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 2 ) {
                            return false;
                        }
                    }
                }
            } else if (entry->dim_of_array == 1) {
                if (right->node_type == NODETYPE_EXP_LENGTH)return false;
                if (right->node_type == NODETYPE_EXP_METHODCALL) {
                    if (right->children[0]->node_type == NODETYPE_PARSEINT)return false;
                    struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
                    if (right->children[0]->node_type == NODETYPE_CALL_MAIN || method->dim_of_array != 1) {
                        return false;
                    }
                } else if (right->node_type == NODETYPE_EXP_TYPE || right->node_type == NODETYPE_EXP_TWO_ARR) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_LEFT) {
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 1 ) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 2) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        return false;
                    }
                }
            } else if (entry->dim_of_array == 2) {
                if (right->node_type == NODETYPE_EXP_LENGTH)return false;
                if (right->node_type == NODETYPE_EXP_METHODCALL) {
                    if (right->children[0]->node_type == NODETYPE_PARSEINT)return false;
                    struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
                    if ( right->children[0]->node_type == NODETYPE_CALL_MAIN || method->dim_of_array != 2) {
                        return false;
                    }
                } else if (right->node_type == NODETYPE_EXP_TYPE || right->node_type == NODETYPE_EXP_ONE_ARR) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_LEFT) {
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 2 ) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR ||
                               right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        return false;
                    }
                }
            }
        } else if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            if (entry->dim_of_array == 1) {
                if (right->node_type == NODETYPE_EXP_METHODCALL && right->children[0]->node_type != NODETYPE_PARSEINT) {
                    struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
                    if (right->children[0]->node_type == NODETYPE_CALL_MAIN || method->dim_of_array != 0) {
                        return false;
                    }
                } else if (right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR ) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_LEFT) {
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 0 ) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 1) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 2) {
                            return false;
                        }
                    }
                }

            } else if (entry->dim_of_array == 2) {
                if (right->node_type == NODETYPE_EXP_LENGTH)return false;
                if (right->node_type == NODETYPE_EXP_METHODCALL) {
                    if (right->children[0]->node_type == NODETYPE_PARSEINT)return false;
                    struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
                    if ( right->children[0]->node_type == NODETYPE_CALL_MAIN || method->dim_of_array != 1) {
                        return false;
                    }
                } else if (right->node_type == NODETYPE_EXP_TYPE || right->node_type == NODETYPE_EXP_TWO_ARR ) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_LEFT) {
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 1 ) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 2) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        return false;
                    }
                }
            }
        } else if (root->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            if (entry->dim_of_array == 2) {
                if (right->node_type == NODETYPE_EXP_METHODCALL && right->children[0]->node_type != NODETYPE_PARSEINT) {
                    struct SymbolTableEntry * method = findLeftValueID(right->children[0]);
                    if ( right->children[0]->node_type == NODETYPE_CALL_MAIN || method->dim_of_array != 0) {
                        return false;
                    }
                } else if (right->node_type == NODETYPE_EXP_ONE_ARR || right->node_type == NODETYPE_EXP_TWO_ARR) {
                    return false;
                } else if (right->node_type == NODETYPE_EXP_LEFT) {
                    if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 0 ) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 1) {
                            return false;
                        }
                    } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                        struct SymbolTableEntry * left = findLeftValueID(right->children[0]);
                        if (left == NULL || left->dim_of_array != 2) {
                            return false;
                        }
                    }
                }
            }
        }
    }  
    return true;
}

bool checkExpLength(struct ASTNode * root) {
    struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
    if (entry == NULL) return false;
    if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
        struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
        if (entry == NULL || entry->is_method) return false;
        if (entry->dim_of_array == 0) {
            return false;
        }
    } else if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
        if (entry->dim_of_array != 2) {
            return false;
        }
    } else if (root->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
        return false;
    }
    return true;
}

bool checkParseInt(struct ASTNode * root) {
    struct ASTNode * right = exploreNodeTypEXP(root->children[0]);
    if ((FindExpType(right) != DATATYPE_STR && FindExpType(right) != DATATYPE_UNDEFINED) ||
        (root->children[0]->node_type == NODETYPE_EXP_ONE_ARR) ||
        (root->children[0]->node_type == NODETYPE_EXP_TWO_ARR)) {
        return false;
    } else if (right->node_type == NODETYPE_EXP_LEFT) {
        struct SymbolTableEntry * entry = findLeftValueID(right->children[0]);
        if (entry == NULL) return false;
        if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ID && entry->dim_of_array != 0) {
            return false;
        } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_ONE_ARR && entry->dim_of_array != 1) {
            return false;
        } else if (right->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR && entry->dim_of_array != 2) {
            return false;
        }
    } else if (right->node_type == NODETYPE_EXP_METHODCALL) {
        struct SymbolTableEntry * entry = findLeftValueID(right->children[0]);
        if (entry == NULL) return false;
        if (right->children[0]->node_type == NODETYPE_CALL_MAIN || entry->dim_of_array != 0) {
            return false;
        }
    }
    return true;
}

int exploreExpCommaList(struct ASTNode * root) {
    if (root == NULL) return 0;
    if (root->node_type == NODETYPE_EXPLIST) {
        return 1 + exploreExpCommaList(root->children[root->num_children-1]);
    } else if (root->node_type == NODETYPE_COMMAEXPLIST) {
        return 1 + exploreExpCommaList(root->children[root->num_children-1]);
    } else {
        return 0;
    }
}

struct TypeList * exploreExpCommaListType(struct ASTNode * root) {
    if (root->node_type == NODETYPE_EXPLIST) {
        struct TypeList * list = (struct TypeList *)malloc(sizeof(struct TypeList));
        struct ASTNode * exp = exploreNodeTypEXP(root->children[0]);
        if (exp->node_type == NODETYPE_EXP_LEFT || exp->node_type == NODETYPE_EXP_METHODCALL) {
            struct SymbolTableEntry * entry = findLeftValueID(exp->children[0]);
            list->type = entry->type;
            list->node = exp->children[0];
            list->info = entry;
        } else {
            list->type = FindExpType(exp);
            list->node = exp;
            list->info = NULL;
        }
        list->next = exploreExpCommaListType(root->children[root->num_children-1]);
        return list;
    } else if (root->node_type == NODETYPE_COMMAEXPLIST) {
        struct TypeList * list = (struct TypeList *)malloc(sizeof(struct TypeList));
        struct ASTNode * exp = exploreNodeTypEXP(root->children[0]);
        if (exp->node_type == NODETYPE_EXP_LEFT || exp->node_type == NODETYPE_EXP_METHODCALL) {
            struct SymbolTableEntry * entry = findLeftValueID(exp->children[0]);
            list->type = entry->type;
            list->node = exp->children[0];
            list->info = entry;
        } else {
            list->type = FindExpType(exp);
            list->node = exp;
            list->info = NULL;
        }
        list->next = exploreExpCommaListType(root->children[root->num_children-1]);
        return list;
    } else {
        return NULL;
    }
}

bool checkExpMethodCall(struct ASTNode * root) {
    if (root->children[0]->node_type == NODETYPE_CALL_MAIN) {
        int num_of_exp = 0;
        if (root->children[0]->num_children > 0) {
            num_of_exp = exploreExpCommaList(root->children[0]->children[0]);
        }
        if (num_of_exp != 1) {
            return false;
        } else if (FindExpType(root->children[0]->children[0]->children[0]) != DATATYPE_STR ||
                  root->children[0]->children[0]->children[0]->node_type == NODETYPE_EXP_TWO_ARR) {
            return false;
        }  
        // still need to deal with leftvalue and methodcall
    } else if (root->children[0]->node_type == NODETYPE_METHODCALL) {
        struct SymbolTableEntry * entry = NULL;
        if (root->num_children > 0) {
            entry = findLeftValueID(root->children[0]);
        }
        if (entry != NULL) {
            int num_of_param = entry->number_of_parameters;
            struct SymbolTable * scope = entry->correspond_scope;
            int num_of_exp = exploreExpCommaList(root->children[0]->children[0]);
            if (num_of_param != num_of_exp) {
                return false;
            } else {
                struct TypeList * list = NULL;
                if (root->children[0]->num_children > 0) {
                    list = exploreExpCommaListType(root->children[0]->children[0]);
                }
                if (list != NULL) {
                    for (int i = 0; i < num_of_param; i++) {
                        if (list->type != scope->entries[i]->type && list->type != DATATYPE_UNDEFINED) {
                            return false;
                        } else if (list->info != NULL) {
                            if (list->node->node_type == NODETYPE_LEFTVALUE_ID || list->node->node_type == NODETYPE_EXP_METHODCALL) {
                                if (list->info->dim_of_array != scope->entries[i]->dim_of_array) return false;
                            } else if (list->node->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
                                if (scope->entries[i]->dim_of_array == 0 && list->info->dim_of_array != 1) {
                                    return false;
                                } else if (scope->entries[i]->dim_of_array == 1 && list->info->dim_of_array != 2) {
                                    return false;
                                } else if (scope->entries[i]->dim_of_array == 2) {
                                    return false;
                                }
                            } else if (list->node->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
                                if (scope->entries[i]->dim_of_array != 0 ) {
                                    return false;
                                }
                            }
                        }
                        list = list->next;
                    }
                }
            }
        }
    } else if (root->children[0]->node_type == NODETYPE_PARSEINT) {
        return checkParseInt(root->children[0]);
    }
    return true;
}

bool checkLeftValueArr(struct ASTNode * root) {
    if (root->node_type == NODETYPE_LEFTVALUE_ONE_ARR) { // forget to check if it is int index 
        // if call array id, its dim must be valid
        struct SymbolTableEntry * entry = findLeftValueID(root);
        if (entry != NULL && entry->dim_of_array == 0) {
            return false;
        } else {
            struct ASTNode * left = exploreNodeTypEXP(root->children[0]);
            if (FindExpType(left) != DATATYPE_INT) {
                return false;
            }
        }
    } else if (root->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
        struct SymbolTableEntry * entry = findLeftValueID(root);
        if (entry != NULL && entry->dim_of_array < 2) {
            return false;
        } else {
            struct ASTNode * right1 = exploreNodeTypEXP(root->children[0]);
            struct ASTNode * right2 = exploreNodeTypEXP(root->children[1]);
            if (FindExpType(right1) != DATATYPE_INT || FindExpType(right2) != DATATYPE_INT) {
                return false;
            }
        }
    }
    return true;
}
