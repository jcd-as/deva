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
	~Scope();
	// add ref to a local (MUST BE A PTR TO LOCAL IN THE FRAME!)
	void AddSymbol( string name, Object* ob );
	Object* FindSymbol( const char* name ) const;
	const char* FindSymbolName( Object* o );
};

class ScopeTable
{
	vector<Scope*> data;

public:
	~ScopeTable();
	inline void PushScope( Scope* s ) { data.push_back( s ); }
	inline void PopScope() { delete data.back(); data.pop_back(); }
	inline Scope* CurrentScope() const { return data.back(); }
	inline Scope* At( size_t idx ) const { return data[idx]; }
	Object* FindSymbol( const char* name ) const;
	const char* FindSymbolName( Object* o );
};


} // end namespace deva

#endif // __SCOPETABLE_H__
