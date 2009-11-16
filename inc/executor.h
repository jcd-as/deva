// Copyright (c) 2009 Joshua C. Shepard
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
// deva language intermediate language & virtual machine execution engine
// created by jcs, september 26, 2009 

// TODO:
// * 

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include "opcodes.h"
#include "symbol.h"
#include "types.h"
#include "exceptions.h"
#include "scope.h"
#include "instructions.h"
#include <fstream>
#include <vector>
#include <cstring>

using namespace std;

class Executor;
typedef void (*builtin_fcn)(Executor*);

class Executor
{
public:
	// public data
	////////////////////////////////////////////////////
	// the data stack
	vector<DevaObject> stack;

	// static data
	static int args_on_stack;

private:
	// private data
	////////////////////////////////////////////////////
	
	// in debug_mode?
	bool debug_mode;

	// the current location in the file ("instruction pointer")
	size_t ip;

	// the top-level executing file (the path given to RunFile())
	string executing_filepath;

	// currently executing file and line number. 
	// (only tracked if compiled with debug info)
	string file;
	int line;

	// the stack of scopes representing the symbol table(s)
	// for the current state
	struct Scope : public map<string, DevaObject*>
	{
		void AddObject( DevaObject* ob )
		{
			insert( pair<string, DevaObject*>(ob->name, ob) );
		}
		~Scope()
		{
			// delete all the DevaObject ptrs
			for( map<string, DevaObject*>::iterator i = begin(); i != end(); ++i )
			{
				delete i->second;
			}
		}
	};
	struct ScopeTable : public vector<Scope*>
	{
		void AddObject( DevaObject* ob )
		{
			back()->AddObject( ob );
		}
		void Push()
		{
			push_back( new Scope() );
		}
		void Pop()
		{
			// free the objects in this scope
			delete back();
			pop_back();
		}
		~ScopeTable()
		{
			// it is normal for 'namespace' scope tables (i.e. all but the
			// global scope table) to have ONE scope, the module scope, left at
			// destruction
			if( size() > 1 )
				throw DevaRuntimeException( "ScopeTable not empty.\nIf your program exited normally, this indicates an internal memory leak of some kind.\nIf your program did not exit normally, this is expected." );
			else if( size() == 1 )
			{
				delete back();
				pop_back();
			}
		}
	};
	ScopeTable global_scopes;
	map<string, ScopeTable> namespaces;
	ScopeTable *current_scopes;

	// maps for built-in module fcns
	// map of module names to a list of fcn names
	map<string, vector<string> > builtin_modules;
	// map fcn name to fcn ptr, where fcn name is of the form 'fcn@module'
	map<string, builtin_fcn> builtin_module_fcns;

	// the various pieces of bytecode (file, dynamically loaded pieces etc)
	vector<unsigned char*> code_blocks;

	// the current bytecode (current code_block we're working in/with)
	unsigned char *code;

	// private helper functions:
	////////////////////////////////////////////////////
	// load the bytecode from the file
	unsigned char* LoadByteCode( const char* const filename );
	// fixup all the offsets in 'function' symbols (fcns, returns, jumps) to
	// pointers in actual memory
	void FixupOffsets();
	// peek at what the next instruction is (doesn't modify ip)
	Opcode PeekInstr();
	// read a string from *ip into s
	// (allocates mem for s, caller must delete!)
	void read_string( char* & s );
	// read a byte
	void read_byte( unsigned char & l );
	// read a long
	void read_long( long & l );
	// read a size_t
	void read_size_t( size_t & l );
	// read a double
	void read_double( double & d );

	// helper for the comparison ops:
	// returns 0 if equal, -1 if lhs < rhs, +1 if lhs > rhs
	int compare_objects( DevaObject & lhs, DevaObject & rhs );
	// evaluate an object as a boolean value

	// locate a module
	string find_module( string mod );

	// individual op-code methods
	////////////////////////////////////////////////////
	// 0 pop top item off stack
	void Pop( Instruction const & inst );
	// 1 push item onto top of stack
	void Push( Instruction const & inst );
	// 2 load a variable from memory to the stack
	void Load( Instruction const & inst );
	// 3 store a variable from the stack to memory
	void Store( Instruction const & inst );
	// 4 define function. arg is location in instruction stream, named the fcn name
	void Defun( Instruction const & inst );
	// 5 define an argument to a fcn. argument (to opcode) is arg name
	void Defarg( Instruction const & inst );
	// 6 dup a stack item from 'arg' position to the top of the stack
	void Dup( Instruction const & inst );
	// 7 create a new map object and push onto stack
	void New_map( Instruction const & inst );
	// 8 create a new vector object and push onto stack
	void New_vec( Instruction const & inst );
	// 9 get item from vector or map
	void Tbl_load( Instruction const & inst );
	// 10 set item in vector or map. args: index, value
	void Tbl_store( Instruction const & inst );
	// 11 swap top two items on stack. no args
	void Swap( Instruction const & inst );
	// 12 line number (file name and line number in args)
	void Line_num( Instruction const & inst );
	// 13 unconditional jump to the address on top of the stack
	void Jmp( Instruction const & inst );
	// 14 jump on top of stack evaluating to false 
	void Jmpf( Instruction const & inst );
	// 15 == compare top two values on stack
	void Eq( Instruction const & inst );
	// 16 != compare top two values on stack
	void Neq( Instruction const & inst );
	// 17 < compare top two values on stack
	void Lt( Instruction const & inst );
	// 18 <= compare top two values on stack
	void Lte( Instruction const & inst );
	// 19 > compare top two values on stack
	void Gt( Instruction const & inst );
	// 20 >= compare top two values on stack
	void Gte( Instruction const & inst );
	// 21 || the top two values
	void Or( Instruction const & inst );
	// 22 && the top two values
	void And( Instruction const & inst );
	// 23 negate the top value ('-' operator)
	void Neg( Instruction const & inst );
	// 24 boolean not the top value ('!' operator)
	void Not( Instruction const & inst );
	// 25 add top two values on stack
	void Add( Instruction const & inst );
	// 26 subtract top two values on stack
	void Sub( Instruction const & inst );
	// 27 multiply top two values on stack
	void Mul( Instruction const & inst );
	// 28 divide top two values on stack
	void Div( Instruction const & inst );
	// 29 modulus top two values on stack
	void Mod( Instruction const & inst );
	// 30 dump top of stack to stdout
	void Output( Instruction const & inst );
	// 31 call a function. arguments on stack
	void Call( Instruction const & inst );
	// 32 pop the return address and unconditionally jump to it
	void Return( Instruction const & inst );
	// 33 break out of loop, respecting scope (enter/leave)
	void Break( Instruction const & inst );
	// 34 enter new scope
	void Enter( Instruction const & inst );
	// 35 leave scope
	void Leave( Instruction const & inst );
	// 36 no op
	void Nop( Instruction const & inst );
	// 37 finish program, 0 or 1 ops (return code)
	void Halt( Instruction const & inst );
	// 38 import a module, 1 arg: module name
	void Import( Instruction const & inst );
	// 39 new class
	void New_class( Instruction const & inst );
	// 40 new instance
	void New_instance( Instruction const & inst );
	// illegal operation, if exists there was a compiler error/fault
	void Illegal( Instruction const & inst );
	///////////////////////////////////////////////////////////

	bool DoInstr( Instruction & inst );
	// get the next instruction from the current position
	Instruction NextInstr();

public:
	// public methods
	////////////////////////////////////////////////////
	Executor( bool debug_mode = false );
	~Executor();

	void ExecuteDevaFunction( string fcn_name, int num_args );
	void StartGlobalScope();
	void EndGlobalScope();
	bool RunFile( const char* const filename );
	bool RunText( const char* const text );

	bool AddBuiltinModule( string name, map<string, builtin_fcn> & fcns );

	// helper methods. useful for built-ins
	////////////////////////////////////////////////////
	// locate a symbol in the symbol table(namespace)
	DevaObject* find_symbol( const DevaObject & ob, ScopeTable* scopes = NULL );

	// remove a symbol from the symbol table(namespace)
	void remove_symbol( const DevaObject & ob, ScopeTable* scopes = NULL );

	// object must be evaluated already to a value (i.e. no variables)
	bool evaluate_object_as_boolean( DevaObject & o );
};

#endif // __EXECUTOR_H__
