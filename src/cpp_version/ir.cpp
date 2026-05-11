#include "ir.h"
#include <cassert>
#include <cstring>

// ============================================================================
// IRFunction helpers
// ============================================================================
int IRFunction::newValue(const std::string& name, DataType type) {
    int id = (int)values.size();
    SSAValue v;
    v.id = id;
    v.name = name;
    v.type = type;
    values.push_back(v);
    return id;
}

int IRFunction::newBlock(const std::string& label) {
    int id = (int)blocks.size();
    BasicBlock bb;
    bb.id = id;
    bb.label = label + "_" + std::to_string(id);
    blocks.push_back(bb);
    return id;
}

void IRFunction::addSucc(int from, int to) {
    blocks[from].succs.push_back(to);
    blocks[to].preds.push_back(from);
}

// ============================================================================
// IRGenerator
// ============================================================================
int IRGenerator::freshVal(DataType type) {
    std::string name = "t" + std::to_string(tempCounter_++);
    return func_->newValue(name, type);
}

int IRGenerator::emit(IROpcode op, int dst, std::vector<int> operands,
                      int64_t imm, const std::string& label) {
    IRInstr instr;
    instr.op = op;
    instr.dst = dst;
    instr.operands = std::move(operands);
    instr.imm = imm;
    instr.label = label;
    func_->blocks[curBlock_].instrs.push_back(instr);
    return dst;
}

int IRGenerator::emitConst(int64_t val) {
    int dst = freshVal(DataType::INT);
    emit(IROpcode::CONST_INT, dst, {}, val);
    return dst;
}

int IRGenerator::emitConstBool(bool val) {
    int dst = freshVal(DataType::BOOLEAN);
    emit(IROpcode::CONST_BOOL, dst, {}, val ? 1 : 0);
    return dst;
}

int IRGenerator::emitCopy(int src) {
    int dst = freshVal(func_->values[src].type);
    emit(IROpcode::COPY, dst, {src});
    return dst;
}

void IRGenerator::writeVar(const std::string& name, int ssaVal) {
    varMap_[name] = ssaVal;
}

int IRGenerator::readVar(const std::string& name) {
    auto it = varMap_.find(name);
    if (it != varMap_.end()) return it->second;
    // Variable not yet defined in this path - return a zero
    int v = emitConst(0);
    varMap_[name] = v;
    return v;
}

ASTNode* IRGenerator::unwrapExp(ASTNode* node) {
    if (!node) return nullptr;
    if (node->node_type == NodeType::EXP && !node->children.empty())
        return unwrapExp(node->children[0].get());
    return node;
}

SymbolInfo* IRGenerator::findLeftValueInfo(ASTNode* node) {
    if (!node) return nullptr;
    if (node->node_type == NodeType::LEFTVALUE_ID ||
        node->node_type == NodeType::LEFTVALUE_ONE_ARR ||
        node->node_type == NodeType::LEFTVALUE_TWO_ARR) {
        if (node->scope) {
            auto* info = node->scope->lookup(node->str_val);
            if (info) return info;
        }
        return methods_->lookup(node->str_val);
    }
    if (node->node_type == NodeType::VAR_DECL ||
        node->node_type == NodeType::ID_EXP_LIST) {
        if (node->scope) {
            auto* info = node->scope->lookup(node->str_val);
            if (info) return info;
        }
        return nullptr;
    }
    if (node->node_type == NodeType::METHOD_CALL ||
        node->node_type == NodeType::CALL_MAIN)
        return methods_->lookup(node->str_val);
    if (node->node_type == NodeType::EXP_METHOD_CALL && !node->children.empty())
        return findLeftValueInfo(node->children[0].get());
    return nullptr;
}

// Collect variable names that are assigned in the given AST subtree
static void collectModifiedVars(ASTNode* node, std::set<std::string>& vars) {
    if (!node) return;
    if (node->node_type == NodeType::ASSIGN && !node->children.empty()) {
        auto* lv = node->children[0].get();
        if (lv && !lv->str_val.empty())
            vars.insert(lv->str_val);
    }
    if (node->node_type == NodeType::VAR_DECL && !node->str_val.empty()) {
        vars.insert(node->str_val);
    }
    for (auto& child : node->children)
        collectModifiedVars(child.get(), vars);
}

DataType IRGenerator::findExpType(ASTNode* root) {
    root = unwrapExp(root);
    if (!root) return DataType::UNDEFINED;
    switch (root->node_type) {
        case NodeType::EXP_TYPE: return root->data_type;
        case NodeType::EXP_LENGTH: return DataType::INT;
        case NodeType::EXP_LEFT:
            if (!root->children.empty()) {
                auto* info = findLeftValueInfo(root->children[0].get());
                if (info) return info->type;
            }
            return DataType::UNDEFINED;
        case NodeType::EXP_METHOD_CALL:
            if (!root->children.empty()) {
                if (root->children[0]->node_type == NodeType::PARSE_INT) return DataType::INT;
                auto* info = findLeftValueInfo(root->children[0].get());
                if (info) return info->type;
            }
            return DataType::UNDEFINED;
        case NodeType::EXP_PLUS: {
            if (!root->children.empty())
                return findExpType(root->children[0].get());
            return DataType::UNDEFINED;
        }
        case NodeType::EXP_OPERATION: return DataType::INT;
        case NodeType::EXP_OPERATION_COMP:
        case NodeType::EXP_COMP:
        case NodeType::EXP_LOGIC: return DataType::BOOLEAN;
        case NodeType::EXP_NEG:
        case NodeType::EXP_POS: return DataType::INT;
        case NodeType::EXP_LOGIC_NOT: return DataType::BOOLEAN;
        default:
            if (!root->children.empty()) return findExpType(root->children[0].get());
            return DataType::UNDEFINED;
    }
}

// ============================================================================
// Top-level generation
// ============================================================================
IRModule IRGenerator::generate(ASTNode* programRoot, MethodTable& methods) {
    methods_ = &methods;
    module_ = IRModule();

    // Walk the AST to find main class
    ASTNode* mainClass = programRoot->children.empty() ? nullptr : programRoot->children[0].get();
    if (!mainClass) return module_;

    // Process static variable declarations (in data section)
    // Also collect VAR_DECL nodes for initialization code
    std::vector<ASTNode*> staticVarDecls;
    for (auto& child : mainClass->children) {
        if (child->node_type == NodeType::STATIC_VAR_DECL_LIST) {
            for (auto& svd : child->children) {
                if (svd->node_type == NodeType::STATIC_VAR_DECL && !svd->children.empty()) {
                    auto* varDecl = svd->children[0].get();
                    module_.staticVars.push_back(varDecl->str_val);
                    staticVarDecls.push_back(varDecl);
                    // Also add additional variables from multi-declarations
                    for (size_t k = 1; k < varDecl->children.size(); k++) {
                        auto* extra = varDecl->children[k].get();
                        if (extra->node_type == NodeType::ID_EXP_LIST)
                            module_.staticVars.push_back(extra->str_val);
                    }
                }
            }
        }
    }

    // Generate methods first, then main
    for (auto& child : mainClass->children) {
        if (child->node_type == NodeType::STATIC_METHOD_DECL_LIST) {
            for (auto& method : child->children) {
                genFunction(method.get(), false);
            }
        }
    }
    staticVarDecls_ = staticVarDecls;
    for (auto& child : mainClass->children) {
        if (child->node_type == NodeType::MAINMETHOD) {
            genFunction(child.get(), true);
        }
    }

    return module_;
}

void IRGenerator::genFunction(ASTNode* methodDecl, bool isMain) {
    module_.functions.emplace_back();
    func_ = &module_.functions.back();
    tempCounter_ = 0;
    varMap_.clear();

    if (isMain) {
        func_->name = "main";
        func_->returnType = DataType::INT;
        func_->numParams = 2; // argc, argv
    } else {
        func_->name = methodDecl->str_val;
        // First child is return type, second is formal list, third is body
        if (!methodDecl->children.empty()) {
            auto* typeNode = methodDecl->children[0].get();
            func_->returnType = DataType::UNDEFINED;
            if (typeNode->node_type == NodeType::PRIME_TYPE)
                func_->returnType = typeNode->data_type;
            else if (!typeNode->children.empty())
                func_->returnType = typeNode->children[0]->data_type;
        }
        // Parse formal parameters
        if (methodDecl->children.size() > 1 && methodDecl->children[1]) {
            ASTNode* formal = methodDecl->children[1].get();
            while (formal && (formal->node_type == NodeType::FORMAL_LIST ||
                             formal->node_type == NodeType::TYPE_ID_LIST)) {
                func_->paramNames.push_back(formal->str_val);
                DataType pType = DataType::UNDEFINED;
                if (!formal->children.empty())
                    pType = formal->children[0]->data_type;
                func_->paramTypes.push_back(pType);
                func_->numParams++;
                if (formal->children.size() > 1)
                    formal = formal->children[1].get();
                else
                    formal = nullptr;
            }
        }
    }

    curBlock_ = func_->newBlock("entry");

    // Create SSA values for parameters
    if (isMain) {
        // main gets argc in rdi, argv in rsi
        int argcVal = func_->newValue("argc", DataType::INT);
        int argvVal = func_->newValue("argv", DataType::STRING);
        writeVar("__argc", argcVal);
        writeVar("__argv", argvVal);
        // args = argv + 8 (skip argv[0] which is the program name)
        int eight = emitConst(8);
        int argsVal = freshVal(DataType::STRING);
        emit(IROpcode::ADD, argsVal, {argvVal, eight});
        writeVar("args", argsVal);
    } else {
        for (int i = 0; i < (int)func_->paramNames.size(); i++) {
            int pVal = func_->newValue(func_->paramNames[i], func_->paramTypes[i]);
            writeVar(func_->paramNames[i], pVal);
        }
    }

    // Initialize static variables at the start of main
    if (isMain) {
        for (auto* svd : staticVarDecls_) {
            genVarDecl(svd);
        }
    }

    // Generate body
    ASTNode* body = nullptr;
    if (isMain) {
        body = methodDecl->children.empty() ? nullptr : methodDecl->children[0].get();
    } else {
        // Body is the last child (statement list)
        if (methodDecl->children.size() > 2)
            body = methodDecl->children[2].get();
        else if (methodDecl->children.size() > 1)
            body = methodDecl->children[1].get();
    }

    if (body) genStatementList(body);

    // If main, add return 0
    if (isMain) {
        int zero = emitConst(0);
        emit(IROpcode::RETURN_VAL, -1, {zero});
    }
}

void IRGenerator::genStatementList(ASTNode* stmtList) {
    if (!stmtList) return;
    for (auto& child : stmtList->children) {
        genStatement(child.get());
    }
}

void IRGenerator::genStatement(ASTNode* stmt) {
    if (!stmt) return;
    switch (stmt->node_type) {
        case NodeType::STATEMENT_LIST:
            genStatementList(stmt);
            break;
        case NodeType::STATEMENT:
            for (auto& c : stmt->children) genStatement(c.get());
            break;
        case NodeType::VAR_DECL:
            genVarDecl(stmt);
            break;
        case NodeType::ASSIGN:
            genAssign(stmt);
            break;
        case NodeType::IF:
            genIf(stmt);
            break;
        case NodeType::WHILE:
            genWhile(stmt);
            break;
        case NodeType::RETURN:
            genReturn(stmt);
            break;
        case NodeType::PRINTLN:
            genPrint(stmt, true);
            break;
        case NodeType::PRINT:
            genPrint(stmt, false);
            break;
        case NodeType::STATEMENT_METHOD_CALL:
        case NodeType::EXP_METHOD_CALL:
            genMethodCall(stmt);
            break;
        case NodeType::STATIC_VAR_DECL:
            if (!stmt->children.empty())
                genVarDecl(stmt->children[0].get());
            break;
        default:
            for (auto& c : stmt->children) genStatement(c.get());
            break;
    }
}

int IRGenerator::genExpression(ASTNode* expr) {
    expr = unwrapExp(expr);
    if (!expr) return emitConst(0);

    switch (expr->node_type) {
        case NodeType::EXP_TYPE: {
            if (expr->data_type == DataType::INT)
                return emitConst(expr->int_val);
            if (expr->data_type == DataType::BOOLEAN)
                return emitConstBool(expr->bool_val);
            if (expr->data_type == DataType::STRING) {
                // Add string constant
                std::string lbl = ".Lstr" + std::to_string(module_.stringCounter++);
                module_.stringConstants.push_back({lbl, expr->str_val});
                int dst = freshVal(DataType::STRING);
                emit(IROpcode::CONST_STR, dst, {}, 0, lbl);
                return dst;
            }
            return emitConst(0);
        }
        case NodeType::EXP_LEFT: {
            if (expr->children.empty()) return emitConst(0);
            auto* lv = expr->children[0].get();
            if (lv->node_type == NodeType::LEFTVALUE_ID) {
                auto* info = findLeftValueInfo(lv);
                if (info && info->is_static) {
                    int dst = freshVal(info->type);
                    emit(IROpcode::LOAD_STATIC, dst, {}, 0, lv->str_val);
                    return dst;
                }
                return readVar(lv->str_val);
            }
            if (lv->node_type == NodeType::LEFTVALUE_ONE_ARR ||
                lv->node_type == NodeType::LEFTVALUE_TWO_ARR) {
                return genArrayAccess(lv);
            }
            return emitConst(0);
        }
        case NodeType::EXP_PLUS: {
            int left = genExpression(expr->children[0].get());
            int right = genExpression(expr->children[1].get());
            DataType lt = findExpType(expr->children[0].get());
            if (lt == DataType::STRING) {
                // String concatenation via __str_concat(str1, str2)
                emit(IROpcode::PARAM, -1, {left}, 0);
                emit(IROpcode::PARAM, -1, {right}, 1);
                int dst = freshVal(DataType::STRING);
                emit(IROpcode::CALL, dst, {left, right}, 0, "__str_concat");
                return dst;
            }
            int dst = freshVal(DataType::INT);
            emit(IROpcode::ADD, dst, {left, right});
            return dst;
        }
        case NodeType::EXP_OPERATION: {
            int left = genExpression(expr->children[0].get());
            int right = genExpression(expr->children[1].get());
            int dst = freshVal(DataType::INT);
            IROpcode op = IROpcode::SUB;
            if (expr->str_val == "-") op = IROpcode::SUB;
            else if (expr->str_val == "*") op = IROpcode::MUL;
            else if (expr->str_val == "/") op = IROpcode::DIV;
            emit(op, dst, {left, right});
            return dst;
        }
        case NodeType::EXP_OPERATION_COMP: {
            int left = genExpression(expr->children[0].get());
            int right = genExpression(expr->children[1].get());
            int dst = freshVal(DataType::BOOLEAN);
            IROpcode op = IROpcode::CMP_LT;
            if (expr->str_val == "<") op = IROpcode::CMP_LT;
            else if (expr->str_val == ">") op = IROpcode::CMP_GT;
            else if (expr->str_val == "<=") op = IROpcode::CMP_LE;
            else if (expr->str_val == ">=") op = IROpcode::CMP_GE;
            emit(op, dst, {left, right});
            return dst;
        }
        case NodeType::EXP_COMP: {
            int left = genExpression(expr->children[0].get());
            int right = genExpression(expr->children[1].get());
            int dst = freshVal(DataType::BOOLEAN);
            IROpcode op = (expr->str_val == "==") ? IROpcode::CMP_EQ : IROpcode::CMP_NE;
            emit(op, dst, {left, right});
            return dst;
        }
        case NodeType::EXP_LOGIC: {
            int left = genExpression(expr->children[0].get());
            int right = genExpression(expr->children[1].get());
            int dst = freshVal(DataType::BOOLEAN);
            IROpcode op = (expr->str_val == "&&") ? IROpcode::AND : IROpcode::OR;
            emit(op, dst, {left, right});
            return dst;
        }
        case NodeType::EXP_LOGIC_NOT: {
            int operand = genExpression(expr->children[0].get());
            int dst = freshVal(DataType::BOOLEAN);
            emit(IROpcode::NOT, dst, {operand});
            return dst;
        }
        case NodeType::EXP_NEG: {
            int operand = genExpression(expr->children[0].get());
            int dst = freshVal(DataType::INT);
            emit(IROpcode::NEG, dst, {operand});
            return dst;
        }
        case NodeType::EXP_POS: {
            return genExpression(expr->children[0].get());
        }
        case NodeType::EXP_METHOD_CALL:
            return genMethodCall(expr);
        case NodeType::EXP_ONE_ARR:
            return genArrayAlloc1D(expr);
        case NodeType::EXP_TWO_ARR:
            return genArrayAlloc2D(expr);
        case NodeType::EXP_LENGTH:
            return genArrayLength(expr);
        default:
            if (!expr->children.empty())
                return genExpression(expr->children[0].get());
            return emitConst(0);
    }
}

void IRGenerator::genVarDecl(ASTNode* decl) {
    if (!decl) return;

    DataType type = DataType::UNDEFINED;
    if (!decl->children.empty())
        type = decl->children[0]->data_type;

    auto* info = findLeftValueInfo(decl);

    // Check for initialization
    for (size_t i = 1; i < decl->children.size(); i++) {
        auto* child = decl->children[i].get();
        if (child->node_type == NodeType::INIT && !child->children.empty()) {
            int val = genExpression(child->children[0].get());
            if (info && info->is_static) {
                emit(IROpcode::STORE_STATIC, -1, {val}, 0, decl->str_val);
            } else {
                writeVar(decl->str_val, val);
            }
        } else if (child->node_type == NodeType::ID_EXP_LIST) {
            // Check if this extra var is static
            auto* extraInfo = findLeftValueInfo(child);
            if (!child->children.empty() && child->children[0]->node_type == NodeType::INIT) {
                auto* init = child->children[0].get();
                if (!init->children.empty()) {
                    int val = genExpression(init->children[0].get());
                    if (extraInfo && extraInfo->is_static) {
                        emit(IROpcode::STORE_STATIC, -1, {val}, 0, child->str_val);
                    } else {
                        writeVar(child->str_val, val);
                    }
                }
            }
        }
    }
}

void IRGenerator::genAssign(ASTNode* assign) {
    if (assign->children.size() < 2) return;
    auto* left = assign->children[0].get();
    int rhs = genExpression(assign->children[1].get());

    if (left->node_type == NodeType::LEFTVALUE_ID) {
        auto* info = findLeftValueInfo(left);
        if (info && info->is_static) {
            emit(IROpcode::STORE_STATIC, -1, {rhs}, 0, left->str_val);
        } else {
            writeVar(left->str_val, rhs);
        }
    } else if (left->node_type == NodeType::LEFTVALUE_ONE_ARR ||
               left->node_type == NodeType::LEFTVALUE_TWO_ARR) {
        genArrayStore(left, rhs);
    }
}

void IRGenerator::genIf(ASTNode* ifNode) {
    if (ifNode->children.size() < 3) return;

    int cond = genExpression(ifNode->children[0].get());

    int thenBlock = func_->newBlock("if_then");
    int elseBlock = func_->newBlock("if_else");
    int mergeBlock = func_->newBlock("if_merge");

    // Emit conditional branch
    IRInstr cbr;
    cbr.op = IROpcode::CBRANCH;
    cbr.operands = {cond};
    cbr.trueBlock = thenBlock;
    cbr.falseBlock = elseBlock;
    func_->blocks[curBlock_].instrs.push_back(cbr);
    func_->addSucc(curBlock_, thenBlock);
    func_->addSucc(curBlock_, elseBlock);

    // Save variable state before branches
    auto savedVars = varMap_;

    // Then block
    curBlock_ = thenBlock;
    genStatement(ifNode->children[1].get());
    int thenEndBlock = curBlock_;  // Actual final block after then processing
    // Branch to merge
    bool thenReturned = false;
    if (!func_->blocks[curBlock_].instrs.empty() &&
        func_->blocks[curBlock_].instrs.back().op == IROpcode::RETURN_VAL) {
        thenReturned = true;
    } else {
        IRInstr br;
        br.op = IROpcode::BRANCH;
        br.targetBlock = mergeBlock;
        func_->blocks[curBlock_].instrs.push_back(br);
        func_->addSucc(curBlock_, mergeBlock);
    }
    auto thenVars = varMap_;

    // Else block
    varMap_ = savedVars;
    curBlock_ = elseBlock;
    genStatement(ifNode->children[2].get());
    int elseEndBlock = curBlock_;  // Actual final block after else processing
    bool elseReturned = false;
    if (!func_->blocks[curBlock_].instrs.empty() &&
        func_->blocks[curBlock_].instrs.back().op == IROpcode::RETURN_VAL) {
        elseReturned = true;
    } else {
        IRInstr br;
        br.op = IROpcode::BRANCH;
        br.targetBlock = mergeBlock;
        func_->blocks[curBlock_].instrs.push_back(br);
        func_->addSucc(curBlock_, mergeBlock);
    }
    auto elseVars = varMap_;

    // Merge block - insert PHI nodes for variables that differ
    curBlock_ = mergeBlock;
    if (!thenReturned || !elseReturned) {
        for (auto& [name, thenVal] : thenVars) {
            auto elseIt = elseVars.find(name);
            if (elseIt != elseVars.end() && elseIt->second != thenVal) {
                // Insert PHI using actual end blocks as predecessors
                int phi = freshVal(func_->values[thenVal].type);
                IRInstr phiInstr;
                phiInstr.op = IROpcode::PHI;
                phiInstr.dst = phi;
                phiInstr.operands = {thenVal, elseIt->second};
                phiInstr.phiBlocks = {thenEndBlock, elseEndBlock};
                func_->blocks[curBlock_].instrs.push_back(phiInstr);
                varMap_[name] = phi;
            } else {
                varMap_[name] = thenVal;
            }
        }
        // Also pick up vars only in else
        for (auto& [name, elseVal] : elseVars) {
            if (thenVars.find(name) == thenVars.end())
                varMap_[name] = elseVal;
        }
    }
}

void IRGenerator::genWhile(ASTNode* whileNode) {
    if (whileNode->children.size() < 2) return;

    int preBlock = curBlock_;
    auto preMap = varMap_;  // Snapshot before loop

    int headerBlock = func_->newBlock("while_header");
    int bodyBlock = func_->newBlock("while_body");
    int exitBlock = func_->newBlock("while_exit");

    // Branch to header
    IRInstr br;
    br.op = IROpcode::BRANCH;
    br.targetBlock = headerBlock;
    func_->blocks[curBlock_].instrs.push_back(br);
    func_->addSucc(curBlock_, headerBlock);

    // Find which variables are modified in the loop body/condition
    curBlock_ = headerBlock;
    std::set<std::string> modifiedVars;
    collectModifiedVars(whileNode->children[1].get(), modifiedVars);

    struct PhiInfo {
        std::string varName;
        int phiId;
        int preVal;
        size_t instrIdx;
    };
    std::vector<PhiInfo> phis;

    // Only insert PHI nodes for variables that are modified in the loop
    for (auto& [name, preVal] : preMap) {
        if (modifiedVars.find(name) == modifiedVars.end()) continue;
        int phi = freshVal(func_->values[preVal].type);
        IRInstr phiInstr;
        phiInstr.op = IROpcode::PHI;
        phiInstr.dst = phi;
        phiInstr.operands = {preVal};  // From pre-loop
        phiInstr.phiBlocks = {preBlock};
        size_t idx = func_->blocks[headerBlock].instrs.size();
        func_->blocks[headerBlock].instrs.push_back(phiInstr);
        varMap_[name] = phi;
        phis.push_back({name, phi, preVal, idx});
    }

    // Evaluate condition (uses PHI values)
    int cond = genExpression(whileNode->children[0].get());

    IRInstr cbr;
    cbr.op = IROpcode::CBRANCH;
    cbr.operands = {cond};
    cbr.trueBlock = bodyBlock;
    cbr.falseBlock = exitBlock;
    func_->blocks[curBlock_].instrs.push_back(cbr);
    func_->addSucc(curBlock_, bodyBlock);
    func_->addSucc(curBlock_, exitBlock);

    // Body
    curBlock_ = bodyBlock;
    genStatement(whileNode->children[1].get());

    int bodyEndBlock = curBlock_;

    // Complete PHI nodes with back-edge values
    for (auto& phi : phis) {
        int bodyVal = varMap_[phi.varName];
        auto& phiInstr = func_->blocks[headerBlock].instrs[phi.instrIdx];
        phiInstr.operands.push_back(bodyVal);
        phiInstr.phiBlocks.push_back(bodyEndBlock);
    }

    // Loop back to header
    IRInstr brBack;
    brBack.op = IROpcode::BRANCH;
    brBack.targetBlock = headerBlock;
    func_->blocks[curBlock_].instrs.push_back(brBack);
    func_->addSucc(curBlock_, headerBlock);

    // After the loop, variables have their PHI values (from the header)
    curBlock_ = exitBlock;
    for (auto& phi : phis) {
        varMap_[phi.varName] = phi.phiId;
    }
}

void IRGenerator::genReturn(ASTNode* retNode) {
    if (retNode->children.empty()) return;
    int val = genExpression(retNode->children[0].get());
    emit(IROpcode::RETURN_VAL, -1, {val});
}

void IRGenerator::genPrint(ASTNode* printNode, bool newline) {
    if (printNode->children.empty()) return;
    ASTNode* expr = unwrapExp(printNode->children[0].get());
    DataType type = findExpType(expr);
    int val = genExpression(expr);

    if (type == DataType::STRING || (expr && expr->node_type == NodeType::EXP_TYPE && expr->data_type == DataType::STRING)) {
        emit(IROpcode::PRINT_STR, -1, {val});
    } else {
        emit(IROpcode::PRINT_INT, -1, {val});
    }
    if (newline) {
        emit(IROpcode::PRINT_NEWLINE, -1, {});
    }
}

int IRGenerator::genMethodCall(ASTNode* callNode) {
    ASTNode* inner = callNode;
    if (callNode->node_type == NodeType::EXP_METHOD_CALL ||
        callNode->node_type == NodeType::STATEMENT_METHOD_CALL) {
        if (callNode->children.empty()) return emitConst(0);
        inner = callNode->children[0].get();
    }

    // Integer.parseInt
    if (inner->node_type == NodeType::PARSE_INT) {
        if (inner->children.empty()) return emitConst(0);
        int arg = genExpression(inner->children[0].get());
        int dst = freshVal(DataType::INT);
        emit(IROpcode::CALL, dst, {arg}, 0, "atoi");
        return dst;
    }

    // Regular method call or main call
    std::string funcName = inner->str_val;
    std::vector<int> args;

    // Process arguments
    if (!inner->children.empty()) {
        auto* argList = inner->children[0].get();
        if (argList->node_type == NodeType::COMMA_EXP_LIST) {
            for (auto& arg : argList->children) {
                args.push_back(genExpression(arg.get()));
            }
        } else {
            args.push_back(genExpression(argList));
        }
    }

    // Emit PARAM instructions for each argument
    for (int i = 0; i < (int)args.size(); i++) {
        emit(IROpcode::PARAM, -1, {args[i]}, i);
    }

    int dst = freshVal(DataType::INT);
    emit(IROpcode::CALL, dst, args, 0, funcName);
    return dst;
}

int IRGenerator::genArrayAlloc1D(ASTNode* node) {
    // new Type[size]
    if (node->children.size() < 2) return emitConst(0);
    int size = genExpression(node->children[1].get());
    int dst = freshVal(DataType::INT);
    emit(IROpcode::ALLOC_ARRAY, dst, {size});
    return dst;
}

int IRGenerator::genArrayAlloc2D(ASTNode* node) {
    // new Type[rows][cols]
    if (node->children.size() < 3) return emitConst(0);
    int rows = genExpression(node->children[1].get());
    int cols = genExpression(node->children[2].get());

    // Allocate the row pointer array
    int rowArr = freshVal(DataType::INT);
    emit(IROpcode::ALLOC_ARRAY, rowArr, {rows});

    // Loop: allocate each row (with PHI for counter)
    int counterInit = emitConst(0);
    int preBlock = curBlock_;
    int headerBlock = func_->newBlock("alloc2d_header");
    int bodyBlock = func_->newBlock("alloc2d_body");
    int exitBlock = func_->newBlock("alloc2d_exit");

    IRInstr br;
    br.op = IROpcode::BRANCH;
    br.targetBlock = headerBlock;
    func_->blocks[curBlock_].instrs.push_back(br);
    func_->addSucc(curBlock_, headerBlock);

    // Header: PHI for counter, then check counter < rows
    curBlock_ = headerBlock;
    int counterPhi = freshVal(DataType::INT);
    IRInstr phiInstr;
    phiInstr.op = IROpcode::PHI;
    phiInstr.dst = counterPhi;
    phiInstr.operands = {counterInit};
    phiInstr.phiBlocks = {preBlock};
    size_t phiIdx = func_->blocks[headerBlock].instrs.size();
    func_->blocks[headerBlock].instrs.push_back(phiInstr);

    int cmpVal = freshVal(DataType::BOOLEAN);
    emit(IROpcode::CMP_LT, cmpVal, {counterPhi, rows});

    IRInstr cbr;
    cbr.op = IROpcode::CBRANCH;
    cbr.operands = {cmpVal};
    cbr.trueBlock = bodyBlock;
    cbr.falseBlock = exitBlock;
    func_->blocks[curBlock_].instrs.push_back(cbr);
    func_->addSucc(curBlock_, bodyBlock);
    func_->addSucc(curBlock_, exitBlock);

    // Body: allocate one row and store it
    curBlock_ = bodyBlock;
    int rowAlloc = freshVal(DataType::INT);
    emit(IROpcode::ALLOC_ARRAY, rowAlloc, {cols});
    emit(IROpcode::ARRAY_STORE, -1, {rowArr, counterPhi, rowAlloc});

    // counter++
    int one = emitConst(1);
    int newCnt = freshVal(DataType::INT);
    emit(IROpcode::ADD, newCnt, {counterPhi, one});

    // Complete PHI with back-edge value
    auto& phi = func_->blocks[headerBlock].instrs[phiIdx];
    phi.operands.push_back(newCnt);
    phi.phiBlocks.push_back(curBlock_);

    IRInstr brBack;
    brBack.op = IROpcode::BRANCH;
    brBack.targetBlock = headerBlock;
    func_->blocks[curBlock_].instrs.push_back(brBack);
    func_->addSucc(curBlock_, headerBlock);

    curBlock_ = exitBlock;
    return rowArr;
}

int IRGenerator::genArrayAccess(ASTNode* leftVal) {
    std::string arrName = leftVal->str_val;
    int base = readVar(arrName);

    if (leftVal->node_type == NodeType::LEFTVALUE_ONE_ARR) {
        int index = genExpression(leftVal->children[0].get());
        int dst = freshVal(DataType::INT);
        emit(IROpcode::ARRAY_LOAD, dst, {base, index});
        return dst;
    }
    if (leftVal->node_type == NodeType::LEFTVALUE_TWO_ARR) {
        int index1 = genExpression(leftVal->children[0].get());
        // Load row pointer
        int rowPtr = freshVal(DataType::INT);
        emit(IROpcode::ARRAY_LOAD, rowPtr, {base, index1});
        int index2 = genExpression(leftVal->children[1].get());
        int dst = freshVal(DataType::INT);
        emit(IROpcode::ARRAY_LOAD, dst, {rowPtr, index2});
        return dst;
    }
    return emitConst(0);
}

void IRGenerator::genArrayStore(ASTNode* leftVal, int valueSSA) {
    std::string arrName = leftVal->str_val;
    int base = readVar(arrName);

    if (leftVal->node_type == NodeType::LEFTVALUE_ONE_ARR) {
        int index = genExpression(leftVal->children[0].get());
        emit(IROpcode::ARRAY_STORE, -1, {base, index, valueSSA});
    } else if (leftVal->node_type == NodeType::LEFTVALUE_TWO_ARR) {
        int index1 = genExpression(leftVal->children[0].get());
        int rowPtr = freshVal(DataType::INT);
        emit(IROpcode::ARRAY_LOAD, rowPtr, {base, index1});
        int index2 = genExpression(leftVal->children[1].get());
        emit(IROpcode::ARRAY_STORE, -1, {rowPtr, index2, valueSSA});
    }
}

int IRGenerator::genArrayLength(ASTNode* node) {
    if (node->children.empty()) return emitConst(0);
    auto* lv = node->children[0].get();
    int base = readVar(lv->str_val);

    if (lv->node_type == NodeType::LEFTVALUE_ONE_ARR) {
        // Load the row first, then get its length
        int index = genExpression(lv->children[0].get());
        int rowPtr = freshVal(DataType::INT);
        emit(IROpcode::ARRAY_LOAD, rowPtr, {base, index});
        int len = freshVal(DataType::INT);
        emit(IROpcode::ARRAY_LENGTH, len, {rowPtr});
        return len;
    }

    int dst = freshVal(DataType::INT);
    emit(IROpcode::ARRAY_LENGTH, dst, {base});
    return dst;
}
