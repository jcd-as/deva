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

// scopetable.cpp
// runtime scope and scope table objects for the deva language
// created by jcs, january 18, 2010 

#include "scopetable.h"


namespace deva
{


/////////////////////////////////////////////////////////////////////////////
// Scope methods:
/////////////////////////////////////////////////////////////////////////////

Scope::~Scope()
{
	// release-ref the vectors/maps and
	// 'zero' out the locals non-ref types, to error out in case they get 
	// accidentally used after they should be gone
	for( map<string, Object*>::iterator i = data.begin(); i != data.end(); ++i )
	{
		if( IsRefType( i->second->type ) )
			DecRef( *(i->second) );
		else
			*(i->second) = Object();
	}
	// clear the map & vector 'dead pools' (items to be deleted)
	Map::ClearDeadPool();
	Vector::ClearDeadPool();
}

Object* Scope::FindSymbol( const char* name ) const
{
	// check locals
	map<string, Object*>::const_iterator i = data.find( string(name) );
	if( i != data.end() )
		return i->second;
	return NULL;
}

const char* Scope::FindSymbolName( Object* o )
{
	// check locals
	for( map<string, Object*>::iterator i = data.begin(); i != data.end(); ++i )
	{
		if( *(i->second) == *o )
			return i->first.c_str();
	}
	return NULL;
}

Object* Scope::FindFunction( const char* name ) const
{
	// check for function
	map<string, Object*>::const_iterator i = functions.find( string( name ) );
	if( i == functions.end() )
		return NULL;
	else
		return i->second;
}

/////////////////////////////////////////////////////////////////////////////
// ScopeTable methods:
/////////////////////////////////////////////////////////////////////////////

ScopeTable::~ScopeTable()
{
	// more than one scope (global scope)??
	if( data.size() > 1 )
		throw ICE( "Scope table not empty at exit." );
	if( data.size() == 1 )
		delete data.back();
}

Object* ScopeTable::FindSymbol( const char* name ) const
{
	// look in each scope
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		// check for the symbol
		Object* o = (*i)->FindSymbol( name );
		if( o )
			return o;
	}
	return NULL;
}

const char* ScopeTable::FindSymbolName( Object* o )
{
	// look in each scope
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		// check for the symbol
		const char* n = (*i)->FindSymbolName( o );
		if( n )
			return n;
	}
	return NULL;
}

Object* ScopeTable::FindFunction( const char* name ) const
{
	// look in each scope
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		// check for the function
		Object* o = (*i)->FindFunction( name );
		if( o )
			return o;
	}
	return NULL;
}


} // end namespace deva
