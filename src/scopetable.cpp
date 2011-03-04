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
#include "frame.h"


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
		{
			// functions aren't stored in the frame's locals, but in the executor,
			// don't try to decref/free them
			if( i->second->type != obj_function )
				*(i->second) = Object();
		}
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

int Scope::FindSymbolIndex( Object* o, Frame* f ) const
{
	// check locals
	for( map<string, Object*>::const_iterator i = data.begin(); i != data.end(); ++i )
	{
		if( *(i->second) == *o )
		{
			// TODO: is there a more efficient way to do this?
			// On^2 isn't great, even they are just simple compares...

			// found, now we need to get the index of this local in the frame
			for( int i = 0; i < f->GetNumberOfLocals(); i++ )
			{
				if( o == f->GetLocalRef( i ) )
					return i;
			}
		}
	}
	// not found
	return -1;
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

Object* ScopeTable::FindSymbol( const char* name, bool local_only /*= false*/ ) const
{
	if( local_only )
	{
		return CurrentScope()->FindSymbol( name );
	}
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

const char* ScopeTable::FindSymbolName( Object* o, bool local_only /*= false*/ )
{
	if( local_only )
	{
		return CurrentScope()->FindSymbolName( o );
	}
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


} // end namespace deva
