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
#include "scopetable.h"
#include "util.h"
#include <vector>


using namespace std;


namespace deva
{


class Frame
{
	Frame* parent;

	union
	{
		Function* function;
		NativeFunction native_function;
	};
	bool is_native;
	// array of locals (including args at front of array):
	Object* locals;
	// number of slots in the locals array
	int num_locals;

	// string data that the locals in this frame point to (i.e. non-constant
	// strings that are created by actions in the executor)
	vector<char*> strings;

	// number of arguments actually passed to the call
	int num_args;

	// return address (ip at the time of call - address of caller)
	byte* addr;

	// pointer at the scopes so symbols can be resolved from the frame object
	ScopeTable* scopes;

	// TODO: what else? debugging info?
	
public:
	Frame( Frame* p, ScopeTable* s, byte* loc, int args_passed, Function* f );
	Frame( Frame* p, ScopeTable* s, byte* loc, int args_passed, NativeFunction f );
	~Frame();
	inline Frame* GetParent() { return parent; }
	inline bool IsNative() const { return is_native; }
	inline Function* GetFunction() const { return (is_native ? NULL : function ); }
	inline const NativeFunction GetNativeFunction() const { NativeFunction nf; nf.p=NULL; nf.is_method=false; return (is_native ? native_function : nf ); }
	inline int GetNumberOfLocals() const { return num_locals; }
	inline Object GetLocal( int i ) const { return locals[i]; }
	inline Object* GetLocalRef( int i ) const { return &locals[i]; }
	inline void SetLocal( int i, Object o ) { DecRef( locals[i] ); locals[i] = o; }
	inline byte* GetReturnAddress() const { return addr; }
	inline int NumArgsPassed() const { return num_args; }
	inline void AddString( char* s ) { strings.push_back( s ); }
	inline const char* AddString( string s ) { char* str = copystr( s.c_str() ); strings.push_back( str ); return str; }

	// resolve symbols through the scope table
	Object* FindSymbol( const char* name ) const;
	// TODO: doesn't find names of builtins, modules, fcns etc
	// find a symbol's name
	inline const char* FindSymbolName( Object* o ) { return scopes->FindSymbolName( o ); }
};


} // end namespace deva

#endif // __FRAME_H__
