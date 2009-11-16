// Copyright (c) 2009 Joshua C. Shepard
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

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
