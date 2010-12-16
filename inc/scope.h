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

// scope.h
// scope types for the deva language
// created by jcs, december 09, 2010 

// TODO:
// * 

#ifndef __SCOPE_H__
#define __SCOPE_H__

#include "symbol.h"
#include <string>
#include <map>

using namespace std;


struct Scope
{
	virtual const string & Name() const = 0;
	// return the *accessible* parent scope, NULL if none
	virtual Scope* EnclosingScope() const = 0;
	// define a new symbol, returns false if symbol already exists
	virtual bool Define( const Symbol* const s ) = 0;
	// resolve a symbol, returns NULL if cannot be found in this scope
	virtual const Symbol* const Resolve( const string & name, SymbolType type = sym_end ) const = 0;

	// print the scope to stdout
	virtual void Print() = 0;
	// add to the parent function's list of names
	virtual void AddName( Symbol* s ) = 0;

	virtual ~Scope(){}
};


class LocalScope : public Scope
{
protected:
	string name;
	map<const string, Symbol*> data;
	Scope *parent;

public:
	LocalScope() : name( "" ), parent( NULL ) {}
	LocalScope( string n, Scope* p = NULL ) : name( n ), parent( p ) {}
	virtual ~LocalScope();

	// Scope "interface"
	const string & Name() const;
	Scope* EnclosingScope() const;
	bool Define( const Symbol* const  s );
	const Symbol* const Resolve( const string & name, SymbolType type = sym_end ) const;
	void Print();
	// add to the parent function's list of names
	virtual void AddName( Symbol* s );
};


class FunctionScope : public LocalScope
{
protected:
	bool isMethod;
	int numArgs;
	int numLocals;

	// all the names, local, external, functions or undeclared, used in the
	// function
	map<const string, Symbol*> names;

public:
	FunctionScope() : LocalScope(), isMethod( false ), numArgs( 0 ), numLocals( 0 ) {}
	FunctionScope( string n, Scope* p = NULL, bool m = false ) : LocalScope( n, p ), 
		isMethod( m ), numArgs( 0 ), numLocals( 0 ) {}
	virtual ~FunctionScope() {}

	// Scope "interface" overrides
	const Symbol* const Resolve( const string & name, SymbolType type = sym_end ) const;
	bool Define( const Symbol* const  s );
	void Print();

	const int NumArgs() const { return numArgs; }
	const int NumLocals() const { return numLocals; }
	map<const string, Symbol*> GetNames() { return names; }

	// add to the parent function's list of names
	virtual void AddName( Symbol* s );
	// add one to the count of locals
//	virtual void IncrementFunLocals(){ numLocals++; }
};


#endif // __SCOPE_H__
