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
#include "frame.h"
#include <vector>
#include <map>


using namespace std;


namespace deva
{


class Scope
{
	// helper class for storing references to scope vars, 
	// which can be locals or functions
	struct LocalRef
	{
		bool is_function;
		union
		{
			size_t index;
			Object* fcn;
		};
		LocalRef( size_t i ) : is_function( false ), index( i ) {}
		LocalRef( Object* f ) : is_function( true ), fcn( f ) {}
	};

	Frame* frame;

	bool is_function;
	bool is_module;
	// references to:
	// - locals (actual objects stored in the frame, but the scope
	// controls freeing objects when they go out of scope) and
	// - functions (actual objects stored in the executor)
	// TODO: switch to boost unordered map (hash table)??
	map<string, LocalRef> data;

	Object* GetLocal( LocalRef r ) const
	{
		if( r.is_function )
			return r.fcn;
		else
		{
			return frame->GetLocalRef( r.index );
		}
	}

public:
	Scope( Frame* f, bool is_func = false, bool is_mod = false ) : frame( f ), is_function( is_func ), is_module( is_mod ) {}
	~Scope();

	// for module scopes only, delete the data without deleting the scope:
	// (necessary because modules need to call destructors for objects, which
	// will call *back* to the scope looking for methods etc. if we did this in
	// the destructor the 'this' pointer would already be in a bad state)
	void DeleteData();
	inline bool IsFunction() { return is_function; }
	inline bool IsModule() { return is_module; }
	// add ref to a local (the index of a local in this scope's Frame)
	inline void AddSymbol( string name, size_t idx )
	{ 
		// if the symbol exists already, erase it
		map<string, LocalRef>::iterator i = data.find( name );
		if( i != data.end() )
			data.erase( i );
		data.insert( make_pair( name, LocalRef( idx ) ) );
	}
	// add a ref to a function (pointer to the Object in the Executor's function
	// collection)
	inline void AddFunction( string name, Object* f )
	{
		// if the symbol exists already, erase it
		map<string, LocalRef>::iterator i = data.find( name );
		if( i != data.end() )
			data.erase( i );
		data.insert( make_pair( name, LocalRef( f ) ) );
	}
	Object* FindSymbol( const char* name ) const;
	int FindSymbolIndex( Object* o, Frame* f ) const;
	const char* FindSymbolName( Object* o );

	vector<Function*> GetFunctions();
	vector<pair<string, Object*> > GetLocals();
};

class ScopeTable
{
	vector<Scope*> data;

public:
	~ScopeTable();
	inline void PushScope( Scope* s ) { data.push_back( s ); }
	void PopScope();
	void PopModuleScope();
	inline Scope* CurrentScope() const { return data.back(); }
	inline Scope* At( size_t idx ) const { return data[idx]; }
	Object* FindSymbol( const char* name, bool local_only = false ) const;
	Object* FindExternSymbol( const char* name ) const;
	const char* FindSymbolName( Object* o, bool local_only = false );
	Object* FindFunction( const char* name, bool local_only = false ) const;
};


} // end namespace deva

#endif // __SCOPETABLE_H__
