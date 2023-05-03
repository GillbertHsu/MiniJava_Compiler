#include "typecheck.hh"
#include "node.hh"
#include "codegen.hh"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>

extern struct SymbolTableEntry * method_symbol_table[50];

std::vector<struct instruction> instructions;
std::vector<struct instruction> string_declarations;
std::vector<struct instruction> static_int_declarations;
std::vector<int> method_index = {0};
int counter_of_messages = 1;
int counter_of_if = 0;
int counter_of_while = 0;

void traverse_and_codegen(struct ASTNode * root, bool is_method) {
    if (root == NULL) return;
    for (int i = 0; i < root->num_children; i++) {
        if (is_method == false) {
            if (root->node_type == NODETYPE_STATICMETHODDECLLIST)return;
            traverse_and_codegen(root->children[i], is_method);
        } else {
            if (root->node_type == NODETYPE_STATICVARDECLLIST || root->node_type == NODETYPE_MAINMETHOD)return;
            traverse_and_codegen(root->children[i], is_method);
        }
    }
    if (root->node_type == NODETYPE_EXP_TYPE) {
        create_instruction_for_exp_type(root);
    } else if (root->node_type == NODETYPE_EXP_LEFT) {
        create_instruction_for_exp_leftID(root);
    } else if (root->node_type == NODETYPE_VARDECL) {
        create_instruction_for_varDecl(root);
    } else if (root->node_type == NODETYPE_ASSIGN) {
        create_instruction_for_assign(root);
    } else if (root->node_type == NODETYPE_EXP_PLUS) {
        create_instruction_for_exp_plus(root);
    } else if (root->node_type == NODETYPE_EXP_OPERATION) {
        create_instruction_for_exp_operation(root);
    } else if (root->node_type == NODETYPE_PRINTLN) {
        create_instruction_for_print_println(root);
    } else if (root->node_type == NODETYPE_PRINT) {
        create_instruction_for_print_println(root);
    } else if (root->node_type == NODETYPE_STATICMETHODDECL && is_method == true) {
        create_instruction_for_method_decl(root);
    } else if (root->node_type == NODETYPE_MAINMETHOD && is_method == false) {
        create_instruction_for_method_decl(root);
    } else if (root->node_type == NODETYPE_EXP_METHODCALL || root->node_type == NODETYPE_STATEMENT_METHODCALL) {
        create_instruction_for_method_call(root);
    } else if (root->node_type == NODETYPE_RETURN) {
        create_instruction_for_return(root);
    } else if (root->node_type == NODETYPE_EXP_NEG || root->node_type == NODETYPE_EXP_POS || root->node_type == NODETYPE_EXP_LOGIC_NOT) {
        create_instruction_for_exp_neg_pos(root);
    } else if (root->node_type == NODETYPE_IF) {
        create_instruction_for_if(root);
    } else if (root->node_type == NODETYPE_WHILE) {
        create_instruction_for_while(root);
    } else if (root->node_type == NODETYPE_EXP_OPERATION_COMP) {
        create_instruction_for_exp_operation_comp(root);
    } else if (root->node_type == NODETYPE_EXP_LOGIC) {
        create_instruction_for_exp_logic(root);
    } else if (root->node_type == NODETYPE_EXP_COMP) {
        create_instruction_for_exp_comp(root);
    } else if (root->node_type == NODETYPE_EXP_ONE_ARR) {
        create_instruction_for_exp_one_arr(root);
    } else if (root->node_type == NODETYPE_EXP_TWO_ARR) {
        create_instruction_for_exp_two_arr(root);
    } else if (root->node_type == NODETYPE_EXP_LENGTH) {
        create_instruction_for_exp_length(root);
    }
}
// TODO: cancel drag instruction all ldr instead. Need to change the structure of if else
void convert_to_real_assembly() {
    std::vector<struct instruction> copy = instructions;
    for (int i = 0; i < instructions.size(); i++ ) {
        if (instructions[i].instrType == MOV_LEFTVAL) {  
            convert_mov(copy, i);
        } else if (instructions[i].instrType == ADD_MINUS_MUL) {
            convert_exp_operation_plus(copy, i);
        } else if (instructions[i].instrType == LDR_LITERAL) {
            convert_ldr(copy, i);
        } else if (instructions[i].instrType == STR_ASSIGN_VARDECL) {
            convert_str(copy, i);
        } else if (instructions[i].instrType == MOV_CMP) {
            convert_exp_operation_comp(copy, i);
        } else if (instructions[i].instrType == AND_OR) {
            convert_exp_logic(copy, i);
        } else if (instructions[i].instrType == NEG || instructions[i].instrType == LOGIC_NOT || instructions[i].instrType == POS) {
            convert_neg_not(copy, i);
        } else if (instructions[i].instrType == EXP_LENGTH) {
            convert_exp_length(copy, i);
        } else if (instructions[i].instrType == LDR_FROM_MEM) {
            convert_ldr_from_mem(copy, i);
        } else if (instructions[i].instrType == IF_WHILE_CMP) {
            convert_if_while_cmp(copy, i);
        }
    }
}

void generateCode(char * filename) {
    FILE *fptr;
    char * assembly = strcat(filename, ".s");
    fptr = fopen(assembly, "w");
    
    fprintf(fptr, ".section .data\n");
    for (int i = 0; i < string_declarations.size(); i++) {
        fprintf(fptr, "%s\n", string_declarations[i].instr.c_str());
    }
    for (int i = 0; i < static_int_declarations.size(); i++) {
        fprintf(fptr, "%s\n", static_int_declarations[i].instr.c_str());
    }

    fprintf(fptr, "string_format: .asciz \"%%s\"\n\n");
    fprintf(fptr, ".section .text\n");
    fprintf(fptr, "int_format: .asciz \"%%d\"\n");
    fprintf(fptr, "newline: .asciz \"\\n\"\n\n");
    fprintf(fptr, ".global main\n");
    fprintf(fptr, ".balign 4\n"); 
    convert_to_real_assembly();
    for (int i = 0 ; i < method_index.size()-1; i++) {
        fprintf(fptr, "%s\n", instructions[method_index[i]].instr.c_str());
        int start = method_index[i]+1;
        for (int j = start; j < method_index[i+1]; j++) {
            if (instructions[j].is_return) {
                struct ASTNode * method = find_method(instructions[j].root);
                instructions[j].instr += "\n    add sp, sp, #"+std::to_string(method->symbolic_register->symbolic_register_counter*4)+"\n";
                instructions[j].instr += "    pop {pc}";
            }
            if (instructions[j].instrType ==  NO_NEED_INDENT) {
                fprintf(fptr, "%s\n", instructions[j].instr.c_str());
            } else {
                fprintf(fptr, "    %s\n", instructions[j].instr.c_str());
            }
        }
        std::string release_space = "    add sp, sp, #"+std::to_string(instructions[method_index[i]].root->symbolic_register->symbolic_register_counter*4)+"\n";
        if (i==0)fprintf(fptr, release_space.c_str());
        if (i==0)fprintf(fptr, "    pop {pc}\n");
    }

    fclose(fptr);
}

void convert_mov(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    if (instructions[i].entry->is_static == false && instructions[i].entry->type != DATATYPE_STR && instructions[i].argument_number == 0) {
        if (instructions[i].root->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            instructions[i].instr = "ldr r" + num + ", [sp, #"+ std::to_string(instructions[i].entry->offset_number) + "]";
        } else {
            instructions[i].instr = "ldr r" + num + ", [r1, #0]";
        } 
        instructions[i].instr += "\n    str r" + num + ", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
    } else if (instructions[i].entry->is_static == false && instructions[i].entry->type != DATATYPE_STR && instructions[i].argument_number > 0) {
        std::string arg = std::to_string(instructions[i].argument_number-1);
        if (instructions[i].root->children[0]->node_type == NODETYPE_LEFTVALUE_ID)
            instructions[i].instr = "ldr r"+ arg +", [sp, #"+ std::to_string(instructions[i].entry->offset_number) + "]";
        else
            instructions[i].instr = "ldr r"+ arg +", [r1, #0]";
        instructions[i].instr += "\n    str r"+ arg +", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
    } else if (instructions[i].entry->is_static == true && instructions[i].entry->type != DATATYPE_STR && instructions[i].argument_number == 0) {
        std::string id;
        id.assign(instructions[i].entry->id);
        instructions[i].instr = "ldr r" + num + ", ="+ id;
        instructions[i].instr += "\n    ldr r" + num + ", [r" + num + "]";
    } else if (instructions[i].entry->is_static == true && instructions[i].entry->type != DATATYPE_STR && instructions[i].argument_number > 0) {
        std::string id; id.assign(instructions[i].entry->id);
        std::string arg = std::to_string(instructions[i].argument_number-1);
        instructions[i].instr = "ldr r"+ arg +", ="+ id;
        instructions[i].instr += "\n    ldr r"+ arg +", [r"+ arg +"]";
    } else if (instructions[i].entry->type == DATATYPE_STR) {
        if (instructions[i].root->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
            std::string id;
            id.assign(instructions[i].entry->id);
            instructions[i].instr = "ldr r" + num + ", ="+ id;
            instructions[i].instr += "\n    ldr r" + num + ", [r" + num + "]";
            if (instructions[i].entry->is_arg) {
                instructions[i].instr = "ldr r" + num + ", [sp, #"+ std::to_string(instructions[i].entry->offset_number) + "]";
            }
        } else {
            instructions[i].instr = "ldr r" + num + ", [r1, #0]";
            instructions[i].instr += "\n    str r" + num + ", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
        }
    }
}

void convert_ldr(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    if (instructions[i].root->data.type == DATATYPE_INT) {
        int found = instructions[i].instr.find("=");
        if (instructions[i].argument_number == 0) {
            instructions[i].instr = "ldr r" + num + ", ="+ instructions[i].instr.substr(found + 1);
            instructions[i].instr += "\n    str r"+num+", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
        } else {
            instructions[i].instr = "ldr r" + std::to_string(instructions[i].argument_number-1) + ", ="+ instructions[i].instr.substr(found + 1);
            instructions[i].instr += "\n    str r"+std::to_string(instructions[i].argument_number-1)+", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
        }
    } else if (instructions[i].root->data.type == DATATYPE_BOOLEAN) {
        // just like ldr exptype but with # instead of = (for boolean)
        int found = instructions[i].instr.find("#");
        if (instructions[i].argument_number == 0) {
            instructions[i].instr = "mov r" + num + ", #"+ instructions[i].instr.substr(found + 1);
            instructions[i].instr += "\n    str r"+num+", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
        } else {
            instructions[i].instr = "mov r" + std::to_string(instructions[i].argument_number-1) + ", #"+ instructions[i].instr.substr(found + 1);
            instructions[i].instr += "\n    str r"+std::to_string(instructions[i].argument_number-1)+", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
        }
    } else if (instructions[i].root->data.type == DATATYPE_STR) {
        int found = instructions[i].instr.find("=");
        if (instructions[i].argument_number == 0) {
            instructions[i].instr = "ldr r" + num + ", ="+ instructions[i].instr.substr(found + 1);
        } else {
            instructions[i].instr = "ldr r" + std::to_string(instructions[i].argument_number-1) + ", ="+ instructions[i].instr.substr(found + 1);
        }
    }
}

void convert_exp_operation_plus(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    std::vector<std::string> tokens = split(copy[i].instr, " ");
    int operand1 = std::stoi(tokens[3].substr(2));
    int operand2 = std::stoi(tokens[5].substr(2));
    instructions[i].instr = "ldr r0, [sp, #"+ std::to_string(operand1*4) + "]\n    ";
    instructions[i].instr += "ldr r1, [sp, #"+ std::to_string(operand2*4) + "]\n    ";

    if (copy[i].instr.find("add $") != std::string::npos) {
        instructions[i].instr += "add r2, r0, r1";
    } else if (copy[i].instr.find("sub $") != std::string::npos) {
        instructions[i].instr += "sub r2, r0, r1";
    } else if (copy[i].instr.find("mul $") != std::string::npos) {
        instructions[i].instr += "mul r2, r0, r1";
    }
    instructions[i].instr += "\n    str r2, [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
}

void convert_exp_operation_comp(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    std::vector<std::string> tokens = split(copy[i].instr, " ");
    int operand1 = std::stoi(tokens[3].substr(2));
    int operand2 = std::stoi(tokens[5].substr(2));
    instructions[i].instr = "ldr r0, [sp, #"+ std::to_string(operand1*4) + "]\n    ";
    instructions[i].instr += "ldr r1, [sp, #"+ std::to_string(operand2*4) + "]\n    ";

    if (copy[i].instr.find("movgt $") != std::string::npos) {
        instructions[i].instr += "cmp r0, r1\n    mov r0, #0\n    movgt r0, #1";
    } else if (copy[i].instr.find("movlt $") != std::string::npos) {
        instructions[i].instr += "cmp r0, r1\n    mov r0, #0\n    movlt r0, #1";
    } else if (copy[i].instr.find("movge $") != std::string::npos) {
        instructions[i].instr += "cmp r0, r1\n    mov r0, #0\n    movge r0, #1";
    } else if (copy[i].instr.find("movle $") != std::string::npos) {
        instructions[i].instr += "cmp r0, r1\n    mov r0, #0\n    movle r0, #1";
    } else if (copy[i].instr.find("moveq $") != std::string::npos) {
        instructions[i].instr += "cmp r0, r1\n    mov r0, #0\n    moveq r0, #1";
    } else if (copy[i].instr.find("movne $") != std::string::npos) {
        instructions[i].instr += "cmp r0, r1\n    mov r0, #0\n    movne r0, #1";
    }
    instructions[i].instr += "\n    str r0, [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
}

void convert_exp_logic(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    std::vector<std::string> tokens = split(copy[i].instr, " ");
    int operand1 = std::stoi(tokens[3].substr(2));
    int operand2 = std::stoi(tokens[5].substr(2));
    instructions[i].instr = "ldr r0, [sp, #"+ std::to_string(operand1*4) + "]\n    ";
    instructions[i].instr += "ldr r1, [sp, #"+ std::to_string(operand2*4) + "]\n    ";

    if (copy[i].instr.find("and $") != std::string::npos) {
        instructions[i].instr += "and r2, r0, r1";
    } else if (copy[i].instr.find("orr $") != std::string::npos) {
        instructions[i].instr += "orr r2, r0, r1";
    }
    instructions[i].instr += "\n    str r2, [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
}

void convert_neg_not(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    std::vector<std::string> tokens = split(copy[i].instr, " ");
    int operand1 = std::stoi(tokens[3].substr(2));
    instructions[i].instr = "ldr r0, [sp, #"+ std::to_string(operand1*4) + "]\n    ";

    // TODO:probably can change to r+num for every r0
    if (copy[i].instr.find("neg $") != std::string::npos) {
        instructions[i].instr += "neg r0, r0";
    } else if (copy[i].instr.find("mvn $") != std::string::npos) {
        instructions[i].instr += "mvn r0, r0\n    and r0, r0, #1";
    } else if (copy[i].instr.find("mov $") != std::string::npos) {
        instructions[i].instr += "mov r0, r0";
    }
    instructions[i].instr += "\n    str r0, [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
}

void convert_str(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    instructions[i].instr = "ldr r"+num+", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";
    
    if (instructions[i].root->node_type == NODETYPE_ASSIGN && instructions[i].root->children[0]->node_type != NODETYPE_LEFTVALUE_ID) {
        instructions[i].instr += "\n    str r"+num+", [r0, #0]";
    } else {
        instructions[i].instr += "\n    str r"+num+", [sp, #"+ std::to_string(instructions[i].entry->offset_number) + "]";
    }
}

void convert_exp_length(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    instructions[i].instr = "ldr r"+num+", [r0, #-4]";
    instructions[i].instr += "\n    str r"+num+", [sp, #"+ std::to_string(instructions[i].reg_number*4) + "]";

}

void convert_ldr_from_mem(std::vector<struct instruction> copy, int i) {
    std::string num = std::to_string(instructions[i].is_left);
    instructions[i].instr = "ldr r"+ num+ ", [sp, #"+ std::to_string(instructions[i].entry->offset_number) + "]";
}

void convert_if_while_cmp(std::vector<struct instruction> copy, int i) {
    instructions[i].instr = "ldr r0, [sp, #" + std::to_string(instructions[i].reg_number*4) + "]\n    ";
    instructions[i].instr += "cmp r0, #0";
}

struct ASTNode * find_init_parent(struct ASTNode * root) {
    if (root->node_type == NODETYPE_VARDECL)return root;
    find_init_parent(root->parent);
}

void create_instruction_for_exp_type(struct ASTNode * root) {
    struct ASTNode * parent = exploreNodeTypEXP_Parent(root->parent);
    if (parent->node_type == NODETYPE_EXP_NEG || parent->node_type == NODETYPE_EXP_POS || parent->node_type == NODETYPE_EXP_LOGIC_NOT)parent = exploreNodeTypEXP_Parent(parent->parent);
    if (root->data.type == DATATYPE_INT) {
        if (!(parent->node_type == NODETYPE_INIT && find_init_parent(parent)->parent->node_type == NODETYPE_STATICVARDECL) && 
            !(parent->node_type == NODETYPE_ASSIGN && findLeftValueID(parent->children[0])->is_static)) {
            instruction new_instruction;
            new_instruction.instrType = LDR_LITERAL;
            new_instruction.root = root;
            std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
            new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
            root->symbolic_register->symbolic_register_counter++;
            new_instruction.instr = "ldr " + symb_reg + " , =" + std::to_string(root->data.value.int_value);
            new_instruction.instruction_id = instructions.size();
            instructions.push_back(new_instruction);
            root->instr = new_instruction;
        } 
    } else if (root->data.type == DATATYPE_BOOLEAN) {
        if (!(parent->node_type == NODETYPE_INIT && find_init_parent(parent)->parent->node_type == NODETYPE_STATICVARDECL) && 
            !(parent->node_type == NODETYPE_ASSIGN && findLeftValueID(parent->children[0])->is_static)) {
            instruction new_instruction;
            new_instruction.root = root;
            new_instruction.instrType = LDR_LITERAL;
            std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
            new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
            root->symbolic_register->symbolic_register_counter++;
            if (root->data.value.boolean_value == false) {
                new_instruction.instr = "ldr " + symb_reg + " , #0";
            } else {
                new_instruction.instr = "ldr " + symb_reg + " , #1";
            }
            new_instruction.instruction_id = instructions.size();
            instructions.push_back(new_instruction);
            root->instr = new_instruction;
        }
    } else if (root->data.type == DATATYPE_STR) {
        instruction load_message;
        load_message.root = root;
        load_message.instr = "message" + std::to_string(counter_of_messages) + ": .asciz " + root->data.value.string_value;
        load_message.message_number = counter_of_messages;
        counter_of_messages++;
        string_declarations.push_back(load_message);        

        instruction new_instruction;
        std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
        new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
        root->symbolic_register->symbolic_register_counter++;
        new_instruction.instrType = LDR_LITERAL;
        new_instruction.root = root;
        new_instruction.message_number = load_message.message_number;
        new_instruction.instr = "ldr " +symb_reg+ ", =message" + std::to_string(new_instruction.message_number);
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);  
        root->instr = new_instruction;
    }
}

void create_instruction_for_exp_leftID(struct ASTNode * root) {
    struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
    instruction new_instruction;
    new_instruction.instrType = MOV_LEFTVAL;
    new_instruction.root = root;
    if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
        std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
        new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
        root->symbolic_register->symbolic_register_counter++;
        new_instruction.instr = "mov " + symb_reg + " , " + root->children[0]->data.value.string_value;
        new_instruction.entry = entry;
        new_instruction.operand1_number = entry->offset_number/4;
        new_instruction.argument_number = 0;
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
        root->instr = new_instruction;
        root->children[0]->instr = new_instruction;
    } else {
        struct ASTNode * index = exploreNodeTypEXP(root->children[0]->children[0]);
        if (strcmp(entry->id, "args") == 0)entry->offset_number = 0;
        
        instruction load_index;
        load_index.root = root;
        load_index.instr = "ldr r0, [sp, #" + std::to_string(index->instr.reg_number*4) + "]";
        load_index.instruction_id = instructions.size();
        instructions.push_back(load_index);
        
        instruction lsl_index;
        lsl_index.root = root;
        lsl_index.instrType = LSL;
        lsl_index.instr = "lsl r0, r0, #2";
        lsl_index.instruction_id = instructions.size();
        instructions.push_back(lsl_index);

        instruction load_base_address;
        load_base_address.root = root;
        load_base_address.instrType = LDR_FROM_MEM;
        load_base_address.instr = "ldr r1, [sp, #" + std::to_string(entry->offset_number) + "]";
        load_base_address.is_left = 1;
        load_base_address.entry = entry;
        load_base_address.instruction_id = instructions.size();
        instructions.push_back(load_base_address);

        instruction add_index;
        add_index.root = root;
        add_index.instr = "add r1, r1, r0";
        add_index.instruction_id = instructions.size();
        instructions.push_back(add_index);

        if (root->children[0]->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            struct ASTNode * index2 = exploreNodeTypEXP(root->children[0]->children[1]);

            instruction load_index2;
            load_index2.root = root;
            load_index2.instr = "ldr r0, [sp, #" + std::to_string(index2->instr.reg_number*4) + "]";
            load_index2.instruction_id = instructions.size();
            instructions.push_back(load_index2);

            instruction lsl_index2;
            lsl_index2.root = root;
            lsl_index2.instrType = LSL;
            lsl_index2.instr = "lsl r0, r0, #2";
            lsl_index2.instruction_id = instructions.size();
            instructions.push_back(lsl_index2);

            instruction load_base_address2;
            load_base_address2.root = root;
            load_base_address2.instr = "ldr r1, [r1, #0]";
            load_base_address2.instruction_id = instructions.size();
            instructions.push_back(load_base_address2);

            instruction add_index2;
            add_index2.root = root;
            add_index2.instr = "add r1, r1, r0";
            add_index2.instruction_id = instructions.size();
            instructions.push_back(add_index2);
        }
        std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
        new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
        root->symbolic_register->symbolic_register_counter++;
        new_instruction.instr = "mov " + symb_reg + " , " + root->children[0]->data.value.string_value;
        new_instruction.operand1_number = load_base_address.reg_number;
        new_instruction.entry = entry;
        new_instruction.argument_number = 0;
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
        root->instr = new_instruction;
        root->children[0]->instr = new_instruction;
    }
}

void create_instruction_for_exp_length(struct ASTNode * root) {
    struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
    instruction load_base_address;
    load_base_address.instr = "ldr r0, [sp, #" + std::to_string(entry->offset_number) + "]";
    load_base_address.entry = entry;
    load_base_address.root = root;
    load_base_address.instrType = LDR_FROM_MEM;
    load_base_address.instruction_id = instructions.size();
    instructions.push_back(load_base_address);

    if (root->children[0]->node_type == NODETYPE_LEFTVALUE_ID) {
        int symb_reg = root->symbolic_register->symbolic_register_counter;
        std::string symb_reg_str = "$t"+std::to_string(symb_reg);
        root->symbolic_register->symbolic_register_counter++;
        instruction load_length;
        load_length.instr = "ldr "+symb_reg_str+", [r0, #-4]";
        load_length.instruction_id = instructions.size();
        load_length.root = root;
        load_length.reg_number = symb_reg;
        load_length.instrType = EXP_LENGTH;
        root->instr = load_length;
        instructions.push_back(load_length);
    } else {
        instruction load_base_address2;
        load_base_address2.instr = "ldr r0, [r0, #0]";
        load_base_address2.instruction_id = instructions.size();
        instructions.push_back(load_base_address2);

        int symb_reg = root->symbolic_register->symbolic_register_counter;
        std::string symb_reg_str = "$t"+std::to_string(symb_reg);
        root->symbolic_register->symbolic_register_counter++;
        instruction load_length2;
        load_length2.instr = "ldr "+symb_reg_str+", [r0, #-4]";
        load_length2.instruction_id = instructions.size();
        load_length2.root = root;
        load_length2.reg_number = symb_reg;
        load_length2.instrType = EXP_LENGTH;
        root->instr = load_length2;
        instructions.push_back(load_length2);
    }
    
}

void create_instruction_for_exp_plus(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (FindExpType(left) == DATATYPE_INT) {
        instruction new_instruction;
        new_instruction.root = root;
        new_instruction.instrType = ADD_MINUS_MUL;
        std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
        new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
        root->symbolic_register->symbolic_register_counter++;
        int left_symbolic_reg = left->instr.reg_number;
        int right_symbolic_reg = right->instr.reg_number;
        new_instruction.instr = "add "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
        root->instr = new_instruction;
    } else {
        
        instruction load_value1;
        load_value1.root = right;
        load_value1.instr = "ldr r0, [sp, #"+std::to_string(left->instr.reg_number*4)+"]";
        load_value1.instruction_id = instructions.size();
        instructions.push_back(load_value1);
        
        
        instruction load_value2;
        load_value2.root = right;
        load_value2.instr = "ldr r1, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
        load_value2.instruction_id = instructions.size();
        instructions.push_back(load_value2);
        
        instruction new_instruction;
        new_instruction.root = root;
        new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
        root->symbolic_register->symbolic_register_counter++;
        new_instruction.instr = "bl strcat";
        new_instruction.instr += "\n    str r0, [sp, #" + std::to_string(new_instruction.reg_number*4) + "]";
        new_instruction.instr += "\n    bl strlen";
        new_instruction.instr += "\n    lsl r0, r0, #1";
        new_instruction.instr += "\n    bl malloc";
        new_instruction.instr += "\n    ldr r1, [sp, #" + std::to_string(new_instruction.reg_number*4) + "]";
        new_instruction.instr += "\n    bl strcpy";
        new_instruction.instr += "\n    str r0, [sp, #" + std::to_string(new_instruction.reg_number*4) + "]";
        new_instruction.instruction_id = instructions.size();
        root->instr = new_instruction;
        instructions.push_back(new_instruction);
        instructions[find_instruction_index(right)].is_left = 1;
    }
}

void create_instruction_for_exp_operation(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    instruction new_instruction;
    new_instruction.instrType = ADD_MINUS_MUL;
    new_instruction.root = root;
    std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
    new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    int left_symbolic_reg = left->instr.reg_number;
    int right_symbolic_reg = right->instr.reg_number;
    if (strcmp(root->data.value.string_value,"-") == 0) {
        new_instruction.instr = "sub "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,"*") == 0) {
        new_instruction.instr = "mul "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,"/") == 0) {
        
    } 
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    root->instr = new_instruction;
}

void create_instruction_for_exp_operation_comp(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    instruction new_instruction;
    new_instruction.root = root;
    new_instruction.instrType = MOV_CMP;
    std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
    new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    int left_symbolic_reg = left->instr.reg_number;
    int right_symbolic_reg = right->instr.reg_number;
    if (strcmp(root->data.value.string_value,"<") == 0) {
        new_instruction.instr = "movlt "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,">") == 0) {
        new_instruction.instr = "movgt "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,"<=") == 0) {
        new_instruction.instr = "movle "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,">=") == 0) {
        new_instruction.instr = "movge "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    }
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    root->instr = new_instruction;

}

void create_instruction_for_exp_logic(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    instruction new_instruction;
    new_instruction.root = root;
    new_instruction.instrType = AND_OR;
    std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
    new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    int left_symbolic_reg = left->instr.reg_number;
    int right_symbolic_reg = right->instr.reg_number;
    if (strcmp(root->data.value.string_value,"&&") == 0) {
        new_instruction.instr = "and "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,"||") == 0) {
        new_instruction.instr = "orr "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    }
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    root->instr = new_instruction;
}

void create_instruction_for_exp_comp(struct ASTNode * root) {
    struct ASTNode * left = exploreNodeTypEXP(root->children[0]); struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    instruction new_instruction;
    new_instruction.root = root;
    new_instruction.instrType = MOV_CMP;
    std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
    new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    int left_symbolic_reg = left->instr.reg_number;
    int right_symbolic_reg = right->instr.reg_number;
    if (strcmp(root->data.value.string_value,"==") == 0) {
        new_instruction.instr = "moveq "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    } else if (strcmp(root->data.value.string_value,"!=") == 0) {
        new_instruction.instr = "movne "+symb_reg+" , "+ "$t"+std::to_string(left_symbolic_reg)+" , "+ "$t"+std::to_string(right_symbolic_reg);
    }
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    root->instr = new_instruction;
}

void create_instruction_for_exp_neg_pos(struct ASTNode * root) {
    struct ASTNode * right = exploreNodeTypEXP(root->children[0]);
    instruction new_instruction;
    new_instruction.root = root;
    std::string symb_reg = "$t"+std::to_string(root->symbolic_register->symbolic_register_counter);
    new_instruction.reg_number = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    int right_symbolic_reg = right->instr.reg_number;
    if (root->node_type == NODETYPE_EXP_NEG) {
        new_instruction.instrType = NEG;
        new_instruction.instr = "neg " + symb_reg + " , " + "$t" + std::to_string(right_symbolic_reg);
    } else if (root->node_type == NODETYPE_EXP_POS){
        new_instruction.instrType = POS;
        new_instruction.instr = "mov " + symb_reg + " , " + "$t" + std::to_string(right_symbolic_reg);
    } else if (root->node_type == NODETYPE_EXP_LOGIC_NOT){
        new_instruction.instrType = LOGIC_NOT;
        new_instruction.instr = "mvn " +symb_reg+ " , " + "$t" + std::to_string(right_symbolic_reg);
    }
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    root->instr = new_instruction;
    
}


// working on vardecl with method return string

void create_instruction_for_varDecl(struct ASTNode * root) {
    if (root->num_children > 1 == false) {
        struct SymbolTableEntry * entry = findLeftValueID(root);
        if (entry->type == DATATYPE_INT || entry->type == DATATYPE_BOOLEAN) {
            if (entry->is_static) {
                instruction new_instruction;
                new_instruction.root = root;
                std::string id;
                id.assign(root->data.value.string_value);
                new_instruction.instr = id + ": .skip 4";
                static_int_declarations.push_back(new_instruction);
                root->instr = new_instruction;
            } else {
                entry->offset_number = 4*root->symbolic_register->symbolic_register_counter;
                root->symbolic_register->symbolic_register_counter++;
            }
        } else if (entry->type == DATATYPE_STR) {
            instruction new_instruction;
            new_instruction.root = root;
            std::string id;
            id.assign(root->data.value.string_value);
            new_instruction.instr = id + ": .skip 64";
            string_declarations.push_back(new_instruction);
            root->instr = new_instruction;
        }
        return;
    }
    struct ASTNode * right = exploreNodeTypEXP(root->children[1]->children[0]);
    if (root->children[0]->node_type == NODETYPE_TYPE_ONE_ARR || root->children[0]->node_type == NODETYPE_TYPE_TWO_ARR) {
        instruction new_instruction;
        int right_symbolic_reg = right->instr.reg_number;
        new_instruction.root = root;
        new_instruction.reg_number = right_symbolic_reg;
        int offset = 4*new_instruction.reg_number;
        new_instruction.instr = "str $t" + std::to_string(right_symbolic_reg) + " , " + "[sp,"+ " #"+ std::to_string(offset) + "]";
        new_instruction.instrType = STR_ASSIGN_VARDECL;
        struct SymbolTableEntry * entry = findLeftValueID(root);
        entry->offset_number = offset;
        new_instruction.entry = entry;
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
        root->instr = new_instruction;
        if (right->node_type == NODETYPE_EXP_TWO_ARR)create_instruction_for_allocate_second_dimension(right);
    } else {
        if (root->children[root->num_children-1]->node_type == NODETYPE_IDEXPLIST) {
            enum DataType type = FindType(root->children[0]);
            create_instruction_for_idExpList(root->children[root->num_children-2], type, 0);
            create_instruction_for_idExpList(root->children[root->num_children-1], type, 0);
        } else {
            // store string in rodata
            if (FindType(root->children[0]) == DATATYPE_STR) {
                std::string id;
                id.assign(root->data.value.string_value);
                instruction new_instruction;
                new_instruction.root = root;
                new_instruction.instr = id + ": .skip 64";
                string_declarations.push_back(new_instruction);
                root->instr = new_instruction;
                if (right->node_type == NODETYPE_EXP_TYPE || right->node_type == NODETYPE_EXP_LEFT) {
                    instruction store_string;
                    store_string.instr = "ldr r1, ="+id;
                    store_string.instr += "\n    str r0, [r1]";
                    store_string.instruction_id = instructions.size();
                    store_string.root = root;
                    instructions.push_back(store_string);
                } else {
                    instruction store_string;
                    store_string.instr = "ldr r0, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
                    store_string.instr += "\n    ldr r1, ="+id;
                    store_string.instr += "\n    str r0, [r1]";
                    store_string.instruction_id = instructions.size();
                    store_string.root = root;
                    instructions.push_back(store_string);
                }
            } else if (root->parent->node_type == NODETYPE_STATICVARDECL) {
                std::string id;
                id.assign(root->data.value.string_value);
                instruction new_instruction;
                new_instruction.root = root;
                new_instruction.instr = id + ": .skip 4";
                static_int_declarations.push_back(new_instruction);
                root->instr = new_instruction;
                if (FindType(root->children[0]) == DATATYPE_INT) {
                    instruction store_static_int;
                    store_static_int.instr = "ldr r0, =" + std::to_string(right->data.value.int_value);
                    store_static_int.instr += "\n    ldr r1, ="+id;
                    store_static_int.instr += "\n    str r0, [r1]";
                    store_static_int.instruction_id = instructions.size();
                    store_static_int.root = root;
                    instructions.push_back(store_static_int);
                    root->instr = store_static_int;
                } else if (FindType(root->children[0]) == DATATYPE_BOOLEAN) {
                    instruction store_static_bool;
                    store_static_bool.instr = "mov r0, #"+std::to_string(right->data.value.boolean_value);
                    store_static_bool.instr += "\n    ldr r1, ="+id;
                    store_static_bool.instr += "\n    str r0, [r1]";
                    store_static_bool.instruction_id = instructions.size();
                    store_static_bool.root = root;
                    instructions.push_back(store_static_bool);
                }
            } else {
                instruction new_instruction;
                int right_symbolic_reg = right->instr.reg_number;
                new_instruction.root = root;
                new_instruction.reg_number = right_symbolic_reg;
                int offset = 4*new_instruction.reg_number;
                new_instruction.instr = "str $t" + std::to_string(right_symbolic_reg) + " , " + "[sp,"+ " #"+ std::to_string(offset) + "]";
                new_instruction.instrType = STR_ASSIGN_VARDECL;
                struct SymbolTableEntry * entry = findLeftValueID(root);
                entry->offset_number = offset;
                new_instruction.entry = entry;
                new_instruction.instruction_id = instructions.size();
                instructions.push_back(new_instruction);
                root->instr = new_instruction;
            }
        }
    }
}

// not deal with static int yet
void create_instruction_for_idExpList(struct ASTNode * root, enum DataType type, int dim_of_array) {
    if (root == NULL) return;
    if (root->node_type != NODETYPE_INIT) {
        create_instruction_for_idExpList(root->children[0], type, dim_of_array);
        if (root->num_children == 2)create_instruction_for_idExpList(root->children[1], type, dim_of_array);
    } else {
        struct ASTNode * right = exploreNodeTypEXP(root->children[0]);
        // need to check root, root here is not varDecl
        if (type == DATATYPE_STR) {
            instruction new_instruction;
            new_instruction.root = root;
            std::string id;
            id.assign(root->parent->data.value.string_value);
            new_instruction.instr = id + ": .skip 64";
            string_declarations.push_back(new_instruction);
            root->instr = new_instruction;

            
            instruction store_string;
            store_string.instr = "ldr r0, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
            store_string.instr += "\n    ldr r1, ="+id;
            store_string.instr += "\n    str r0, [r1]";
            store_string.instruction_id = instructions.size();
            store_string.root = root;
            instructions.push_back(store_string);
            
        } else if (find_init_parent(root)->parent->node_type == NODETYPE_STATICVARDECL ) {
            std::string id;
            id.assign(root->parent->data.value.string_value);
            instruction new_instruction;
            new_instruction.root = find_init_parent(root);
            new_instruction.instr = id + ": .skip 4";
            static_int_declarations.push_back(new_instruction);
            root->instr = new_instruction;
            if (type == DATATYPE_INT) {
                instruction store_static_int;
                store_static_int.instr = "ldr r0, =" + std::to_string(right->data.value.int_value);
                store_static_int.instr += "\n    ldr r1, ="+id;
                store_static_int.instr += "\n    str r0, [r1]";
                store_static_int.instruction_id = instructions.size();
                store_static_int.root = root;
                instructions.push_back(store_static_int);
                root->instr = store_static_int;
            } else if (type == DATATYPE_BOOLEAN) {
                instruction store_static_bool;
                store_static_bool.instr = "mov r0, #"+std::to_string(right->data.value.boolean_value);
                store_static_bool.instr += "\n    ldr r1, ="+id;
                store_static_bool.instr += "\n    str r0, [r1]";
                store_static_bool.instruction_id = instructions.size();
                store_static_bool.root = root;
                instructions.push_back(store_static_bool);
                root->instr = store_static_bool;
            }
        } else {
            instruction new_instruction;
            int right_symbolic_reg = right->instr.reg_number;
            new_instruction.root = root;
            new_instruction.reg_number = right_symbolic_reg;
            int offset = 4*new_instruction.reg_number;
            new_instruction.instr = "str $t" + std::to_string(right_symbolic_reg) + " , " + "[sp,"+ " #"+ std::to_string(offset) + "]";
            new_instruction.instrType = STR_ASSIGN_VARDECL;
            struct SymbolTableEntry * entry = findLeftValueID(root->parent);
            entry->offset_number = offset;
            new_instruction.entry = entry;
            new_instruction.instruction_id = instructions.size();
            root->instr = new_instruction;
            auto it = std::find_if(instructions.begin(), instructions.end(), [&](const instruction& i) {
                return i.instruction_id == right->instr.instruction_id;
            });
            int index = std::distance(instructions.begin(), it);
            instructions.insert(instructions.begin()+index+1, new_instruction);
        }
    }
}

void create_instruction_for_assign(struct ASTNode * root) {
    struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
    struct ASTNode * left = root->children[0];
    struct ASTNode * right = exploreNodeTypEXP(root->children[1]);
    if (left->node_type == NODETYPE_LEFTVALUE_ID) {
        if (entry->type == DATATYPE_STR) {
            std::string id;
            id.assign(entry->id);
            instruction store_string;
            if (right->node_type == NODETYPE_EXP_TYPE || right->node_type == NODETYPE_EXP_LEFT) {
                store_string.instr += "ldr r1, ="+id;
                store_string.instr += "\n    str r0, [r1]";
                store_string.instruction_id = instructions.size();
                store_string.root = root;
                instructions.push_back(store_string);
            } else {
                store_string.instr = "ldr r0, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
                store_string.instr += "\n    ldr r1, ="+id;
                store_string.instr += "\n    str r0, [r1]";
                store_string.instruction_id = instructions.size();
                store_string.root = root;
                instructions.push_back(store_string);
            }
        } else if (entry->type == DATATYPE_INT || entry->type == DATATYPE_BOOLEAN) {
            if (entry->is_static) {
                std::string id;
                id.assign(entry->id);
                instruction store_static_int;
                store_static_int.instr = "ldr r0, =" + std::to_string(right->data.value.int_value);
                store_static_int.instr += "\n    ldr r1, ="+id;
                store_static_int.instr += "\n    str r0, [r1]";
                store_static_int.instruction_id = instructions.size();
                store_static_int.root = root;
                instructions.push_back(store_static_int);
            } else {
                instruction new_instruction;
                new_instruction.root = root;
                int right_symbolic_reg = right->instr.reg_number;
                int offset = entry->offset_number;
                new_instruction.reg_number = right_symbolic_reg;
                new_instruction.instr = "str $t" + std::to_string(right_symbolic_reg) + " , " + "[sp,"+ " #"+ std::to_string(offset) + "]";
                new_instruction.instrType = STR_ASSIGN_VARDECL;
                new_instruction.entry = entry;
                new_instruction.instruction_id = instructions.size();
                instructions.push_back(new_instruction);
                root->instr = new_instruction;
            }
        }
    } else {
        struct ASTNode * index = exploreNodeTypEXP(left->children[0]);
        instruction get_index_offset;
        get_index_offset.instr = "ldr r0, [sp, #"+std::to_string(index->instr.reg_number*4)+"]\n    ";
        get_index_offset.instr += "lsl r0, r0, #2";
        get_index_offset.instruction_id = instructions.size();
        instructions.push_back(get_index_offset);

        instruction load_address;
        load_address.root = index;
        load_address.instr = "ldr r1, [sp, #"+std::to_string(entry->offset_number)+"]";
        load_address.instrType = LDR_FROM_MEM;
        load_address.entry = entry;
        load_address.is_left = 1;
        load_address.instruction_id = instructions.size();
        instructions.push_back(load_address);

        instruction add_offset;
        add_offset.instr = "add r1, r1, r0";
        add_offset.instruction_id = instructions.size();
        instructions.push_back(add_offset);

        if (left->node_type == NODETYPE_LEFTVALUE_ONE_ARR) {
            
            instruction load_value;
            load_value.root = right;
            load_value.instr = "ldr r0, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
            load_value.instruction_id = instructions.size();
            instructions.push_back(load_value);
            
            instruction new_instruction;
            new_instruction.root = root;
            int right_symbolic_reg = right->instr.reg_number;;
            new_instruction.reg_number = right_symbolic_reg;
            new_instruction.instr = "str r0, [r1, #0]";
            new_instruction.entry = entry;
            new_instruction.instruction_id = instructions.size();
            instructions.push_back(new_instruction);
            root->instr = new_instruction;
        } else if (left->node_type == NODETYPE_LEFTVALUE_TWO_ARR) {
            struct ASTNode * index2 = exploreNodeTypEXP(left->children[1]);
            instruction get_index_offset2;
            get_index_offset2.instr = "ldr r0, [sp, #"+std::to_string(index2->instr.reg_number*4)+"]\n    ";
            get_index_offset2.instr += "lsl r0, r0, #2";
            get_index_offset2.instruction_id = instructions.size();
            instructions.push_back(get_index_offset2);     

            instruction load_address2;
            load_address2.root = index2;
            load_address2.instr = "ldr r1, [r1, #0]";
            load_address2.instr += "\n    add r1, r1, r0";
            load_address2.instruction_id = instructions.size();
            instructions.push_back(load_address2);

            
            instruction load_value;
            load_value.root = right;
            load_value.instr = "ldr r0, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
            load_value.instruction_id = instructions.size();
            instructions.push_back(load_value);
            

            instruction new_instruction;
            new_instruction.root = root;
            int right_symbolic_reg = right->instr.reg_number;;
            new_instruction.reg_number = right_symbolic_reg;
            new_instruction.instr = "str r0, [r1, #0]";
            new_instruction.entry = entry;
            new_instruction.instruction_id = instructions.size();
            instructions.push_back(new_instruction);
            root->instr = new_instruction;
        }
    }
}


void create_instruction_for_print_println(struct ASTNode * root) {
    struct ASTNode * right = exploreNodeTypEXP(root->children[0]);
    if ((right->node_type == NODETYPE_EXP_TYPE && right->data.type == DATATYPE_INT) || right->node_type == NODETYPE_EXP_LENGTH) {
        instruction int_format;
        int_format.instr = "ldr r0 , =int_format";
        int_format.instrType = ONE_OPERAND;
        int_format.instruction_id = instructions.size();
        instructions.push_back(int_format);
        instructions[find_instruction_index(right)].is_left = 1;
    } else if (right->node_type == NODETYPE_EXP_LEFT) {
        struct SymbolTableEntry * entry = findLeftValueID(right->children[0]);
        if (entry->type != DATATYPE_STR) {
            instruction int_format;
            int_format.instrType = ONE_OPERAND;
            int_format.instr = "ldr r0 , =int_format";
            int_format.instruction_id = instructions.size();
            instructions.push_back(int_format);
            instructions[find_instruction_index(right)].is_left = 1;
        }
    } else if (right->node_type == NODETYPE_EXP_OPERATION || 
                right->node_type == NODETYPE_EXP_PLUS ||
                right->node_type == NODETYPE_EXP_NEG ||
                right->node_type == NODETYPE_EXP_POS || 
                right->node_type == NODETYPE_EXP_LOGIC ||
                right->node_type == NODETYPE_EXP_LOGIC_NOT ||
                right->node_type == NODETYPE_EXP_COMP ||
                right->node_type == NODETYPE_EXP_OPERATION_COMP) {
            enum DataType type = FindExpType(right);
            if (type != DATATYPE_STR) {
                instruction int_format;
                int_format.instr = "ldr r0 , =int_format";
                int_format.instrType = ONE_OPERAND;
                int_format.instruction_id = instructions.size();
                instructions.push_back(int_format);
                instruction new_instruction;
                new_instruction.root = root;
                new_instruction.instr = "ldr r1 , [sp, #" + std::to_string(right->instr.reg_number*4) + "]";
                new_instruction.instrType = ONE_OPERAND;
                new_instruction.operand1_number = right->instr.reg_number;
                new_instruction.instruction_id = instructions.size();
                instructions.push_back(new_instruction);
                instructions[find_instruction_index(right)].is_left = 1;
            }

    } else if (right->node_type == NODETYPE_EXP_METHODCALL) {
        enum DataType type = FindExpType(right);
        if (type == DATATYPE_INT) {
            instruction int_format;
            int_format.instr = "ldr r0 , =int_format";
            int_format.instrType = ONE_OPERAND;
            int_format.instruction_id = instructions.size();
            instructions.push_back(int_format);
            instruction new_instruction;
            new_instruction.root = root;
            new_instruction.instrType = ONE_OPERAND;
            new_instruction.instr = "ldr r1 , [sp, #" + std::to_string(right->instr.reg_number*4) + "]";
            new_instruction.instruction_id = instructions.size();
            instructions.push_back(new_instruction);
            instructions[find_instruction_index(right)].is_left = 1;
        } else if (type == DATATYPE_STR) {
            instruction load_string;
            load_string.instr = "ldr r0 , [sp, #" + std::to_string(right->instr.reg_number*4) + "]";
            load_string.instruction_id = instructions.size();
            load_string.root = root;
            instructions.push_back(load_string);
        }
    }

    instruction new_instruction;
    new_instruction.root = root;
    new_instruction.instr = "bl printf";
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    if (root->node_type == NODETYPE_PRINTLN) {
        instruction println;
        println.root = root;
        println.instr = "ldr r0 , =newline";
        println.instrType = ONE_OPERAND;
        println.instruction_id = instructions.size();
        instructions.push_back(println);
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
    }
}

void create_instruction_for_method_decl(struct ASTNode * root) {
    instruction new_instruction;
    new_instruction.root = root;
    std::string method_name;
    if (root->node_type == NODETYPE_MAINMETHOD) root->data.value.string_value = "main";
    method_name.assign(root->data.value.string_value);
    new_instruction.instr = method_name + ":";
    new_instruction.instr += "\n    push {lr}";
    new_instruction.instr += "\n    sub sp, sp, #"+std::to_string(root->symbolic_register->symbolic_register_counter*4);
    if (root->node_type == NODETYPE_MAINMETHOD) {
        new_instruction.instr += "\n    add r1, r1, #4\n    str r1, [sp, #0]";
    } else if (root->node_type == NODETYPE_STATICMETHODDECL) {
        struct SymbolTableEntry * entry = find_method(root->data.value.string_value);
        for (int i = 0; i < entry->number_of_parameters; i++) {
            new_instruction.instr += "\n    str r" + std::to_string(i) + ", [sp, #" + std::to_string(i*4) + "]";
            struct SymbolTableEntry * param = entry->correspond_scope->entries[i];
            param->offset_number = i*4;
            param->is_arg = true;
        }
    }
    new_instruction.instruction_id = instructions.size();
    // insert the instruction to the first position of the method
    instructions.insert(instructions.begin()+method_index.back(), new_instruction);
    method_index.push_back(instructions.size());
    root->instr = new_instruction;
}

void create_instruction_for_method_call(struct ASTNode * root) {
    instruction new_instruction;
    new_instruction.root = root;
    if (root->children[0]->node_type == NODETYPE_METHODCALL) {
        struct SymbolTableEntry * entry = findLeftValueID(root->children[0]);
        struct TypeList * list = NULL;
        if (root->children[0]->num_children > 0) {
            list = exploreExpCommaListType(root->children[0]->children[0]);
        }
        int counter = 1;
        while (list != NULL) {
            if (list->node->parent->node_type == NODETYPE_EXP_LEFT) {
                instruction load_var;
                load_var.instr = "ldr r" + std::to_string(counter-1) + " , [sp, #" + std::to_string(list->node->parent->instr.reg_number*4) + "]";
                load_var.instruction_id = instructions.size();
                instructions.push_back(load_var);
            } else if (list->node->node_type == NODETYPE_EXP_TYPE) {
                instruction load_int;
                load_int.instr = "ldr r" + std::to_string(counter-1) + " , [sp, #" + std::to_string(list->node->instr.reg_number*4) + "]";
                load_int.instruction_id = instructions.size();
                instructions.push_back(load_int);
            } else if (list->node->parent->node_type == NODETYPE_EXP_METHODCALL) {
                instruction load_int;
                load_int.instr = "ldr r" + std::to_string(counter-1) + " , [sp, #" + std::to_string(list->node->parent->instr.reg_number*4) + "]";
                load_int.instrType = ONE_OPERAND;
                load_int.instruction_id = instructions.size();
                instructions.push_back(load_int);
            } else if (list->node->node_type == NODETYPE_EXP_PLUS || list->node->node_type == NODETYPE_EXP_OPERATION ||
                       list->node->node_type == NODETYPE_EXP_OPERATION_COMP || list->node->node_type == NODETYPE_EXP_COMP ||
                       list->node->node_type == NODETYPE_EXP_LOGIC_NOT || list->node->node_type == NODETYPE_EXP_LOGIC ||
                       list->node->node_type == NODETYPE_EXP_NEG || list->node->node_type == NODETYPE_EXP_POS) {
                enum DataType type = FindExpType(list->node);
                if (type != DATATYPE_STR) {
                    instruction load_int;
                    load_int.instrType = ONE_OPERAND;
                    load_int.instr = "ldr r" + std::to_string(counter-1) + " , [sp, #" + std::to_string(list->node->instr.reg_number*4) + "]";
                    load_int.instruction_id = instructions.size();
                    instructions.push_back(load_int);
                }
            }
            list = list->next;
            counter++;
        }
        std::string method_name;
        method_name.assign(root->children[0]->data.value.string_value);
        new_instruction.root = root;
        new_instruction.instrType = METHOD_CALL;
        new_instruction.instr = "bl " + method_name;
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);

    } else if (root->children[0]->node_type == NODETYPE_PARSEINT) {
        struct ASTNode * right = exploreNodeTypEXP(root->children[0]->children[0]);
        if (right->node_type == NODETYPE_EXP_METHODCALL) {
            instruction load_string;
            load_string.instr = "ldr r0 , [sp, #" + std::to_string(right->instr.reg_number*4) + "]";
            load_string.instrType = ONE_OPERAND;
            load_string.instruction_id = instructions.size();
            load_string.root = root;
            instructions.push_back(load_string);
        } else if (right->node_type == NODETYPE_EXP_PLUS) {
            // TODO: string concatenation
        }
        new_instruction.instr = "bl atoi";
        new_instruction.instrType = METHOD_CALL;
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
    }
    instruction return_value;
    int symbolic_register = root->symbolic_register->symbolic_register_counter;
    return_value.reg_number = symbolic_register;
    root->symbolic_register->symbolic_register_counter++;
    return_value.instr = "str r0 , [sp, #" + std::to_string(symbolic_register*4) + "]";
    return_value.instruction_id = instructions.size();
    return_value.root = root;
    return_value.instrType = ONE_OPERAND;
    instructions.push_back(return_value);
    root->instr = return_value;
} 

struct ASTNode * find_method(struct ASTNode * root) {
    if (root->node_type == NODETYPE_STATICMETHODDECL) return root;
    else return find_method(root->parent);
}

void create_instruction_for_return(struct ASTNode * root) {
    struct ASTNode * right = exploreNodeTypEXP(root->children[0]);
    if (right->node_type == NODETYPE_EXP_OPERATION || right->node_type == NODETYPE_EXP_PLUS ||
        right->node_type == NODETYPE_EXP_OPERATION_COMP || right->node_type == NODETYPE_EXP_COMP ||
        right->node_type == NODETYPE_EXP_LOGIC_NOT || right->node_type == NODETYPE_EXP_LOGIC ||
        right->node_type == NODETYPE_EXP_NEG || right->node_type == NODETYPE_EXP_POS) {
        instruction new_instruction;
        new_instruction.root = root;
        new_instruction.is_return = true;
        new_instruction.instr = "ldr r0 , [sp, #" + std::to_string(right->instr.reg_number*4) + "]";
        new_instruction.instrType = ONE_OPERAND;
        new_instruction.instruction_id = instructions.size();
        instructions.push_back(new_instruction);
    } else {
        int index = find_instruction_index(right);
        instructions[index].is_return = true;
    }
}

void dfs(struct ASTNode * root, struct ASTNode *& max_depth_node, std::string if_or_while, bool& is_set) {
    if (root == NULL || is_set)return;
    for (int i = 0 ; i < root->num_children; i++) {
        dfs(root->children[i], max_depth_node, if_or_while, is_set);
    }
    if (root->node_type == NODETYPE_EXP_TYPE || root->node_type == NODETYPE_EXP_LEFT) {
        max_depth_node = root;
        is_set = true;
    }
}    

void set_if(struct ASTNode * root, int counter_of_if) {
    struct ASTNode * max_depth_node = NULL;
    bool is_set = false;
    dfs(root, max_depth_node, "if", is_set);

    struct ASTNode * ifNode = exploreNodeTypEXP_Parent(root->parent);

    if (max_depth_node != NULL) {
        int index = find_instruction_index(max_depth_node);
        
        instructions[index].is_then = true;
        instruction if_label;
        if_label.instr = "_if" + std::to_string(counter_of_if) + ":";
        if_label.instruction_id = instructions.size();
        if_label.instrType = NO_NEED_INDENT;
        instructions.insert(instructions.begin() + index, if_label);
        instructions[index].branch_number = counter_of_if;
    }
}

void set_then(struct ASTNode * root, int counter_of_if) {
    struct ASTNode * max_depth_node = NULL;
    bool is_set = false;
    dfs(root, max_depth_node, "if", is_set);

    struct ASTNode * ifNode = exploreNodeTypEXP_Parent(root->parent);
    instruction branch;
    if (ifNode->node_type == NODETYPE_EXP_LOGIC_NOT)ifNode = exploreNodeTypEXP_Parent(ifNode->parent);
    if (ifNode->children[2]->node_type == NODETYPE_STATEMENT) {
        branch.instr = "beq end_if" + std::to_string(counter_of_if);
    } else if (ifNode->children[2]->node_type == NODETYPE_IF) {
        branch.instr = "beq _if" + std::to_string(ifNode->children[2]->instr.branch_number);
    } else {
        branch.instr = "beq _false" + std::to_string(counter_of_if);
    }

    int condition_index = find_instruction_index(exploreNodeTypEXP(ifNode->children[0]));
    instruction compare;
    compare.instrType = IF_WHILE_CMP;
    std::string symb_reg = "$t"+std::to_string(instructions[condition_index].reg_number);
    compare.reg_number = instructions[condition_index].reg_number;
    compare.instr = "cmp "+symb_reg+" , #0";


    if (max_depth_node != NULL) {
        int index = find_instruction_index(max_depth_node);
        branch.instruction_id = instructions.size();
        instructions.insert(instructions.begin()+index, branch);
        compare.instruction_id = instructions.size();
        instructions.insert(instructions.begin()+index, compare);
    } else {
        compare.instruction_id = instructions.size();
        instructions.insert(instructions.begin()+condition_index+1, compare);
        branch.instruction_id = instructions.size();
        instructions.insert(instructions.begin()+condition_index+2, branch);
    }

    instruction branch_endif;
    branch_endif.instrType = BRANCH;
    branch_endif.instr = "b end_if" + std::to_string(counter_of_if);

    bool noreturn = true;
    if (root->node_type == NODETYPE_STATEMENTLIST && root->children[0]->node_type == NODETYPE_STATEMENTLIST) {
        int num_children = root->children[0]->num_children;
        if (root->children[0]->children[num_children-1]->node_type == NODETYPE_RETURN)noreturn = false;
    }
    
    if (root->node_type != NODETYPE_RETURN && noreturn) {
        branch_endif.instruction_id = instructions.size();
        struct ASTNode * last_instruction_node = find_first_node_not_statementList(root);
        int index = find_instruction_index(last_instruction_node);
        instructions.insert(instructions.begin()+index+1, branch_endif);
    }
    
}

void set_else(struct ASTNode * root, int counter_of_if) {
    struct ASTNode * max_depth_node = NULL;
    int max_depth = -1;
    bool is_set = false;
    dfs(root, max_depth_node, "if", is_set);
    if (max_depth_node != NULL) {
        int index = find_instruction_index(max_depth_node);
        instruction false_label;
        false_label.instr = "_false" + std::to_string(counter_of_if) + ":";
        false_label.instrType = NO_NEED_INDENT;
        instructions[index].branch_number = counter_of_if;

        if (root->node_type != NODETYPE_IF) {
            false_label.instruction_id = instructions.size();
            instructions.insert(instructions.begin()+index, false_label);
        }
    }
}

void set_do(struct ASTNode * root, int counter_of_while) {
    struct ASTNode * max_depth_node = NULL;
    int max_depth = -1;
    bool is_set = false;
    dfs(root, max_depth_node, "while", is_set);
    if (max_depth_node != NULL) {
        int index = find_instruction_index(max_depth_node);
        instructions[index].is_do = true;
        instruction while_label;
        while_label.instrType = NO_NEED_INDENT;
        while_label.instr = "_while" + std::to_string(counter_of_while) + ":";
        while_label.instruction_id = instructions.size();
        instructions.insert(instructions.begin()+index, while_label);
        
        instructions[index].branch_number = counter_of_while;
    }
}

void create_instruction_for_if(struct ASTNode * root) {
    struct ASTNode * condition = exploreNodeTypEXP(root->children[0]);
    int index = find_instruction_index(condition);
    instructions[index].branch_number = counter_of_if;

    set_if(root->children[0], counter_of_if);
    set_then(root->children[1], counter_of_if);


    set_else(root->children[2], counter_of_if);
    
    instruction new_instruction;
    new_instruction.instrType = NO_NEED_INDENT;
    new_instruction.instr = "b end_if"+std::to_string(counter_of_if);
    new_instruction.branch_number = counter_of_if;
    root->instr = new_instruction;
    int new_instruction_index = -1;

    if (find_end_if(new_instruction.instr)) {
        new_instruction.instr = "end_if"+std::to_string(counter_of_if) + ":";
        new_instruction.instruction_id = instructions.size();
        new_instruction_index = instructions.size();
        instructions.push_back(new_instruction);
    }

    counter_of_if++; 
}

void create_instruction_for_while(struct ASTNode * root) {
    struct ASTNode * condition = exploreNodeTypEXP(root->children[0]);
    int index = find_instruction_index(condition);
    instructions[index].is_while = true;
    instructions[index].branch_number = counter_of_while;

    instruction compare;
    compare.instrType = IF_WHILE_CMP;
    std::string symb_reg = "$t"+std::to_string(instructions[index].reg_number);
    compare.instr = "cmp "+symb_reg+" , #0";
    compare.instruction_id = instructions.size();
    compare.reg_number = instructions[index].reg_number;
    instructions.insert(instructions.begin()+index+1, compare);

    instruction branch;
    branch.instr = "beq end_while" + std::to_string(instructions[index].branch_number);
    branch.instruction_id = instructions.size();
    instructions.insert(instructions.begin()+index+2, branch);

    set_do(root->children[0], counter_of_while);

    instruction branch_instruction;
    branch_instruction.instrType = BRANCH;
    branch_instruction.instr = "b _while"+std::to_string(counter_of_while);
    branch_instruction.instruction_id = instructions.size();
    int index_of_b_while = instructions.size();
    instructions.push_back(branch_instruction);

    instruction new_instruction;
    new_instruction.instrType = NO_NEED_INDENT;
    new_instruction.instr = "end_while"+std::to_string(counter_of_while) + ":";
    new_instruction.instruction_id = instructions.size();
    root->instr = new_instruction;
    int index_of_end_while = instructions.size();
    instructions.push_back(new_instruction);

    
    counter_of_while++;
}

void create_instruction_for_exp_one_arr(struct ASTNode * root) {
    struct ASTNode * right = exploreNodeTypEXP(root->children[1]);

    instruction allocate_memory; 
    int symbolic_register = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    allocate_memory.root = root;
    allocate_memory.reg_number = symbolic_register;
    allocate_memory.instr = "ldr r0, [sp, #"+std::to_string(right->instr.reg_number*4)+"]";
    allocate_memory.instr += "\n    add r0, r0, #1";
    allocate_memory.instr += "\n    lsl r0, r0, #2";
    allocate_memory.instr += "\n    bl malloc";
    allocate_memory.instr += "\n    str r0, [sp, #"+std::to_string(symbolic_register*4)+"]";
    allocate_memory.instruction_id = instructions.size();
    root->instr = allocate_memory;
    instructions.push_back(allocate_memory);

    store_array_length(right, symbolic_register);

}

void create_instruction_for_exp_two_arr(struct ASTNode * root) {
    struct ASTNode * index1 = exploreNodeTypEXP(root->children[1]);
    struct ASTNode * parent = find_init_parent(root);
    struct SymbolTableEntry * entry = findLeftValueID(parent);

    instruction allocate_memory;
    int symbolic_register = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    allocate_memory.root = root;
    allocate_memory.reg_number = symbolic_register;
    allocate_memory.instr = "ldr r0, [sp, #"+std::to_string(index1->instr.reg_number*4)+"]";
    allocate_memory.instr += "\n    add r0, r0, #1";
    allocate_memory.instr += "\n    lsl r0, r0, #2";
    allocate_memory.instr += "\n    bl malloc";
    allocate_memory.instr += "\n    str r0, [sp, #"+std::to_string(symbolic_register*4)+"]";
    allocate_memory.instruction_id = instructions.size();
    root->instr = allocate_memory;
    instructions.push_back(allocate_memory);

    store_array_length(index1, symbolic_register);

}

void create_instruction_for_allocate_second_dimension(struct ASTNode * root) {
    struct ASTNode * index1 = exploreNodeTypEXP(root->children[1]);struct ASTNode * index2 = exploreNodeTypEXP(root->children[2]);
    instruction initialize_counter;
    initialize_counter.instr = "mov r0, #0";
    int symbolic_register2 = root->symbolic_register->symbolic_register_counter;
    root->symbolic_register->symbolic_register_counter++;
    initialize_counter.reg_number = symbolic_register2;
    initialize_counter.instr += "\n    str r0, [sp, #"+std::to_string(symbolic_register2*4)+"]";
    initialize_counter.branch_number = counter_of_while;
    initialize_counter.instruction_id = instructions.size();
    instructions.push_back(initialize_counter);


    instruction while_label;
    while_label.instrType = NO_NEED_INDENT;
    while_label.instr = "_while" + std::to_string(counter_of_while) + ":";
    while_label.instruction_id = instructions.size();
    instructions.push_back(while_label);

    instruction while_loop;
    while_loop.instr = "ldr r2, [sp, #"+std::to_string(symbolic_register2*4)+"]";
    while_loop.instr += "\n    ldr r0, [sp, #"+std::to_string(index1->instr.reg_number*4)+"]";
    while_loop.instr += "\n    cmp r2, r0";
    while_loop.instr += "\n    bge end_while"+std::to_string(counter_of_while);
    while_loop.is_do = true;
    while_loop.branch_number = counter_of_while;
    while_loop.instruction_id = instructions.size();
    instructions.push_back(while_loop);

    instruction allocate_memory_for_each_row;
    allocate_memory_for_each_row.root = root;
    allocate_memory_for_each_row.instr = "ldr r0, [sp, #"+std::to_string(index2->instr.reg_number*4)+"]";
    allocate_memory_for_each_row.instr += "\n    add r0, r0, #1";
    allocate_memory_for_each_row.instr += "\n    lsl r0, r0, #2";
    allocate_memory_for_each_row.instr += "\n    bl malloc";
    allocate_memory_for_each_row.instr += "\n    ldr r1, [sp, #"+std::to_string(root->instr.reg_number*4)+"]";
    allocate_memory_for_each_row.instr += "\n    ldr r2, [sp, #"+std::to_string(symbolic_register2*4)+"]";
    allocate_memory_for_each_row.instr += "\n    lsl r2, r2, #2";
    allocate_memory_for_each_row.instr += "\n    add r1, r1, r2";
    allocate_memory_for_each_row.instr += "\n    str r0, [r1, #0]";
    allocate_memory_for_each_row.instr += "\n    ldr r2, [sp, #"+std::to_string(index2->instr.reg_number*4)+"]";
    allocate_memory_for_each_row.instr += "\n    str r2, [r0, #0]";
    allocate_memory_for_each_row.instr += "\n    add r0, r0, #4";
    allocate_memory_for_each_row.instr += "\n    str r0, [r1, #0]";
    allocate_memory_for_each_row.instr += "\n    ldr r2, [sp, #"+std::to_string(symbolic_register2*4)+"]";
    allocate_memory_for_each_row.instr += "\n    add r2, r2, #1";
    allocate_memory_for_each_row.instr += "\n    str r2, [sp, #"+std::to_string(symbolic_register2*4)+"]";
    allocate_memory_for_each_row.instruction_id = instructions.size();
    instructions.push_back(allocate_memory_for_each_row);

    instruction branch_instruction;
    branch_instruction.instr = "b _while"+std::to_string(counter_of_while);
    branch_instruction.instruction_id = instructions.size();
    instructions.push_back(branch_instruction);

    instruction new_instruction;
    new_instruction.instrType = NO_NEED_INDENT;
    new_instruction.instr = "end_while"+std::to_string(counter_of_while) + ":";
    new_instruction.instruction_id = instructions.size();
    instructions.push_back(new_instruction);
    counter_of_while++;
}


/*--------------------------------------------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------------------*/
// Helper functions // 
std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

int find_instruction_index(struct ASTNode * root) {
    auto it = std::find_if(instructions.begin(), instructions.end(), [&](const instruction& i) {
        return i.instruction_id == root->instr.instruction_id;
    });
    int index = std::distance(instructions.begin(), it);
    return index;
} 

void drag_instruction(struct ASTNode * root, int destination) {
    auto it = std::find_if(instructions.begin(), instructions.end(), [&](const instruction& i) {
        return i.instruction_id == root->instr.instruction_id;
    });
    int index = std::distance(instructions.begin(), it);
    instructions.erase(instructions.begin()+index);
    if (destination == -1) {
        instructions.push_back(root->instr);
    } else {
        instructions.insert(instructions.begin()+destination+1, root->instr);
    }
}

void store_array_length(struct ASTNode * root, int symbolic_register) {
    instruction store_array_length;
    store_array_length.instr = "ldr r1, [sp, #"+std::to_string(root->instr.reg_number*4)+"]";
    store_array_length.instr += "\n    ldr r0, [sp, #"+std::to_string(symbolic_register*4)+"]";
    store_array_length.instr += "\n    str r1, [r0, #0]";
    store_array_length.instr += "\n    add r0, r0, #4";
    store_array_length.instr += "\n    str r0, [sp, #"+std::to_string(symbolic_register*4)+"]";
    store_array_length.instruction_id = instructions.size();
    instructions.push_back(store_array_length);
}

bool find_end_if(std::string instr) {
    for (int i = 0; i < instructions.size(); i++) {
        if (instructions[i].instr.find(instr) != std::string::npos) {
            return true;
        }
    }
    return false;
}

int find_instruction_index_by_id(int id) {
    auto it = std::find_if(instructions.begin(), instructions.end(), [&](const instruction& i) {
        return i.instruction_id == id;
    });
    int index = std::distance(instructions.begin(), it);
    return index;
} 

struct ASTNode * find_first_node_not_statementList(struct ASTNode * root) {
    if (root == NULL) return NULL;
    if (root->node_type == NODETYPE_STATEMENTLIST || root->node_type == NODETYPE_STATEMENT) {
        return find_first_node_not_statementList(root->children[root->num_children-1]);
    } else {
        return root;
    }
}

// valgrind --leak-check=full ./codegen test.java
// scp hsu226@data.cs.purdue.edu:~/CS352/p4/test.s .
// nohup ./frpc -c frpc.ini &