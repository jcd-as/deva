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
	// the name of the scope, if the scope is a namespace (module)
	// otherwise, NULL
	char* name;

	// pointers to:
	// - locals (actual objects stored in the frame, but the scope
	// controls freeing objects when they go out of scope) and
	// - functions (actual objects stored in the executor)
	// TODO: switch to boost unordered map (hash table)??
	map<string, Object*> data;

public:
	Scope() : name( NULL ) {}
	Scope( char* n ) : name( n ) {}
	~Scope();
	const char* Name() { return name; }
	// add ref to a local (MUST BE A PTR TO LOCAL IN THE FRAME OR A FUNCTION IN
	// THE EXECUTOR!)
	inline void AddSymbol( string name, Object* ob )
	{
		// if the symbol exists already, erase it
		map<string, Object*>::iterator i = data.find( name );
		if( i != data.end() )
			data.erase( i );
		data.insert( pair<string, Object*>(name, ob ) );
	}
	inline void AddFunction( string name, Object* fcn ) { AddSymbol( name, fcn ); }
	Object* FindSymbol( const char* name ) const;
	int FindSymbolIndex( Object* o, Frame* f ) const;
	const char* FindSymbolName( Object* o );
};

class ScopeTable
{
	vector<Scope*> data;

public:
	~ScopeTable();
	inline void PushScope( Scope* s ) { data.push_back( s ); }
	void PopScope();
	inline Scope* CurrentScope() const { return data.back(); }
	inline Scope* At( size_t idx ) const { return data[idx]; }
	Object* FindSymbol( const char* name, bool local_only = false ) const;
	const char* FindSymbolName( Object* o, bool local_only = false );
	Object* FindFunction( const char* name ) const { return FindSymbol( name, false ); }
};


} // end namespace deva

#endif // __SCOPETABLE_H__
