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

// frame.h
// callstack frame object for the deva language
// created by jcs, january 07, 2010 

// TODO:
// * 

#ifndef __FRAME_H__
#define __FRAME_H__


#include "object.h"
#include "util.h"
#include <vector>


using namespace std;


namespace deva
{


class ScopeTable;

class Frame
{
	bool is_module;
	Frame* parent;

	union
	{
		Function* function;
		NativeFunction native_function;
	};
	bool is_native;
	// array of locals (including args, at front of array):
	vector<Object> locals;

	// string data that the locals in this frame point to (i.e. non-constant
	// strings that are created by actions in the executor)
	vector<char*> strings;

	// number of arguments actually passed to the call
	int num_args;

	// return address
	byte* addr;

	// call site (ip at the time of call - address of caller)
	byte* call_site;

	// pointer at the scopes so symbols can be resolved from the frame object
	ScopeTable* scopes;

	// TODO: what else? debugging info?
	
public:
	Frame( Frame* p, ScopeTable* s, byte* loc, byte* site, int args_passed, Function* f, bool is_mod = false );
	Frame( Frame* p, ScopeTable* s, byte* loc, byte* site, int args_passed, NativeFunction f );
	~Frame();
	inline bool IsModule() { return is_module; }
	inline Frame* GetParent() { return parent; }
	inline bool IsNative() const { return is_native; }
	inline Function* GetFunction() const { return (is_native ? NULL : function ); }
	inline const NativeFunction GetNativeFunction() const { NativeFunction nf; nf.p=NULL; nf.is_method=false; return (is_native ? native_function : nf ); }
	inline size_t GetNumberOfLocals() const { return locals.size(); }
	inline Object GetLocal( size_t i ) const { return locals[i]; }
	inline Object* GetLocalRef( size_t i ) const { return (Object*)&locals[i]; }
	inline void SetLocal( size_t i, Object o ) { DecRef( locals[i] ); locals[i] = o; }
	inline byte* GetReturnAddress() const { return addr; }
	inline byte* GetCallSite() const { return call_site; }
	inline int NumArgsPassed() const { return num_args; }
	inline void AddString( char* s ) { strings.push_back( s ); }
	inline const char* AddString( string s ) { char* str = copystr( s.c_str() ); strings.push_back( str ); return str; }

	// copy all the strings in 'o' from the parent to here
	// ('o' can be a string, or a vector/map/class/instance which can then can
	// contain strings - which will be looked for recursively)
	// returns an object wrapper of the object passed in that points to the
	// copied strings
	Object CopyStringsFromParent( Object & o );

	// resolve symbols through the scope table
	Object* FindSymbol( const char* name ) const;
};


} // end namespace deva

#endif // __FRAME_H__
