#include "node.hh"
#include "codegen.hh"
#include "optimize.hh"
#include "typecheck.hh"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <set>
#include <stack>
#include <queue>
#include <tuple>

extern std::vector<struct instruction> instructions;
extern std::vector<int> method_index;
std::unordered_map<int, int> instr_id_to_index_block_id;

// Note: How do I change mov left value id, since every time we use it we add the counter. They are actually the same var. 
// Note: How to repeat the process of finding LvIn and LvOut for block with more than one predecessor.

void optimize() {
    calculate_use_def();
    liveness_analysis();
    //debug();
    build_graph();
    color_graph();
    //printf("Optimization done\n");
}

void calculate_use_def() {
    int method_counter = 0;
    for (int i = 0 ; i < method_index.size()-1; i++) {
        int start = method_index[i]+1;
        instructions[start-1].block_id = start-1;
        instr_id_to_index_block_id[instructions[start-1].instruction_id] = start-1;
        for (int j = start; j < method_index[i+1]; j++) {
            if (instructions[j].root != NULL)instructions[j].root->method_scope = instructions[start-1].root->method_scope;
            instructions[j].block_id = j;
            instr_id_to_index_block_id[instructions[j].instruction_id] = j;
            if (instructions[j].instrType != BRANCH && instructions[j].is_return == false && j < method_index[i+1]-1) {
                instructions[j].successor.push_back(instructions[j+1].instruction_id);
                instructions[j+1].predecessor.push_back(instructions[j].instruction_id);
            }
            if (instructions[j].instrType == MOV_LEFTVAL) {  
                calculate_use_def_two_operand(j);
            } else if (instructions[j].instrType == ADD_MINUS_MUL) {
                calculate_use_def_of_exp_op_exp(j);
            } else if (instructions[j].instrType == LDR_LITERAL) {
                calculate_use_def_ldr_literal_mem(j);
            } else if (instructions[j].instrType == MOV_CMP) {
                calculate_use_def_one_operand(j);
            } else if (instructions[j].instrType == AND_OR) {
                calculate_use_def_of_exp_op_exp(j);
            } else if (instructions[j].instrType == NEG || instructions[j].instrType == LOGIC_NOT || instructions[j].instrType == POS) {
                calculate_use_def_two_operand(j);
            } else if (instructions[j].instrType == LDR_FROM_MEM) {
                calculate_use_def_ldr_literal_mem(j);
            } else if (instructions[j].instrType == IF_WHILE_CMP) {
                calculate_use_def_if_while_cmp(j);
            } else if (instructions[j].instrType == ONE_OPERAND) {
                calculate_use_def_one_operand(j);
            } else if (instructions[j].instrType == TWO_OPERAND) {
                calculate_use_def_two_operand(j);
            } else if (instructions[j].instrType == THREE_OPERAND) {
                calculate_use_def_of_exp_op_exp(j);
            } else if (instructions[j].instrType == METHOD_CALL || instructions[j].instrType == PRINTLN) {
                calculate_use_def_method_call(j);
            } else if (instructions[j].instrType == LDR_INT_FORMAT) {
                calculate_use_def_one_operand(j);
            }
        }
    }
}

void calculate_use_def_of_exp_op_exp(int i) {
    std::vector<std::string> tokens = split(instructions[i].instr, " ");
    std::string operand1 = tokens[3];
    std::string operand2 = tokens[5];
    std::string result = tokens[1];

    instructions[i].def.insert(result);
    instructions[i].use.insert(operand1);
    instructions[i].use.insert(operand2);
}

void calculate_use_def_two_operand(int i) {
    std::vector<std::string> tokens = split(instructions[i].instr, " ");
    std::string operand1 = tokens[3];
    std::string result = tokens[1];

    instructions[i].def.insert(result);
    instructions[i].use.insert(operand1);
}

void calculate_use_def_one_operand(int i) {
    std::vector<std::string> tokens = split(instructions[i].instr, " ");
    std::string operand1 = tokens[1];
    instructions[i].def.insert(operand1);
}

void calculate_use_def_if_while_cmp(int i) {
    std::vector<std::string> tokens = split(instructions[i].instr, " ");
    std::string operand1 = tokens[1];
    instructions[i].use.insert(operand1);
}

void calculate_use_def_ldr_literal_mem(int i) {
    std::vector<std::string> tokens = split(instructions[i].instr, " ");
    std::string result = tokens[1];
    instructions[i].def.insert(result);
}


void calculate_use_def_method_call(int i) {
    if (instructions[i].instrType == PRINTLN) {
        instructions[i].use.insert("r0");
        return;
    }
    std::vector<std::string> tokens = split(instructions[i].instr, " ");
    if (tokens[1] == "printf") {
        instructions[i].use.insert("r0");
        if (FindExpType(instructions[i].root->children[0]) != DATATYPE_STR)instructions[i].use.insert("r1");
    } else if (tokens[1] == "atoi" ) {
        instructions[i].use.insert("r0");
    } else {
        struct SymbolTableEntry * entry = find_method(instructions[i].root->children[0]->data.value.string_value);
        for (int j = 0; j < entry->number_of_parameters; j++) {
            instructions[i].use.insert("r" + std::to_string(j));
        }
    }
}

// use std::set_difference and std::set_union to calculate use and def
void liveness_analysis() {
    // TODO: change to use predecessor with largest index to calculate LVin
    // calculate backward
    for (int i = method_index.size()-1; i > 0; i--) {
        int start = method_index[i]-1;
        int end = method_index[i-1]+1;
        bool non_stop = true;
        std::stack<int> stack_of_instruction; 
        stack_of_instruction.push(start);
        while (non_stop) {
            int j = stack_of_instruction.top();
            stack_of_instruction.pop();
            if (j == start) {
                instructions[j].LVout.insert(instructions[j].def.begin(), instructions[j].def.end());
            }
            // calculate LVout
            for (int k = 0; k < instructions[j].successor.size(); k++) {
                int index = instr_id_to_index_block_id[instructions[j].successor[k]];
                std::set<std::string> tempIn;
                std::set_union(instructions[j].LVout.begin(), instructions[j].LVout.end(), instructions[index].LVin.begin(), instructions[index].LVin.end(), std::inserter(tempIn, tempIn.begin()));
                instructions[j].LVout.insert(tempIn.begin(), tempIn.end());
            }
            // calculate LVin
            std::set<std::string> tempOut; 
            std::set_difference(instructions[j].LVout.begin(), instructions[j].LVout.end(), instructions[j].def.begin(), instructions[j].def.end(), std::inserter(tempOut, tempOut.begin()));
            
            std::set<std::string> tempIn;
            std::set_union(instructions[j].use.begin(), instructions[j].use.end(), tempOut.begin(), tempOut.end(), std::inserter(tempIn, tempIn.begin()));

            if (j == end) {
                non_stop = false;
            } else if (instructions[j].predecessor.size() == 2 && std::max(instr_id_to_index_block_id[instructions[j].predecessor[0]], instr_id_to_index_block_id[instructions[j].predecessor[1]]) > j) {
                int max = std::max(instr_id_to_index_block_id[instructions[j].predecessor[0]], instr_id_to_index_block_id[instructions[j].predecessor[1]]);
                stack_of_instruction.push(j-1);
                stack_of_instruction.push(max);
            } else if (instructions[j].LVin == tempIn && tempIn.size() != 0) {
                continue;
            } else if (instructions[j].instr.find(":") != std::string::npos && instructions[j].instr.find("end_while") != std::string::npos 
                        && instructions[j].LVin != tempIn && tempIn.size() != 0 && instructions[j].predecessor.size() == 1
                        && instr_id_to_index_block_id[instructions[j].predecessor[0]] != j-1 ) {
                stack_of_instruction.push(instr_id_to_index_block_id[instructions[j].predecessor[0]]);
                stack_of_instruction.push(j-1);
            } else {
                stack_of_instruction.push(j-1);
            }
            instructions[j].LVin.insert(tempIn.begin(), tempIn.end());
        }
    }
}

void build_graph() {
    for (int i = 0 ; i < method_index.size()-1; i++) {
        int start = method_index[i]+1;
        struct method_scope * method_scope = instructions[start-1].root->method_scope;
        for (int j = start; j < method_index[i+1]; j++) {
            create_node(method_scope, j);
        }
    }
}


void create_node(struct method_scope * method_scope, int index) {
    for (auto element : instructions[index].LVin) {
        std::string name = element;
        if (method_scope->node_table.find(name) == method_scope->node_table.end()) {
            struct Node * node = new Node();
            node->name = name;
            if (name.find("$") != std::string::npos) {
                int offset = std::stoi(name.substr(2))*4;
                node->offset = std::to_string(offset);
            } else if (name.find("r") != std::string::npos) {
                node->assigned_register = std::stoi(name.substr(1));
                if (method_scope->register_in_use.find(node->assigned_register) == method_scope->register_in_use.end()) {
                    method_scope->register_in_use.insert(node->assigned_register);
                    method_scope->register_remain.erase(node->assigned_register);
                }
            }
            method_scope->node_table[name] = node;
        }
    }
    link_node_in_same_block(method_scope, index, "LVin");

    for (auto element : instructions[index].LVout) {
        std::string name = element;
        if (method_scope->node_table.find(name) == method_scope->node_table.end()) {
            struct Node * node = new Node();
            node->name = name;
            if (name.find("$") != std::string::npos) {
                int offset = std::stoi(name.substr(2))*4;
                node->offset = std::to_string(offset);
            } else if (name.find("r") != std::string::npos) {
                node->assigned_register = std::stoi(name.substr(1));
                if (method_scope->register_in_use.find(node->assigned_register) == method_scope->register_in_use.end()) {
                    method_scope->register_in_use.insert(node->assigned_register);
                    method_scope->register_remain.erase(node->assigned_register);
                }
            }
            method_scope->node_table[name] = node;
        }
    }
    link_node_in_same_block(method_scope, index, "LVout");
}

void link_node_in_same_block(struct method_scope * method_scope, int index, std::string LVin_or_LVout) {
    if (LVin_or_LVout == "LVin") {
        for (auto element : instructions[index].LVin) {
            struct Node * node = method_scope->node_table[element];
            std::set<Node *> adj_list;
            for (auto element_to_link : instructions[index].LVin) {
                if (element_to_link != element) {
                    adj_list.insert(method_scope->node_table[element_to_link]);
                }
            }
            std::set_union(node->neighbors.begin(), node->neighbors.end(), adj_list.begin(), adj_list.end(), std::inserter(node->neighbors, node->neighbors.begin()));
            node->degree = node->neighbors.size();
        }
    } else {
        //Let the LVout of the call instruction to be interfere with caller save registers
        for (auto element : instructions[index].LVout) {
            struct Node * node = method_scope->node_table[element];
            std::set<Node *> adj_list;
            for (auto element_to_link : instructions[index].LVout) {
                if (element_to_link != element) {
                    adj_list.insert(method_scope->node_table[element_to_link]);
                }
            }
            if (instructions[index].instrType == METHOD_CALL || instructions[index].instrType == PRINTLN) {
                node->is_LVout_of_method = true;
            }
            std::set_union(node->neighbors.begin(), node->neighbors.end(), adj_list.begin(), adj_list.end(), std::inserter(node->neighbors, node->neighbors.begin()));
            node->degree = node->neighbors.size();
        }
    }
}

struct NodeComparator {
    bool operator() (Node* n1, Node* n2) {
        if (n1->degree == n2->degree) {
            return n1->assigned_register < n2->assigned_register;
        }
        return n1->degree > n2->degree;
    }
};

void decrease_degree_of_neighbors(struct Node * node) {
    for (auto element : node->neighbors) {
        element->degree--;
    }
}


void color_graph() {
    for (int i = 0 ; i < method_index.size()-1; i++) {
        std::vector<instruction> load_callee_saved;
        std::set<int> already_saved;
        int index = method_index[i]+1;
        if (i == 0)index = 1;
        struct method_scope * method_scope = instructions[index-1].root->method_scope;
        std::priority_queue<Node*,std::vector<Node*>, NodeComparator> priorityQueue;
        for (auto element : method_scope->node_table) {
            priorityQueue.push(element.second);
        }
        while (!priorityQueue.empty()) {
            struct Node * node = priorityQueue.top();
            priorityQueue.pop();
            decrease_degree_of_neighbors(node);
            std::make_heap(const_cast<Node**>(&priorityQueue.top()), const_cast<Node**>(&priorityQueue.top()) + priorityQueue.size(), NodeComparator());
            method_scope->coloring_stack.push(node);            
        }
        while (!method_scope->coloring_stack.empty()) {
            struct Node * node = method_scope->coloring_stack.top();
            method_scope->coloring_stack.pop();
            if (node->name.find("r") != std::string::npos || node->assigned_register != -1) {
                continue;
            }
            
            if (method_scope->register_in_use.size() == 0) {
                if (node->is_LVout_of_method) {
                    node->assigned_register = 4;
                    method_scope->callee_save_register_in_use.insert(4);
                    method_scope->callee_save_register_remain.erase(4);
                    method_scope->register_in_use.insert(4);
                    method_scope->register_remain.erase(4);
                } else {
                    node->assigned_register = 0;
                    method_scope->register_in_use.insert(0);
                    method_scope->register_remain.erase(0);
                }
            } else {
                int decided_register = -1;
                std::set<int> possible_register;
                if (node->is_LVout_of_method) {
                    possible_register = method_scope->callee_save_register_in_use;
                } else {
                    possible_register = method_scope->register_in_use;
                }
                for (auto element : possible_register) {
                    decided_register = element;
                    for (auto neighbor : node->neighbors) {
                        if (neighbor->assigned_register == decided_register) {
                            decided_register = -1;
                            break;
                        }
                    }
                    if (decided_register != -1) {
                        break;
                    }
                }
                if (decided_register == -1) {
                    if (node->is_LVout_of_method) {
                        decided_register = *method_scope->callee_save_register_remain.begin();
                        node->assigned_register = decided_register;
                        method_scope->callee_save_register_in_use.insert(decided_register);
                        method_scope->callee_save_register_remain.erase(decided_register);
                        method_scope->register_in_use.insert(decided_register);
                        method_scope->register_remain.erase(decided_register);
                    } else {
                        decided_register = *method_scope->register_remain.begin();
                        node->assigned_register = decided_register;
                        if (decided_register > 3) {
                            method_scope->callee_save_register_in_use.insert(decided_register);
                            method_scope->callee_save_register_remain.erase(decided_register);
                        }
                        method_scope->register_in_use.insert(decided_register);
                        method_scope->register_remain.erase(decided_register);
                    }
                } else {
                    node->assigned_register = decided_register;
                }


                if (node->assigned_register > 3 && method_scope->callee && already_saved.find(node->assigned_register) == already_saved.end()) {
                    instruction callee_save;
                    callee_save.instr = "str r" + std::to_string(node->assigned_register) + ", [sp, #" + node->offset + "]";
                    callee_save.instruction_id = instructions.size();
                    instructions.insert(instructions.begin()+index, callee_save);
                    all_method_index_behind_plus_plus(i);

                    instruction load_back;
                    load_back.instr = "ldr r" + std::to_string(node->assigned_register) + ", [sp, #" + node->offset + "]";
                    load_callee_saved.push_back(load_back);

                    already_saved.insert(node->assigned_register);
                }
            }
        }
        update_id_index_table();
        // after coloring, insert load callee saved instructions
        for (int j = 0; j < method_scope->return_instruction_id.size(); j++) {
            if (load_callee_saved.size() != 0) instructions[instr_id_to_index_block_id[method_scope->return_instruction_id[j]]].is_return = false;
            struct ASTNode * return_node = instructions[instr_id_to_index_block_id[method_scope->return_instruction_id[j]]].root;
            for (int k = 0; k < load_callee_saved.size(); k++) {
                int index = instr_id_to_index_block_id[method_scope->return_instruction_id[j]]+1+k;
                load_callee_saved[k].instruction_id = instructions.size();
                if (k == load_callee_saved.size()-1) {
                    load_callee_saved[k].root = return_node;
                    load_callee_saved[k].is_return = true;
                }
                instructions.insert(instructions.begin()+index, load_callee_saved[k]);
                all_method_index_behind_plus_plus(i);
            }
            update_id_index_table();
        }
    }
}


/*--------------------------------------------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------------------*/
// Helper functions // 
void debug() {
    for (int i = 0 ; i < instructions.size() ; i++) {
        printf("Instruction %d: %s", i, instructions[i].instr.c_str());
        printf("\n");
        printf("USE of block %d: ", i);
        for (auto element : instructions[i].use) {
            printf("%s ", element.c_str());
        }
        printf("\n");
        printf("DEF of block %d: ", i);
        for (auto element : instructions[i].def) {
            printf("%s ", element.c_str());
        }
        printf("\n");
        printf("LVin of block %d: ", i);
        for (auto element : instructions[i].LVin) {
            printf("%s ", element.c_str());
        }
        printf("\n");
        printf("LVout of block %d: ", i);
        for (auto element : instructions[i].LVout) {
            printf("%s ", element.c_str());
        }
        printf("\n\n");
    }
}

void update_id_index_table() {
    for (int i = 0 ; i < instructions.size() ; i++) {
        instr_id_to_index_block_id[instructions[i].instruction_id] = i;
    }
}

void all_method_index_behind_plus_plus(int index) {
    for (int i = index+1; i < method_index.size(); i++) {
        method_index[i]++;
    }
}

// valgrind --leak-check=full ./codegen test.java