// scope.h
// deva language grammar for expressions
// created by jcs, september 10, 2009 

// TODO:
// * 

#ifndef __SCOPE_H__
#define __SCOPE_H__

#include <map>
#include <vector>
#include <utility>
#include "symbol.h"

using namespace std;

// type for the global scope table
typedef map<int, SymbolTable*> Scopes;

// type for the scope-building stack (used to build the global scope table)
typedef vector<pair<int, SymbolTable*> > ScopeBuilder;

// utility function to locate a symbol in parent scopes
extern bool find_identifier_in_parent_scopes( string id, SymbolTable* sym, Scopes s );

// locate a symbol
extern SymbolInfo find_symbol( string id, SymbolTable* sym, Scopes s );

// find a symbol and set its 'const-ness'
extern bool set_symbol_constness( string id, SymbolTable* symtab, Scopes s, bool constness );

#endif // __SCOPE_H__
