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

// scope.cpp
// scope types for the deva language
// created by jcs, december 09, 2010 

// TODO:
// * 

#include "scope.h"
#include <iostream>

/////////////////////////////////////////////////////////////////////////////
// LocalScope class methods:
/////////////////////////////////////////////////////////////////////////////

const string & LocalScope::Name() const
{
	return name;
}

Scope* LocalScope::EnclosingScope() const
{
	return parent;
}

bool LocalScope::Define( const Symbol* const s )
{
	if( data.count( s->Name() ) != 0 )
		return false;
	else
		data[s->Name()] = const_cast<Symbol*>( s );

	// add this name to the parent function's list of names
	AddName( const_cast<Symbol*>(s) );

	return true;
}

const Symbol* const LocalScope::Resolve( const string & n, SymbolType type ) const
{
	// external vars can't be actually resolved until runtime, all we can do is
	// verify that they have been declared

	// check this scope
	if( data.count( n ) != 0 )
		return data.find( n )->second;
	// check parent scopes
	if( parent )
		return parent->Resolve( n, type );
	return NULL;
}

void LocalScope::Print()
{
	cout << "Scope: " << name << endl << "\tvars: ";
	for( map<string, Symbol*>::iterator i = data.begin(); i != data.end(); ++i )
	{
		const char* mod;
		if( i->second->IsConst() ) mod = "const";
		else if( i->second->IsLocal() ) mod = "local";
		else if( i->second->IsExtern() ) mod = "extern";
		else if( i->second->IsArg() ) mod = "argument";
		else if( i->second->Type() == sym_function ) mod = "function";
		else mod = "undeclared";
		cout << mod << " " << i->first << "; ";
	}
	cout << endl;
}

LocalScope::~LocalScope()
{
	// delete the symbol ptrs in the map
	for( map<string, Symbol*>::iterator i = data.begin(); i != data.end(); ++i )
	{
		delete i->second;
	}
}

// add to the parent function's list of names
void LocalScope::AddName( Symbol* s )
{
	if( parent )
		parent->AddName( s );
}


/////////////////////////////////////////////////////////////////////////////
// FunctionScope class methods:
/////////////////////////////////////////////////////////////////////////////

const Symbol* const FunctionScope::Resolve( const string & n, SymbolType type ) const
{
	// external vars can't be actually resolved until runtime, all we can do is
	// verify that they have been declared

	// check this scope
	if( data.count( n ) != 0 )
		return data.find( n )->second;
	// function scope, only look for functions and classes in parent scopes
	if( type == sym_function || type == sym_class || type == sym_end )
	{
		// check parent scopes
		if( parent )
		{
			const Symbol* s = parent->Resolve( n, sym_function );
			if( (s && type == sym_end) && (s->Type() == sym_function || s->Type() == sym_class) )
				return s;
			else
				return s;
		}
	}
	return NULL;
}

bool FunctionScope::Define( const Symbol* const  s )
{
	// call the base-class
	bool ret = LocalScope::Define( s );

	if( ret )
	{
		// if this is an arg, increment the arg counter
		if( s->IsArg() )
			numArgs++;
	}
	return ret;
}

void FunctionScope::Print()
{
	cout << "Function: " << name << ", " << numArgs << " arguments, " << numLocals << " locals" << endl << "\tall names: ";
	for( map<string, Symbol*>::iterator i = names.begin(); i != names.end(); ++i )
	{
		const char* mod;
		if( i->second->IsConst() ) mod = "const";
		else if( i->second->IsLocal() ) mod = "local";
		else if( i->second->IsExtern() ) mod = "extern";
		else if( i->second->IsArg() ) mod = "argument";
		else if( i->second->Type() == sym_function ) mod = "function";
		else mod = "undeclared";
		cout << mod << " " << i->first << "; ";
	}
	cout << endl << "\tlocal scope vars: ";
	for( map<string, Symbol*>::iterator i = data.begin(); i != data.end(); ++i )
	{
		const char* mod;
		if( i->second->IsConst() ) mod = "const";
		else if( i->second->IsLocal() ) mod = "local";
		else if( i->second->IsExtern() ) mod = "extern";
		else if( i->second->IsArg() ) mod = "argument";
		else if( i->second->Type() == sym_function ) mod = "function";
		else mod = "undeclared";
		cout << mod << " " << i->first << "; ";
	}
	cout << endl;
}

// add to the parent function's list of names
void FunctionScope::AddName( Symbol* s )
{
	if( s->IsLocal() )
		numLocals++;
	names.insert( pair<const string, Symbol*>(s->Name(), s) );
}


