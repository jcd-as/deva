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

// scopetable.h
// runtime scope and scope table objects for the deva language
// created by jcs, january 07, 2010 

// TODO:
// * 

#ifndef __SCOPETABLE_H__
#define __SCOPETABLE_H__


#include "object.h"
#include "exceptions.h"
#include <vector>
#include <map>


using namespace std;


namespace deva
{

class Scope
{
	// pointers to locals (actual objects stored in the frame, but the scope
	// controls freeing objects when they go out of scope)
	// TODO: switch to boost unordered map (hash table)??
	map<string, Object*> data;

public:
	Scope() {}
	~Scope()
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
	// add ref to a local (MUST BE A PTR TO LOCAL IN THE FRAME!)
	void AddSymbol( string name, Object* ob )
	{
		data.insert( pair<string, Object*>(string(name), ob) );
	}
	Object* FindSymbol( const char* name ) const
	{
		// check locals
		map<string, Object*>::const_iterator i = data.find( string(name) );
		if( i != data.end() )
			return i->second;
		return NULL;
	}
	const char* FindSymbolName( Object* o )
	{
		// check locals
		for( map<string, Object*>::iterator i = data.begin(); i != data.end(); ++i )
		{
			if( *(i->second) == *o )
				return i->first.c_str();
		}
		return NULL;
	}
};

class ScopeTable
{
	vector<Scope*> data;

public:
	~ScopeTable()
	{
		// more than one scope (global scope)??
		if( data.size() > 1 )
			throw ICE( "Scope table not empty at exit." );
		if( data.size() == 1 )
			delete data.back();
	}
	inline void PushScope( Scope* s ) { data.push_back( s ); }
	inline void PopScope() { delete data.back(); data.pop_back(); }
	inline Scope* CurrentScope() const { return data.back(); }
	inline Scope* At( size_t idx ) const { return data[idx]; }
	Object* FindSymbol( const char* name ) const
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
	const char* FindSymbolName( Object* o )
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
};


} // end namespace deva

#endif // __SCOPETABLE_H__
