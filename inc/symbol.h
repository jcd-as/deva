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

enum VariableModifier
{
	mod_none,
	mod_constant,	// 'const'
	mod_external,	// 'extern'
	mod_local		// 'local'
};

class SymbolInfo
{
protected:
	SymbolType type;
	VariableModifier modifier;

public:
	SymbolInfo() : type( sym_end ), modifier( mod_none )
	{}
	SymbolInfo( SymbolType t ) : type( t ), modifier( mod_none )
	{}
	SymbolInfo( SymbolType t, VariableModifier m ) : type( t ), modifier( m )
	{}

	SymbolType Type() const { return type; }
	bool IsConst() const { return modifier == mod_constant; }
	bool IsExtern() const { return modifier == mod_external; }
	bool IsLocal() const { return modifier == mod_local; }
};

class Symbol : public SymbolInfo
{
protected:
	string name;

public:
	Symbol() : name( "" )
	{}
	Symbol( const char* const n ) : name( n )
	{}
	Symbol( SymbolType t, const char* const n ) : SymbolInfo( t ), name( n )
	{}
	Symbol( SymbolType t, VariableModifier m, const char* const n ) : SymbolInfo( t, m ), name( n )
	{}

	const string & Name() const { return name; }
};

#endif // __SYMBOL_H__
