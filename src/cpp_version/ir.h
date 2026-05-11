#ifndef IR_H
#define IR_H

#include "ast.h"
#include "symtab.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

enum class IROpcode {
    CONST_INT, CONST_BOOL, CONST_STR,
    ADD, SUB, MUL, DIV, NEG,
    CMP_LT, CMP_GT, CMP_LE, CMP_GE, CMP_EQ, CMP_NE,
    AND, OR, NOT,
    COPY, LOAD_STATIC, STORE_STATIC,
    ALLOC_ARRAY, ARRAY_LOAD, ARRAY_STORE, ARRAY_LENGTH,
    BRANCH, CBRANCH, PHI,
    PARAM, CALL, RETURN_VAL,
    PRINT_INT, PRINT_STR, PRINT_NEWLINE,
    NOP,
};

struct IRInstr {
    IROpcode op = IROpcode::NOP;
    int dst = -1;
    std::vector<int> operands;
    int64_t imm = 0;
    std::string label;
    std::vector<int> phiBlocks;
    int trueBlock = -1;
    int falseBlock = -1;
    int targetBlock = -1;
};

struct BasicBlock {
    int id = -1;
    std::string label;
    std::vector<IRInstr> instrs;
    std::vector<int> preds;
    std::vector<int> succs;
};

struct SSAValue {
    int id = -1;
    std::string name;
    DataType type = DataType::UNDEFINED;
    bool isSpilled = false;
};

struct IRFunction {
    std::string name;
    DataType returnType = DataType::UNDEFINED;
    std::vector<std::string> paramNames;
    std::vector<DataType> paramTypes;
    int numParams = 0;

    std::vector<BasicBlock> blocks;
    std::vector<SSAValue> values;

    int entryBlock = 0;

    int newValue(const std::string& name, DataType type);
    int newBlock(const std::string& label);
    void addSucc(int from, int to);
};

struct IRModule {
    std::vector<IRFunction> functions;
    std::vector<std::pair<std::string, std::string>> stringConstants;
    std::vector<std::string> staticVars;
    int stringCounter = 0;
};

class IRGenerator {
public:
    IRModule generate(ASTNode* programRoot, MethodTable& methods);

private:
    IRModule module_;
    MethodTable* methods_ = nullptr;
    IRFunction* func_ = nullptr;
    int curBlock_ = -1;
    int tempCounter_ = 0;

    std::unordered_map<std::string, int> varMap_;
    std::vector<ASTNode*> staticVarDecls_;

    int emit(IROpcode op, int dst, std::vector<int> operands, int64_t imm = 0,
             const std::string& label = "");
    int emitConst(int64_t val);
    int emitConstBool(bool val);
    int emitCopy(int src);
    int freshVal(DataType type = DataType::INT);

    void genFunction(ASTNode* methodDecl, bool isMain);
    void genStatementList(ASTNode* stmtList);
    void genStatement(ASTNode* stmt);
    int  genExpression(ASTNode* expr);
    void genVarDecl(ASTNode* decl);
    void genAssign(ASTNode* assign);
    void genIf(ASTNode* ifNode);
    void genWhile(ASTNode* whileNode);
    void genReturn(ASTNode* retNode);
    void genPrint(ASTNode* printNode, bool newline);
    int  genMethodCall(ASTNode* callNode);
    int  genArrayAlloc1D(ASTNode* node);
    int  genArrayAlloc2D(ASTNode* node);
    int  genArrayAccess(ASTNode* leftVal);
    void genArrayStore(ASTNode* leftVal, int valueSSA);
    int  genArrayLength(ASTNode* node);

    void writeVar(const std::string& name, int ssaVal);
    int  readVar(const std::string& name);

    DataType findExpType(ASTNode* expr);
    ASTNode* unwrapExp(ASTNode* node);
    SymbolInfo* findLeftValueInfo(ASTNode* node);
};

#endif
