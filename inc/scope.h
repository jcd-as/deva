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
#include "object.h"
#include "ordered_set.h"
#include <string>
#include <map>
#include <set>


using namespace std;
using namespace deva;

namespace deva_compile
{

class FunctionScope;

struct Scope
{
	virtual const string & Name() const = 0;
	// return the *accessible* parent scope, NULL if none
	virtual Scope* EnclosingScope() const = 0;
	// define a new symbol, returns false if symbol already exists
	virtual bool Define( const Symbol* const s ) = 0;
	// resolve a symbol, returns NULL if cannot be found in this scope
	virtual const Symbol* const Resolve( const string & name, SymbolType type = sym_end ) const = 0;
	// resolve a local to an index
	virtual int ResolveLocalToIndex( const string & name ) = 0;
	// print the scope to stdout
	virtual void Print() = 0;
	// add to the parent function's list of names
	virtual void AddName( Symbol* s ) = 0;
	// set a local as having been defined in code-gen
	virtual void SetLocalGenerated( int idx ) = 0;
	// has a local been defined yet?
	virtual bool HasLocalBeenGenerated( int idx ) = 0;

	virtual FunctionScope* getParentFun() = 0;

	virtual ~Scope(){}
};

class LocalScope : public Scope
{
protected:
	string name;
	// all names in this scope
	map<const string, Symbol*> data;
	// locals only, int is the index into the parent function scope's locals array
	map<const string, int> local_map;
	Scope *parent;

	// locals that are defined (at any point in time) in the current scope
	// (used at code-gen)
	set<int> generated_locals;

	// helper fcns
	virtual FunctionScope* getParentFun();
	// resolve a local to an index IN THIS SCOPE ONLY
	int ResolveLocalToIndexHelper( const string & name );

public:
	LocalScope( string n, Scope* p = NULL ) : name( n ), parent( p ) { }
	virtual ~LocalScope();

	// Scope "interface"
	const string & Name() const;
	Scope* EnclosingScope() const;
	bool Define( const Symbol* const  s );
	const Symbol* const Resolve( const string & name, SymbolType type = sym_end ) const;
	// resolve a local to an index
	int ResolveLocalToIndex( const string & name );
	// print the scope to stdout
	void Print();
	// add to the parent function's list of names
	virtual void AddName( Symbol* s );
	// set a local as having been defined in code-gen
	void SetLocalGenerated( int idx ){ generated_locals.insert( idx ); }
	// has a local been generated yet?
	bool HasLocalBeenGenerated( int idx );
};


class FunctionScope : public LocalScope
{
protected:
	bool isMethod;
	int numArgs;

	// all the names, local, external, functions or undeclared, used in the
	// function. (names can be duplicated, e.g. locals in different scopes with the same
	// name)
	OrderedMultiSet<Symbol*, SB_ptr_lt> names;

	// locals (incl args), for determining local indices
	// stored in a first-in order (so that arguments are at the beginning)
	vector<string> locals;

	// default argument values
	OrderedSet<Object> default_arg_values;

	// helper fcn
	virtual FunctionScope* getParentFun();

public:
	FunctionScope( string n, Scope* p = NULL, bool m = false ) : LocalScope( n, p ), 
		isMethod( m ), numArgs( 0 ) {}
	virtual ~FunctionScope() {}

	// Scope "interface" overrides
	const Symbol* const Resolve( const string & name, SymbolType type = sym_end ) const;
	int ResolveLocalToIndex( const string & name );
	bool Define( const Symbol* const  s );
	void Print();
	inline bool HasLocalBeenGenerated( int idx ) { return (generated_locals.find( idx ) != generated_locals.end()); }

	// function scope methods
	inline const int NumArgs() const { return numArgs; }
	inline const int NumLocals() const { return (int)locals.size(); }
	inline OrderedMultiSet<Symbol*, SB_ptr_lt> & GetNames() { return names; }
	inline OrderedSet<Object> & GetDefaultArgVals() { return default_arg_values; }
	inline vector<string> & GetLocals() { return locals; }
	inline bool IsMethod(){ return isMethod; }
	// add to the parent function's list of names
	virtual void AddName( Symbol* s );
	// add a defalt arg value
	inline void AddDefaultArgVal( Object o ) { default_arg_values.Add( o ); }
};

} // namespace deva_compile

#endif // __SCOPE_H__
