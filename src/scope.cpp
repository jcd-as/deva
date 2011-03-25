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


#include "scope.h"
#include <iostream>


namespace deva_compile
{

/////////////////////////////////////////////////////////////////////////////
// LocalScope class methods:
/////////////////////////////////////////////////////////////////////////////

// helper fcn
FunctionScope* LocalScope::getParentFun()
{
	if( parent )
		return parent->getParentFun();
	return NULL;
}

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
	// if it's a local, check with the locals for this scope
	if( s->IsLocal() || s->IsArg() )
	{
		map<const string, int>::iterator i = local_map.find( s->Name() );
		if( i != local_map.end() )
			return false;
	}
	// disallow re-def of consts
	else if( s->IsConst() )
	{
		if( data.count( s->Name() ) != 0 )
			return false;
	}

	// data is a map, repeated items won't be added
	// and will leak if 'false' isn't returned
	if( data.count( s->Name() ) != 0 )
		return false;

	// ensure it's added to the list of names for this scope
	data[s->Name()] = const_cast<Symbol*>( s );

	// add this name to the parent function's list of names
	AddName( const_cast<Symbol*>(s) );

	// is this a local? add it to this scope and to the fcn scope
	if( s->IsLocal() || s->IsConst() || s->IsArg() )
	{
		FunctionScope* fun = getParentFun();
		fun->GetLocals().push_back( string( s->Name() ) );
		int idx = fun->GetLocals().size() -1;
		local_map.insert( pair<string, int>( s->Name(), idx ) );
	}

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

// resolve a local to an index IN THIS SCOPE ONLY
int LocalScope::ResolveLocalToIndexHelper( const string & name )
{
	map<const string, int>::iterator i = local_map.find( name );
	int idx = -1;
	if( i != local_map.end() )
		idx = i->second;
	return idx;
}

// resolve a local to an index
int LocalScope::ResolveLocalToIndex( const string & name )
{
	// look in this scope first
	int idx = ResolveLocalToIndexHelper( name );
	if( idx != -1 )
		return idx;

	// not found? pass through to the parent scope
	if( parent )
		return parent->ResolveLocalToIndex( name );
	else
		return -1;
}

void LocalScope::Print()
{
	cout << "Scope: " << name << endl << "\tvars: ";
	for( map<const string, Symbol*>::iterator i = data.begin(); i != data.end(); ++i )
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
	for( map<const string, Symbol*>::iterator i = data.begin(); i != data.end(); ++i )
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

bool LocalScope::HasLocalBeenGenerated( int idx )
{
	bool generated = (generated_locals.find( idx ) != generated_locals.end());
	// if we didn't find it in this scope, try parent scopes
	if( !generated )
	{
		if( parent )
			return parent->HasLocalBeenGenerated( idx );
		else
			return generated;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// FunctionScope class methods:
/////////////////////////////////////////////////////////////////////////////

// helper fcn
FunctionScope* FunctionScope::getParentFun()
{
	return this;
}

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

// resolve a local to an index
int FunctionScope::ResolveLocalToIndex( const string & name )
{
	// call base class fcn
	// do NOT pass any further up the scope chain 
	// (we're looking for a _local_!)
	return LocalScope::ResolveLocalToIndexHelper( name );
}

void FunctionScope::Print()
{
	cout << "Function: " << name << ", " << numArgs << " arguments, " << NumLocals() << " locals" << endl << "\tall names: ";
	for( int i = 0; i < names.Size(); i++ )
	{
		Symbol* s = names.At( i );
		const char* mod;
		if( s->IsConst() ) mod = "const";
		else if( s->IsLocal() ) mod = "local";
		else if( s->IsExtern() ) mod = "extern";
		else if( s->IsArg() ) mod = "argument";
		else if( s->Type() == sym_function ) mod = "function";
		else mod = "undeclared";
		cout << mod << " " << s->Name() << "; ";
	}
	cout << endl << "\targument scope vars: ";
	for( map<const string, Symbol*>::iterator i = data.begin(); i != data.end(); ++i )
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
	names.Add( s );
}

} // namespace deva_compile
