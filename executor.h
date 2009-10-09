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

using namespace std;


class Executor
{
private:
	// private data
	////////////////////////////////////////////////////
	// the file to execute
	string filename;

	// in debug_mode?
	bool debug_mode;

	// the data stack
	vector<DevaObject> stack;

	// the current location in the file ("instruction pointer")
	long ip;

	// the stack of scopes representing the symbol table(s)
	// for the current state
	struct Scope : public map<string, DevaObject*>
	{
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
			back()->insert( pair<string, DevaObject*>(ob->name, ob) );
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
	};
	ScopeTable scopes;

	// the bytecode itself (contents of the file):
	unsigned char *code;

	// private helper functions:
	////////////////////////////////////////////////////
	// load the bytecode from the file
	void LoadByteCode();
	// locate a symbol in the symbol table(s)
	DevaObject* find_symbol( const DevaObject & ob );
	// peek at what the next instruction is (doesn't modify ip)
	Opcode PeekInstr();
	// read a string from *ip into s
	// (allocates mem for s, caller must delete!)
	void read_string( char* & s );
	// read a byte
	void read_byte( unsigned char & l );
	// read a long
	void read_long( long & l );
	// read a double
	void read_double( double & d );

	// helper for the comparison ops:
	// returns 0 if equal, -1 if lhs < rhs, +1 if lhs > rhs
	int compare_objects( DevaObject & lhs, DevaObject & rhs );
	// evaluate an object as a boolean value
	// object must be evaluated already to a value (i.e. no variables)
	bool evaluate_object_as_boolean( DevaObject & o );

	// individual op-code methods
	////////////////////////////////////////////////////
	// 0 pop top item off stack
	DevaObject Pop( Instruction const & inst );
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
	// 9 get item from vector
	void Vec_load( Instruction const & inst );
	// 10 set item in vector. args: index, value
	void Vec_store( Instruction const & inst );
	// 11 swap top two items on stack. no args
	void Swap( Instruction const & inst );
	// 12 set item in map. args: index, value
	void Map_store( Instruction const & inst );
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
	// 33 as return, but stack holds return value and then (at top) return address
	void Returnv( Instruction const & inst );
	// 34 break out of loop, respecting scope (enter/leave)
	void Break( Instruction const & inst );
	// 35 enter new scope
	void Enter( Instruction const & inst );
	// 36 leave scope
	void Leave( Instruction const & inst );
	// 37 no op
	void Nop( Instruction const & inst );
	// 38 finish program, 0 or 1 ops (return code)
	void Halt( Instruction const & inst );
	// illegal operation, if exists there was a compiler error/fault
	void Illegal( Instruction const & inst );
	///////////////////////////////////////////////////////////

	bool DoInstr( Instruction & inst );
	// get the next instruction from the current position
	Instruction NextInstr();

public:
	// public methods
	////////////////////////////////////////////////////
	Executor( string fname, bool debug_mode = false );
	~Executor();

	bool RunFile();

	// be-friend the built-in functions
	friend void do_print( Executor *ex, const Instruction & inst );
	friend void do_str( Executor *ex, const Instruction & inst );
	friend void do_append( Executor *ex, const Instruction & inst );
	friend void do_length( Executor *ex, const Instruction & inst );
};

#endif // __EXECUTOR_H__
