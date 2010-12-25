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

#include <vector>

using namespace std;


namespace deva
{

const size_t FRAME_SIZE = 128;
const size_t STACK_SIZE = 1024;


// TODO: move these into their own file??
class Scope
{
	// TODO: switch to boost unordered map (hash table)??
	map<string, DevaObject*> data;

public:
	~Scope()
	{
		for( map<string, DevaObject*>::iterator i = data.begin(); i != data.end(); ++i )
		{
			delete i->second;
		}
	}
	void AddObject( const char* name, DevaObject* ob )
	{
		data.insert( pair<string, DevaObject*>(string(name), ob) );
	}
	DevaObject* FindSymbol( const char* name )
	{
		map<string, DevaObject*>::iterator i = data.find( string(name) );
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
		if( data.size() != 1 )
			throw DevaICE( "Scope table not empty at exit." );
	}
	inline void PushScope( Scope* s ) { data.push_back( s ); }
	inline void PopScope() { data.pop_back(); }
	inline Scope* CurrentScope() { return data.back(); }
	inline Scope* At( size_t idx ) { return data[idx]; }
	DevaObject* FindSymbol( const char* name )
	{
		// look in each scope
		for( vector<Scope*>::reverse_iterator i = data.rbegin(); i != data.rend(); ++i )
		{
			// check for the symbol
			DevaObject* o = (*i)->FindSymbol( name );
			if( o )
				return o;
		}
		return NULL;
	}
};


// TODO: move this into its own file??
class Frame
{
	union
	{
		DevaFunction* function;
		NativeFunction native_function;
	};
	bool is_native;
	// locals (including args):
	// TODO: a vector's [] op uses one deref and two adds to get to a value,
	// with a plain pointer/array we can get rid of one add.. worth it?
	vector<DevaObject> locals;

	// number of arguments actually passed to the call
	int num_args;

	// return address
	dword addr;
	
	// TODO: what else? debugging info?
	
public:
	Frame( dword loc, int args_passed, DevaFunction* f ) : 
		function( f ), 
		is_native( false ), 
		locals( f->num_locals ), 
		num_args( args_passed ), 
		addr( loc )
		{}
	Frame( dword loc, int args_passed, NativeFunction f ) : 
		native_function( f ), 
		is_native( true ), 
		locals( args_passed ), // native fcn, has no locals except the args passed to it
		num_args( args_passed ), 
		addr( loc )
		{}
	inline bool IsNative() { return is_native; }
	inline const DevaFunction* GetFunction() { return (is_native ? NULL : function ); }
	inline const NativeFunction GetNativeFunction() { return (is_native ? native_function : NULL); }
	inline DevaObject GetLocal( int i ) { return locals[i]; }
	inline void SetLocal( int i, DevaObject o ) { locals[i] = o; }
	inline dword GetReturnAddress() { return addr; }
	inline int NumArgsPassed() { return num_args; }
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
	vector<DevaObject> stack;

	// scope table
	ScopeTable scopes;

//public:
	// set of function objects
	map<string, DevaFunction*> functions;

	// native fcns/builtins
	map<string, NativeFunction> builtins;

	// set of constants (including all names)
	OrderedSet<DevaObject> constants;

public:
	Executor();
	~Executor();

	inline Scope* CurrentScope() { return scopes.CurrentScope(); }
	inline Scope* GlobalScope() { return scopes.At( 0 ); }
	inline Frame* CurrentFrame() { return callstack.back(); }

	inline void PushFrame( Frame* f ) { callstack.push_back( f ); }
	inline void PopFrame() { delete callstack.back(); callstack.pop_back(); }

	inline void PushScope( Scope* s ) { scopes.PushScope( s ); }
	inline void PopScope() { scopes.PopScope(); }

	inline void PushStack( DevaObject o ) { stack.push_back( o ); }
	inline DevaObject PopStack() { DevaObject o = stack.back(); stack.pop_back(); return o; }

	inline DevaObject* FindSymbol( const char* name ) { return scopes.FindSymbol( name ); }
	inline DevaObject* FindLocal( const char* name ) { return CurrentScope()->FindSymbol( name ); }

	// helpers
	// TODO: adding a fcn whose name already exists should *override* the
	// existing fcn (map's behaviour is to not accept the new value)
	inline void AddFunction( DevaFunction* f ) { functions.insert( pair<string, DevaFunction*>( f->name, f ) ); }
	inline DevaFunction* FindFunction( string name ) { map<string, DevaFunction*>::iterator i = functions.find( name ); return (i == functions.end() ? NULL : i->second); }
	inline map<string,DevaFunction*> GetFunctions(){ return functions; }

	inline void AddNativeFunction( string name, NativeFunction f ) { builtins.insert( pair<string, NativeFunction>( name, f ) ); }
	inline NativeFunction FindNativeFunction( string name ) { map<string, NativeFunction>::iterator i = builtins.find( name ); return (i == builtins.end() ? NULL : i->second); }
	inline map<string,NativeFunction> GetNativeFunctions(){ return builtins; }


	inline void AddConstant( DevaObject o ) { constants.Add( o ); }
	inline int FindConstant( const DevaObject & o ) { return constants.Find( o ); }
	inline DevaObject GetConstant( int idx ) { return constants.At(idx); }
	inline size_t NumConstants() { return constants.Size(); }

	void AddBuiltin( const char* name, NativeFunction fcn );

	// code execution functions:
	void ExecuteCode( const Code & code );
};

extern Executor* ex;

} // namespace deva

#endif // __EXECUTOR_H__
