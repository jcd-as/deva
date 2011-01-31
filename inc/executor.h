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
#include "scopetable.h"
#include "frame.h"
#include "builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"

#include <vector>

using namespace std;


namespace deva
{


struct Code
{
	byte* code;
	size_t len;

	Code( byte* c, size_t l ) : code( c ), len( l ) { }
};

class Executor
{
	// current instruction pointer (current code)
	byte* ip;
	// current base pointer (module base)
	byte* bp;
	// end of current code block
	byte* end;
	// list of code blocks (modules, eval'd blocks)
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
	map<string, Object*> functions;

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
	inline Frame* MainFrame() { return callstack[0]; }

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
//	inline void AddFunction( Function* f ) { functions.insert( pair<string, Object*>( f->name, new Object( f ) ) ); }
	inline void AddFunction( const char* name, Function* f ) { functions.insert( pair<string, Object*>( string(name), new Object( f ) ) ); }
	inline Object* FindFunction( string name ) { map<string, Object*>::iterator i = functions.find( name ); return (i == functions.end() ? NULL : i->second); }

	// constant pool handling methods
	inline bool AddConstant( Object o ) { return constants.Add( o ); }
	inline int FindConstant( const Object & o ) { return constants.Find( o ); }
	inline Object GetConstant( int idx ) { return constants.At(idx); }
	inline size_t NumConstants() { return constants.Size(); }

	// code execution methods:
	void CallConstructors( Object o, Object instance, int num_args = 0 );
	void CallDestructors( Object o );
	void ExecuteCode( const Code & code );
	Opcode ExecuteInstruction();
	void ExecuteToReturn( bool is_destructor = false );
	void ExecuteFunction( Function* f, int num_args, bool method_call_op, bool is_destructor = false );
	void ExecuteFunction( NativeFunction f, int num_args, bool method_call_op );

	// debug and output methods:
	// decode and print an opcode/instruction stream
protected:
	int PrintOpcode( Opcode op, const byte* base, byte* ip );
public:
	void Decode( const Code & code );

	void DumpFunctions();
	void DumpConstantPool();

	// process exit
	// TODO: anything needs doing here? prevent the process from exit, just the
	// executor?? (so debugger won't exit, perhaps?)
	void Exit( int exit_code ) { exit( exit_code ); }
};

extern Executor* ex;

} // namespace deva

#endif // __EXECUTOR_H__
