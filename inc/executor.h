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

// executor.h
// executive/VM global object/functions for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__


#include "opcodes.h"
#include "object.h"
#include "ordered_set.h"
#include "exceptions.h"
#include "util.h"

#include <vector>

using namespace std;


namespace deva
{


// TODO: move these into their own file??
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
		if( i == data.end() )
			return NULL;
		else return i->second;
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
		// TODO: full look-up logic: ???
		// builtins(???), module names, functions(???)
		//
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
};


// TODO: move this into its own file??
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

	// string data that the locals in this frame point to (i.e. non-constant
	// strings that are created by actions in the executor)
	vector<char*> strings;

	// number of arguments actually passed to the call
	int num_args;

	// return address
	dword addr;
	
	// pointer at the scopes so symbols can be resolved from the frame object
	ScopeTable* scopes;

	// TODO: what else? debugging info?
	
public:
	Frame( Frame* p, ScopeTable* s, dword loc, int args_passed, Function* f/*, void* self = NULL*/ ) : 
		parent( p ),
		scopes( s ),
		function( f ), 
		is_native( false ), 
		num_args( args_passed ), 
		addr( loc )
		{ locals = new Object[f->num_locals]; }
	Frame( Frame* p, ScopeTable* s, dword loc, int args_passed, NativeFunction f ) : 
		parent( p ),
		scopes( s ),
		native_function( f ), 
		is_native( true ), 
		num_args( args_passed ), 
		addr( loc )
		{ locals = new Object[args_passed]; }
	~Frame()
	{
		// free the local strings
		for( vector<char*>::iterator i = strings.begin(); i != strings.end(); ++i )
		{
			delete [] *i;
		}
		// free the locals array storage
		delete [] locals;
	}
	inline Frame* GetParent() { return parent; }
	inline bool IsNative() const { return is_native; }
	inline Function* GetFunction() const { return (is_native ? NULL : function ); }
	inline const NativeFunction GetNativeFunction() const { NativeFunction nf; nf.p=NULL; nf.is_method=false; return (is_native ? native_function : nf ); }
	inline Object GetLocal( int i ) const { return locals[i]; }
	inline Object* GetLocalRef( int i ) const { return &locals[i]; }
	inline void SetLocal( int i, Object o ) { locals[i] = o; }
	inline dword GetReturnAddress() const { return addr; }
	inline int NumArgsPassed() const { return num_args; }
	inline void AddString( char* s ) { strings.push_back( s ); }
	inline const char* AddString( string s ) { char* str = copystr( s.c_str() ); strings.push_back( str ); return str; }

	// resolve symbols through the scope table
	inline Object* FindSymbol( const char* name ) const { return scopes->FindSymbol( name ); }
};

struct Code
{
	byte* code;
	size_t len;

	Code( byte* c, size_t l ) : code( c ), len( l ) { }
};

class Executor
{
	byte* ip;
	vector<Code> code_blocks;

	// call stack
	vector<Frame*> callstack;

	// operand stack
	// TODO: a vector's [] op uses one deref and two adds to get to a value,
	// with a plain pointer/array we can get rid of one add.. worth it?
	vector<Object> stack;

	// scope table
	ScopeTable* scopes;

	// set of function objects
	map<string, Function*> functions;

	// native fcns/builtins
	map<string, NativeFunction> builtins;

	// set of constants (including all names)
	OrderedSet<Object> constants;

public:
	// flags:
	bool debug;
	bool trace;

public:
	Executor();
	~Executor();

	inline Scope* CurrentScope() { return scopes->CurrentScope(); }
	inline Scope* GlobalScope() { return scopes->At( 0 ); }
	inline Frame* CurrentFrame() { return callstack.back(); }

	inline void PushFrame( Frame* f ) { callstack.push_back( f ); }
	inline void PopFrame() { delete callstack.back(); callstack.pop_back(); }

	inline void PushScope( Scope* s ) { scopes->PushScope( s ); }
	inline void PopScope() { scopes->PopScope(); }

	inline void PushStack( Object o ) { stack.push_back( o ); }
	inline Object PopStack() { Object o = stack.back(); stack.pop_back(); return o; }

	inline Object* FindSymbol( const char* name ) { return scopes->FindSymbol( name ); }
	inline Object* FindLocal( const char* name ) { return CurrentScope()->FindSymbol( name ); }

	// helpers
	// TODO: adding a fcn whose name already exists should *override* the
	// existing fcn (map's behaviour is to not accept the new value)
	inline void AddFunction( Function* f ) { functions.insert( pair<string, Function*>( f->name, f ) ); }
	inline Function* FindFunction( string name ) { map<string, Function*>::iterator i = functions.find( name ); return (i == functions.end() ? NULL : i->second); }

	// constant pool handling methods
	inline bool AddConstant( Object o ) { return constants.Add( o ); }
	inline int FindConstant( const Object & o ) { return constants.Find( o ); }
	inline Object GetConstant( int idx ) { return constants.At(idx); }
	inline size_t NumConstants() { return constants.Size(); }

	// builtin handling methods
	void AddBuiltins();
	void AddBuiltin( const string name, NativeFunction fcn );
	NativeFunction FindBuiltin( string name );

	// code execution methods:
	void ExecuteCode( const Code & code );

	// debug and output methods:
	// decode and print an opcode/instruction stream
protected:
	int PrintOpcode( Opcode op, const byte* base, byte* ip );
public:
	void Decode( const Code & code );

	void DumpFunctions();
	void DumpConstantPool();
};

extern Executor* ex;

} // namespace deva

#endif // __EXECUTOR_H__
