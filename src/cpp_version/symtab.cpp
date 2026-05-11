#include "symtab.h"

SymbolTable::SymbolTable(SymbolTable* parent) : parent_(parent) {}

bool SymbolTable::insert(const std::string& name, SymbolInfo info) {
    if (entries_.count(name)) return false;
    entries_[name] = std::move(info);
    return true;
}

SymbolInfo* SymbolTable::lookupLocal(const std::string& name) {
    auto it = entries_.find(name);
    if (it != entries_.end()) return &it->second;
    return nullptr;
}

SymbolInfo* SymbolTable::lookup(const std::string& name) {
    auto* info = lookupLocal(name);
    if (info) return info;
    if (parent_) return parent_->lookup(name);
    return nullptr;
}

bool MethodTable::insert(const std::string& name, SymbolInfo info) {
    if (methods_.count(name)) return false;
    methods_[name] = std::move(info);
    return true;
}

SymbolInfo* MethodTable::lookup(const std::string& name) {
    auto it = methods_.find(name);
    if (it != methods_.end()) return &it->second;
    return nullptr;
}
