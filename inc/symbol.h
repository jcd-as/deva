// Copyright (c) 2010 Joshua C. Shepard
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

// symbol.h
// symbol type for the deva language
// created by jcs, december 09, 2010 

// TODO:
// * 

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include "type.h"
#include <string>

using namespace std;


namespace deva_compile
{

// base class for symbols
class SymbolBase
{
protected:
	string name;
	SymbolType type;

public:
	SymbolBase() : type( sym_end ) {}
	SymbolBase( SymbolType t ) : type( t ) {}
	SymbolBase( const char* const nm, SymbolType t ) : name( nm ), type( t ) {}

	const string & Name() const { return name; }
	const SymbolType Type() const { return type; }

	bool operator < ( const SymbolBase & rhs ) const { return name < rhs.name; }
};

// functor for comparing SymbolBase ptrs
struct SB_ptr_lt
{
	bool operator()( const SymbolBase* lhs, const SymbolBase* rhs ){ return lhs->operator < (*rhs); }
};

enum VariableModifier
{
	mod_none,
	mod_constant,	// 'const'
	mod_external,	// 'extern'
	mod_local,		// 'local'
	mod_arg,			// function argument. equivalent to local in most ways
	mod_module_name		// the name of a module for import
};

// variable symbol, has modifier
class Symbol : public SymbolBase
{
protected:
	VariableModifier modifier;

public:
	Symbol() : SymbolBase( sym_end ), modifier( mod_none ) {}
	Symbol( SymbolType t ) : SymbolBase( t ), modifier( mod_none ) {}
	Symbol( SymbolType t, VariableModifier m ) : SymbolBase( t ), modifier( m ) {}
	Symbol( const char* const n, SymbolType t, VariableModifier m = mod_none ) : SymbolBase( n, t ), modifier( m ) {}

	VariableModifier Modifier() const { return modifier; }
	bool IsConst() const { return modifier == mod_constant; }
	bool IsExtern() const { return modifier == mod_external; }
	bool IsLocal() const { return modifier == mod_local; }
	bool IsArg() const { return modifier == mod_arg; }
	bool IsUndeclared() const { return modifier == mod_none; }
	bool IsFunction() const { return (type == sym_function || type == sym_method); }
};


} // namespace deva_compile

#endif // __SYMBOL_H__
