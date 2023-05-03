#ifndef CODEGEN_HH
#define CODEGEN_HH

#include "node.hh"
#include <string>
#include <vector>

void traverse_and_codegen(struct ASTNode * root, bool is_method);
void generateCode(char * filename);

void create_instruction_for_exp_type(struct ASTNode * root);
void create_instruction_for_exp_leftID(struct ASTNode * root);
void create_instruction_for_exp_plus(struct ASTNode * root);
void create_instruction_for_exp_operation(struct ASTNode * root);
void create_instruction_for_varDecl(struct ASTNode * root);
void create_instruction_for_assign(struct ASTNode * root);
void create_instruction_for_print_println(struct ASTNode * root);
void create_instruction_for_idExpList(struct ASTNode * root, enum DataType type, int dim_of_array);
void create_instruction_for_method_decl(struct ASTNode * root);
void create_instruction_for_method_call(struct ASTNode * root);
void create_instruction_for_return(struct ASTNode * root);
void create_instruction_for_exp_neg_pos(struct ASTNode * root);
void create_instruction_for_if(struct ASTNode * root);
void create_instruction_for_while(struct ASTNode * root);
void create_instruction_for_exp_operation_comp(struct ASTNode * root);
void create_instruction_for_exp_logic(struct ASTNode * root);
void create_instruction_for_exp_comp(struct ASTNode * root);
void create_instruction_for_exp_one_arr(struct ASTNode * root);
void create_instruction_for_exp_two_arr(struct ASTNode * root);
void create_instruction_for_allocate_second_dimension(struct ASTNode * root);
void create_instruction_for_exp_length(struct ASTNode * root);

void convert_mov(int i, struct method_scope * method_scope);
void convert_ldr(int i, struct method_scope * method_scope);
void convert_exp_operation_plus(int i, struct method_scope * method_scope);
void convert_exp_operation_comp(int i, struct method_scope * method_scope);
void convert_exp_logic(int i, struct method_scope * method_scope);
void convert_neg_not(int i, struct method_scope * method_scope);
void convert_ldr_from_mem(int i, struct method_scope * method_scope);
void convert_if_while_cmp(int i, struct method_scope * method_scope);
void convert_three_operand(int i, struct method_scope * method_scope );
void convert_two_operand(int i, struct method_scope * method_scope);
void convert_one_operand(int i, struct method_scope * method_scope);
void convert_compare_two_reg(int i , struct method_scope * method_scope);
void convert_ldr_var(int i, struct method_scope * method_scope);
void convert_str_assign_vardecl(int i, struct method_scope * method_scope);


std::vector<std::string> split(const std::string& str, const std::string& delim);
int find_instruction_index(struct ASTNode * root);
int find_instruction_index_by_id(int id);
void drag_instruction(struct ASTNode * root, int destination);
struct ASTNode * find_method(struct ASTNode * root);
void store_array_length(struct ASTNode * root, int symbolic_register);
bool find_end_if(std::string instr);
int find_instruction_by_string(std::string instr);
struct ASTNode * find_first_node_not_statementList(struct ASTNode * root);
#endif