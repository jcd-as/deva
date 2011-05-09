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
#include "executor.h"
#include "frame.h"


namespace deva
{


/////////////////////////////////////////////////////////////////////////////
// Scope methods:
/////////////////////////////////////////////////////////////////////////////

Scope::~Scope()
{
	if( !is_module )
		DeleteData();
}

// for module scopes only, delete the data without deleting the scope:
// (necessary because modules need to call destructors for objects, which
// will call *back* to the scope looking for methods etc. if we did this in
// the destructor the 'this' pointer would already be in a bad state)
void Scope::DeleteData()
{
	// release-ref and 'zero' out the locals, to error out in case they get 
	// accidentally used after they should be gone, and to ensure they aren't 
	// DecRef'd again if/when a new value is assigned to them (e.g. in a loop)
	for( map<string, LocalRef>::iterator i = data.begin(); i != data.end(); ++i )
	{
		Object* o = GetLocal( i->second );
		DecRef( *o );
		// functions aren't stored in the frame's locals, but in the executor,
		// don't try to overwrite them
		if( o->type != obj_function )
			*(o) = Object();
	}
	// clear the map & vector 'dead pools' (items to be deleted)
	Map::ClearDeadPool();
	Vector::ClearDeadPool();
}

Object* Scope::FindSymbol( const char* name ) const
{
	// check locals
	map<string, LocalRef>::const_iterator i = data.find( string(name) );
	if( i != data.end() )
		return GetLocal( i->second );
	return NULL;
}

int Scope::FindSymbolIndex( Object* o, Frame* f ) const
{
	// check locals
	for( map<string, LocalRef>::const_iterator i = data.begin(); i != data.end(); ++i )
	{
		Object* loc = GetLocal( i->second );
		if( *loc == *o )
		{
			// TODO: is there a more efficient way to do this?
			// On^2 isn't great, even they are just simple compares...

			// found, now we need to get the index of this local in the frame
			for( size_t i = 0; i < f->GetNumberOfLocals(); i++ )
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
	for( map<string, LocalRef>::iterator i = data.begin(); i != data.end(); ++i )
	{
		if( *(GetLocal( i->second )) == *o )
			return i->first.c_str();
	}
	return NULL;
}

vector<Function*> Scope::GetFunctions()
{
	vector<Function*> fcns;
	for( map<string, LocalRef>::iterator i = data.begin(); i != data.end(); ++i )
	{
		if( i->second.is_function )
			fcns.push_back( GetLocal( i->second )->f );
	}
	return fcns;
}

vector<pair<string, Object*> > Scope::GetLocals()
{
	vector< pair<string, Object*> > locals;
	for( map<string, LocalRef>::iterator i = data.begin(); i != data.end(); ++i )
	{
		locals.push_back( make_pair( i->first, GetLocal( i->second ) ) );
	}
	return locals;
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

void ScopeTable::PopScope()
{ 
	Scope* s = data.back();
	// module scopes are not deleted by the ScopeTable, but rather by the executor,
	// on exit
	if( !s->IsModule() )
	{
		// if this is the last scope (main), we need to free up the executor's
		// error object (because it will go into the dead pool, which the scope
		// cleans up - if we wait for the Executor's destructor, the scope will 
		// already be deleted and it will be too late to add things to the dead
		// pool and thus will leak)
		if( data.size() == 1 )
			ex->DeleteErrorObject();
		delete data.back();
	}
	data.pop_back();
}

Object* ScopeTable::FindSymbol( const char* name, bool local_only /*= false*/ ) const
{
	// look in each scope
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		// check for the symbol
		Object* o = (*i)->FindSymbol( name );
		if( o )
			return o;
		if( local_only && (*i)->IsFunction() )
			break;
	}
	return NULL;
}

Object* ScopeTable::FindExternSymbol( const char* name ) const
{
	// find the first scope out of the current function
	// look in each scope thereafter
	bool external = false;
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		if( external )
		{
			// check for the symbol
			Object* o = (*i)->FindSymbol( name );
			if( o )
				return o;
		}
		// if this was a fcn, start looking in the scopes after this
		if( !external && (*i)->IsFunction() )
			external = true;
	}
	return NULL;
}

const char* ScopeTable::FindSymbolName( Object* o, bool local_only /*= false*/ )
{
	// look in each scope
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		// check for the symbol
		const char* n = (*i)->FindSymbolName( o );
		if( n )
			return n;
		if( local_only && (*i)->IsFunction() )
			break;
	}
	return NULL;
}

Object* ScopeTable::FindFunction( const char* name, bool local_only /*= false*/ ) const
{
	// look in each scope
	for( vector<Scope*>::const_reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
	{
		// check for the symbol
		Object* o = (*i)->FindSymbol( name );
		if( o && (o->type == obj_function || o->type == obj_native_function || o->type == obj_class) )
			return o;
		if( local_only && (*i)->IsFunction() )
			break;
	}
	return NULL;
}

} // end namespace deva
