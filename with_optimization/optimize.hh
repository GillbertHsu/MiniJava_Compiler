#ifndef OPTIMIZE_HH
#define OPTIMIZE_HH

#include "codegen.hh"
#include <vector>
#include <string>
#include <set>

void optimize();
void calculate_use_def();
void calculate_use_def_two_operand(int i);
void calculate_use_def_of_exp_op_exp(int i);
void calculate_use_def_one_operand(int i);
void calculate_use_def_str_vardecl(int i);
void calculate_use_def_ldr_literal_mem(int i);
void calculate_use_def_if_while_cmp(int i);
void calculate_use_def_method_call(int i);

void liveness_analysis();

void build_graph();
void create_node(struct method_scope * method_scope, int index);
void link_node_in_same_block(struct method_scope * method_scope, int index, std::string LVin_or_LVout);
void color_graph();


void debug();
void update_id_index_table();
void all_method_index_behind_plus_plus(int index);

#endif
