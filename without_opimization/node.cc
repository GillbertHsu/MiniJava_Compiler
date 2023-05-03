#include "node.hh"
#include <cstdlib>
#include <cstring>

struct ASTNode* new_node(enum NodeType t, int line_number){
    struct ASTNode* ast_node = (struct ASTNode*) malloc(sizeof(struct ASTNode));
    memset(ast_node, 0, sizeof(struct ASTNode));
    ast_node->node_type = t;
    ast_node->line_number = line_number;
    ast_node->has_curly_braces = false;
    ast_node->is_static = false;
    ast_node->child_index = 0;
    return ast_node;
}

int add_child(struct ASTNode* parent, struct ASTNode* child){
    if (parent->num_children < MAX_NUM_CHILDREN) {
        parent -> children[parent->num_children] = child;
        child->child_index = parent->num_children;
        parent -> num_children++;
        child->parent = parent;
        return 0;
    } else {
        return -1;
    }
}

void set_string_value(struct ASTNode* node, char* s){
    node->data.type = DATATYPE_STR;
    node->data.value.string_value = s;
}

void set_int_value(struct ASTNode* node, int i){
    node->data.type = DATATYPE_INT;
    node->data.value.int_value = i;
}

void set_boolean_value(struct ASTNode* node, bool b){
    node->data.type = DATATYPE_BOOLEAN;
    node->data.value.boolean_value = b;
}




