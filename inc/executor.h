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
// typedef for pointer to built-in fcn
typedef void (*builtin_fcn)(Executor*);
// typedef for pointer to fcn to import a built-in (i.e. native) module
typedef void (*import_module_fcn)(Executor*);

class Executor
{
private:
	////////////////////////////////////////////////////
	// private data
	////////////////////////////////////////////////////
	
	// in debug_mode?
	bool debug_mode;

	// the current location in the file ("instruction pointer")
	size_t ip;

	// the top-level executing file (the path given to RunFile())
	string executing_filepath;

	// currently executing file, function/method and line number. 
	// (only tracked if compiled with debug info)
	string file;
	string function;
	int line;

	// data for built-in module fcns
	// maps of module names to import fcns
	map<string, import_module_fcn> builtin_module_names;
	// map of module names to a list of fcn names
	map<string, vector<string> > builtin_modules;
	// map fcn name to fcn ptr, where fcn name is of the form 'fcn@module'
	map<string, builtin_fcn> builtin_module_fcns;

	// the various pieces of bytecode (file, dynamically loaded pieces etc)
	vector<unsigned char*> code_blocks;

	// the current bytecode (current code_block we're working in/with)
	unsigned char *code;

	// global error handling:
	// the global error flag
	bool is_error;
	// the global error data
	DevaObject* error_data;

	////////////////////////////////////////////////////
	// nested types
	////////////////////////////////////////////////////
	// the stack of scopes representing the symbol table(s)
	// for the current state
	struct Scope : public map<string, DevaObject*>
	{
		Executor* ex;
		bool data_destroyed;

		void AddObject( DevaObject* ob )
		{
			insert( pair<string, DevaObject*>(ob->name, ob) );
		}
		// generally speaking the destructor should be allowed to delete the
		// contents of the scope, BUT in the particular case of the global
		// scope the instances and non-instances need to be deleted separately,
		// so these methods are provided
		void DeleteInstances()
		{
			data_destroyed = true;

			// first call the destructors on instances
			for( map<string, DevaObject*>::iterator i = begin(); i != end(); ++i )
			{
#ifdef DEBUG
				string name = i->first;
				DevaObject* ob = i->second;
#endif
				/////////////////////////////////////////////
				// is this object an instance?
				if( i->second->Type() == sym_instance )
				{
					DevaObject* instance = i->second;
					// is it going to be deleted?
					if( instance->map_val.getRefs() == 1 )
					{
						// get the name of the class
						DOMap::iterator it = instance->map_val->find( DevaObject( "", string( "__class__" ) ) );
						if( it == instance->map_val->end() )
							throw DevaCriticalException( "Class instance cannot be destroyed, it has no '__class__' member." );
						if( it->second.Type() != sym_string )
							throw DevaICE( "__class__ attribute on a class instance is not of type 'string'." );
						string cls_name = it->second.str_val;

						// get the name of the module
						it = instance->map_val->find( DevaObject( "", string( "__module__" ) ) );
						if( it == instance->map_val->end() )
							throw DevaCriticalException( "Class instance cannot be destroyed, it has no '__module__' member." );
						if( it->second.Type() != sym_string )
							throw DevaICE( "__module__ attribute on a class instance is not of type 'string'." );
						string mod_name = it->second.str_val;

						// does it have a 'delete' method?
						string del( "delete@" );
						del += cls_name;

						// no module... (i.e. 'main' module)
						ScopeTable* ns = NULL;
						// look up the namespace if we have a module
						if( mod_name.length() != 0 )
						{
							vector< pair<string, ScopeTable*> >::iterator iter;
							iter = ex->find_namespace( mod_name );
							// not found?
							if( iter == ex->namespaces.end() )
								throw DevaCriticalException( "Trying to destroy an instance of a class in an unknown namespace." );
							// else we found the namespace
							else
							{
								ns = iter->second;
							}
						}
						// look up the class
						DevaObject* cls = NULL;
						cls = ex->find_symbol( DevaObject( cls_name, sym_unknown ), ns );
						if( !cls )
							throw DevaICE( boost::format( "Trying to destroy unknown class '%1%'." ) % cls_name );

						// look up the fcn name
						DevaObject* fcn = ex->find_symbol( del, ns );
						if( fcn )
						{
							if( fcn->Type() != sym_address )
								throw DevaCriticalException( "Object is invalid: 'delete' is not a method." );
							// push 'self'
							ex->stack.push_back( *instance );
							// call it
							ex->ExecuteDevaFunction( del, 1, ns );
							// ignore the return value
							ex->stack.pop_back();
						}
					}
				}
			}

			// next, delete the instances and null them out in the collection
			// (do this first so the classes (& methods) that they
			// implement/inherit from aren't deleted first)
			for( map<string, DevaObject*>::iterator i = begin(); i != end(); ++i )
			{
#ifdef DEBUG
				string name = i->first;
				DevaObject* ob = i->second;
#endif
				// is this object an instance?
				if( i->second->Type() == sym_instance )
				{
					delete i->second;
					i->second = NULL;
				}
			}

		}
		void DeleteNonInstances()
		{
			data_destroyed = true;

			for( map<string, DevaObject*>::iterator i = begin(); i != end(); ++i )
			{
				if( i->second == NULL )
					continue;
#ifdef DEBUG
				string name = i->first;
				DevaObject* ob = i->second;
#endif
				if( i->second->Type() != sym_instance )
					delete i->second;
			}
		}
		Scope( Executor* e ) : ex( e ), data_destroyed( false ){}
		~Scope()
		{
			// if we're already deleted, bail
			if( data_destroyed )
				return;

			// first call the destructors on instances
			DeleteInstances();
			// then delete all the non-instance objects
			DeleteNonInstances();
		}
	};
	friend class equal_to_first;
	struct ScopeTable : public vector<Scope*>
	{
		Executor* ex;
		void AddObject( DevaObject* ob )
		{
			back()->AddObject( ob );
		}
		void Push()
		{
			push_back( new Scope( ex ) );
		}
		void Pop()
		{
			// free the objects in this scope
			delete back();
			pop_back();
		}
		ScopeTable( Executor* e ) : ex( e ){}
		~ScopeTable()
		{
			// it is normal for 'namespace' scope tables (i.e. all but the
			// global scope table) to have ONE scope, the module scope, left at
			// destruction
			if( size() == 1 )
			{
				delete back();
				pop_back();
			}
#ifdef DEBUG
			// when we are destroyed because of an exception resulting from
			// invalid code (DevaRuntimeException), this is a spurious exception
			// because we're exiting anyway... let's only see this in debug
			// builds
			if( size() > 0 )
				throw DevaICE( "Not all scopes removed from scope table" );
#endif
		}
	};

	// class defining a frame of execution
	struct Frame
	{
		// the scope table for this frame
		ScopeTable* scopes;
		// index into the scope table for this frame
		int scope_idx;
		// index of the initial instruction for this frame
		size_t ip;
		// file, function and line number of the initial line for this frame
		string file;
		string function;
		int line;
		// is this frame the entry to a fcn? (e.g. the result of a call)
		// -1 if this is NOT a call, otherwise the linenum of the originating
		// call-site
		int call_site;

		Frame( Executor const & ex, int call ) : scopes( ex.current_scopes ), scope_idx( ex.current_scopes->size() ), ip( ex.ip ), file( ex.file ), function( ex.function ), line( ex.line ), call_site( call )
		{}
	};
	// class for a call-stack/traceback/backtrace
	struct CallStack : public vector<Frame>
	{
		void Push( Executor const & ex, int call )
		{
			push_back( Frame( ex, call ) );
		}
		void Pop()
		{
			pop_back();
		}
	};

	class dataStack
	{
		size_t limit;
		vector<DevaObject> stack;

	public:
		dataStack() : limit( 0 )
		{}
		void push_back( const DevaObject & obj )
		{
			stack.push_back( obj );
		}
		void pop_back()
		{
			if( stack.size() == 0 )
				throw DevaCriticalException( "Stack underflow." );
			if( limit )
			{
				if( stack.size() == limit )
					throw DevaStackException( "Stack limit underflow." );
			}
			stack.pop_back();
		}
		DevaObject back()
		{
			return stack.back();
		}
		size_t size()
		{
			return stack.size();
		}
		DevaObject operator[]( size_t idx )
		{
			return stack[idx];
		}
		// roll the stack from the given position (position is an 'index' 
		// from the top of the stack, which is the same as a 'reverse index'
		// from the end of the vector)
		// (item at position is removed and pushed onto the top of the stack)
		void roll( size_t pos )
		{
			//DevaObject temp = stack[stack.size() - (pos+1)];
			DevaObject temp = *(stack.end() - (pos+1));
			stack.erase( stack.end() - (pos+1) );
			stack.push_back( temp );
		}
		// set the current size as a minimum limit
		void SetLimit()
		{
			limit = stack.size();
		}
		void UnsetLimit()
		{
			limit = 0;
		}
	};

	////////////////////////////////////////////////////
	// private data associated with nested types:
	////////////////////////////////////////////////////
	ScopeTable *global_scopes;
	vector< pair<string, ScopeTable*> > namespaces;
	ScopeTable *current_scopes;

	CallStack trace;

	// list of breakpoints
	vector<pair<string, int> > breakpoints;

public:
	////////////////////////////////////////////////////
	// public data associated with nested types:
	////////////////////////////////////////////////////
	// the data stack
	dataStack stack;

	// static data
	static int args_on_stack;

private:
	////////////////////////////////////////////////////
	// private helper functions:
	////////////////////////////////////////////////////
	// load the bytecode from the file
	unsigned char* LoadByteCode( const char* const filename );
	// fixup all the offsets for a code block in 'function' symbols (fcns, returns, jumps) to
	// pointers in actual memory
	void FixupOffsets( unsigned char* code );
	// peek at what the next instruction is (doesn't modify ip)
	Opcode PeekInstr();
	// read a string from *ip into s
	// (allocates mem for s, caller must delete!)
	unsigned char* read_string( char* & s, unsigned char* ip = 0 );
	// read a byte
	unsigned char* read_byte( unsigned char & l, unsigned char* ip = 0 );
	// read a long
	unsigned char* read_long( long & l, unsigned char* ip = 0 );
	// read a size_t
	unsigned char* read_size_t( size_t & l, unsigned char* ip = 0 );
	// read a double
	unsigned char* read_double( double & d, unsigned char* ip = 0 );

	// helper for the comparison ops:
	// returns 0 if equal, -1 if lhs < rhs, +1 if lhs > rhs
	int compare_objects( DevaObject & lhs, DevaObject & rhs );
	// evaluate an object as a boolean value

	// locate a module
	string find_module( string mod );

	// find a namespace
	vector< pair<string, ScopeTable*> >::iterator find_namespace( string mod );

	// call destructors on base classes
	void destruct_base_classes( DevaObject* ob, DevaObject & instance );

	////////////////////////////////////////////////////
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
	// 42 roll
	void Roll( Instruction const & inst );
	// illegal operation, if exists there was a compiler error/fault
	void Illegal( Instruction const & inst );
	///////////////////////////////////////////////////////////

	bool DoInstr( Instruction & inst );
	// get the next instruction from the current position
	Instruction NextInstr();

public:
	////////////////////////////////////////////////////
	// public methods
	////////////////////////////////////////////////////
	Executor( bool debug_mode = false );
	~Executor();

	void ExecuteDevaFunction( string fcn_name, int num_args, ScopeTable* ns = NULL );
	void StartGlobalScope();
	void EndGlobalScope();
	void AddCodeBlock( unsigned char* cd );
	void RunCode( unsigned char* cd, bool stop_at_breakpoints = true );
	void RunFile( const char* const filename );
	void RunText( const char* const text );
	void Exit( int val );

	// add a built-in module to the list of importable modules
	void AddBuiltinModule( string name, import_module_fcn );
	// import a built-in module (calls the import_module_fcn for this module)
	bool ImportBuiltinModule( string name );
	// import all the functions defined in a built-in module
	bool ImportBuiltinModuleFunctions( string name, map<string, builtin_fcn> & fcns );

	// global error methods
	bool Error(){ return is_error; }
	void SetError( bool err )
	{
		// set the flag
		is_error = err;
		// if we're clearing the flag, we need to clear the data too
		if( !is_error )
		{
			delete error_data;
			error_data = NULL;
		}
	}
	void SetErrorData( const DevaObject & data )
	{
		error_data = new DevaObject( data );
	}
	DevaObject* GetErrorData(){ return error_data; }

	// add all the known built-in modules
	// (for language embedding clients such as deva, devadb etc)
	void AddAllKnownBuiltinModules();

	// dump the stack trace to an output stream
	void DumpTrace( ostream &, bool show_all_scopes = false );

	// dump (at most the top ten items from the) data stack to stdout
	void PrintDataStack();

	string GetExecutingFile(){ return file; }

	// get the current IP
	size_t GetIP(){ return ip; }

	// debugging methods
	////////////////////////////////////////////////////
	// start executing a file, stopping before the first instruction
	void StartExecutingCode( unsigned char* code );
	// execute one line
	int StepOver();
	// execute one line or into one call
	int StepInto();
	// execute one instruction
	int StepInst( Instruction & inst );
	// execute (with breakpoints enabled)
	int Run( bool stop_at_breakpoint = true );
	// add a breakpoint
	void AddBreakpoint( string file, int line );
	// enumerate breakpoints
	const vector<pair<string, int> > & GetBreakpoints();
	// remove a breakpoint
	void RemoveBreakpoint( int idx );

	////////////////////////////////////////////////////
	// helper methods. useful for built-ins
	////////////////////////////////////////////////////
	// locate a symbol in the symbol table(namespace)
	DevaObject* find_symbol( const DevaObject & ob, ScopeTable* scopes = NULL, bool search_all_modules = false );
	// locate a symbol looking in the current symbol table ONLY
	DevaObject* find_symbol_in_current_scope( const DevaObject & ob );

	// object must be evaluated already to a value (i.e. no variables)
	bool evaluate_object_as_boolean( DevaObject & o );
};

#endif // __EXECUTOR_H__
