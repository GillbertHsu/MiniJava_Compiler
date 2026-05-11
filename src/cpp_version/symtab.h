#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

struct SymbolInfo {
    std::string name;
    DataType type = DataType::UNDEFINED;
    bool is_method = false;
    bool is_static = false;
    bool is_arg = false;
    int num_params = 0;
    int array_dim = 0;   // 0=scalar, 1=1D, 2=2D
    int stack_slot = -1;
    SymbolTable* method_scope = nullptr;
};

class SymbolTable {
public:
    explicit SymbolTable(SymbolTable* parent = nullptr);

    bool insert(const std::string& name, SymbolInfo info);
    SymbolInfo* lookupLocal(const std::string& name);
    SymbolInfo* lookup(const std::string& name);
    SymbolTable* parent() const { return parent_; }
    const std::unordered_map<std::string, SymbolInfo>& entries() const { return entries_; }
    int size() const { return (int)entries_.size(); }

private:
    std::unordered_map<std::string, SymbolInfo> entries_;
    SymbolTable* parent_;
};

class MethodTable {
public:
    bool insert(const std::string& name, SymbolInfo info);
    SymbolInfo* lookup(const std::string& name);
    const std::unordered_map<std::string, SymbolInfo>& entries() const { return methods_; }
private:
    std::unordered_map<std::string, SymbolInfo> methods_;
};

#endif
