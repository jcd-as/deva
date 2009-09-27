// executor.cpp
// deva language intermediate language & virtual machine execution engine
// created by jcs, september 26, 2009 

// TODO:
// * maps & vectors, including the dot operator and 'for' loops

#include "opcodes.h"
#include "symbol.h"
#include "types.h"
#include "exceptions.h"
#include "scope.h"
#include "instructions.h"
#include <vector>
#include <cstring>
#include <fstream>

using namespace std;

// "global" data items
///////////////////////////////////////////////////////////
// the data stack
static vector<DevaObject> stack;

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
static vector<Scope*> scopes;
///////////////////////////////////////////////////////////

// "global" utility functions
///////////////////////////////////////////////////////////
// utility function to locate a symbol in parent scopes
//static inline bool find_symbol_in_parent_scopes( DevaObject ob )
//{
//	if( scopes.size() == 0 )
//		return false;
//	for( vector<SymbolTable*>::reverse_iterator i = scopes.rbegin()+1; i < scopes.rend(); ++i )
//	{
//		// get the parent scope object
//		SymbolTable* p = *i;
//
//		// check for the symbol
//		if( p->count( ob.name ) != 0 )
//			return true;
//	}
//	return false;
//}

// locate a symbol
DevaObject* find_symbol( DevaObject ob )
{
	// check each scope on the stack
	for( vector<Scope*>::reverse_iterator i = scopes.rbegin(); i < scopes.rend(); ++i )
	{
		// get the scope object
		Scope* p = *i;

		// check for the symbol
		if( p->count( ob.name ) != 0 )
			return p->operator[]( ob.name );
	}
	return NULL;
}
///////////////////////////////////////////////////////////

// Op-code functions
///////////////////////////////////////////////////////////
// 0 pop top item off stack
DevaObject Pop( Instruction const & inst )
{
	if( stack.size() == 0 )
		throw DevaRuntimeException( "Pop operation executed on empty stack." );
	DevaObject temp = stack.back();
	stack.pop_back();
	return temp;
}
// 1 push item onto top of stack
void Push( Instruction const & inst )
{
	if( inst.args.size() > 1 )
		throw DevaRuntimeException( "Push instruction contains too many arguments." );
	stack.push_back( inst.args[0] );
}
// 2 load a variable from memory to the stack
void Load( Instruction const & inst )
{
	// TODO: implement. so far, this instruction is never generated
}
// 3 store a variable from the stack to memory
void Store( Instruction const & inst )
{
	// TODO: implement
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	DevaObject rhs = Pop( inst );
	DevaObject lhs = Pop( inst );
//    sym_null,			// 0
//    sym_number,			// 1
//    sym_string,			// 2
//	sym_boolean,		// 3
//    sym_vector,			// 4
//	sym_map,			// 5
//    sym_function,		// 6
//    sym_function_call,	// 7
//    sym_unknown,		// 8
	// if the rhs is a variable or function, get it from the symbol table
	// TODO: map & vector
	if( rhs.Type() == sym_unknown || rhs.Type() == sym_function )
	{
		// if not found, error
		DevaObject* ob = find_symbol( rhs );
		if( !ob )
			throw DevaRuntimeException( "Reference to unknown variable." );
	}
	// verify the lhs is a variable (sym_unknown)
	if( lhs.Type() != sym_unknown )
		throw DevaRuntimeException( "Attempting to assign a value into a non-variable l-value." );
	// get the lhs from the symbol table
	DevaObject* ob = find_symbol( lhs );
	//  - not found? add it to the current scope
	if( !ob )
	{
		ob = new DevaObject( lhs.name, sym_unknown );
		scopes.back()->insert( pair<string, DevaObject*>(ob->name, ob) );
	}
	//  set its value to the rhs
	*ob = rhs;
}
// 4 define function. arg is location in instruction stream, named the fcn name
void Defun( Instruction const & inst )
{
	// TODO: implement
}
// 5 define an argument to a fcn. argument (to opcode) is arg name
void Defarg( Instruction const & inst )
{
	// TODO: implement
}
// 6 create a new object and place on top of stack
void New( Instruction const & inst )
{
	// TODO: implement
}
// 7 create a new map object and push onto stack
void New_map( Instruction const & inst )
{
	// TODO: implement
}
// 8 create a new vector object and push onto stack
void New_vec( Instruction const & inst )
{
	// TODO: implement
}
// 9 get item from vector
void Vec_load( Instruction const & inst )
{
	// TODO: implement
}
// 10 set item in vector. args: index, value
void Vec_store( Instruction const & inst )
{
	// TODO: implement
}
// 11 get item from map
void Map_load( Instruction const & inst )
{
	// TODO: implement
}
// 12 set item in map. args: index, value
void Map_store( Instruction const & inst )
{
	// TODO: implement
}
// 13 unconditional jump to the address on top of the stack
void Jmp( Instruction const & inst )
{
	// TODO: implement
}
// 14 jump on top of stack evaluating to false 
void Jmpf( Instruction const & inst )
{
	// TODO: implement
}
// 15 == compare top two values on stack
void Eq( Instruction const & inst )
{
	// TODO: implement
}
// 16 != compare top two values on stack
void Neq( Instruction const & inst )
{
	// TODO: implement
}
// 17 < compare top two values on stack
void Lt( Instruction const & inst )
{
	// TODO: implement
}
// 18 <= compare top two values on stack
void Lte( Instruction const & inst )
{
	// TODO: implement
}
// 19 > compare top two values on stack
void Gt( Instruction const & inst )
{
	// TODO: implement
}
// 20 >= compare top two values on stack
void Gte( Instruction const & inst )
{
	// TODO: implement
}
// 21 || the top two values
void Or( Instruction const & inst )
{
	// TODO: implement
}
// 22 && the top two values
void And( Instruction const & inst )
{
	// TODO: implement
}
// 23 negate the top value ('-' operator)
void Neg( Instruction const & inst )
{
	// TODO: implement
}
// 24 boolean not the top value ('!' operator)
void Not( Instruction const & inst )
{
	// TODO: implement
}
// 25 add top two values on stack
void Add( Instruction const & inst )
{
	// TODO: implement
}
// 26 subtract top two values on stack
void Sub( Instruction const & inst )
{
	// TODO: implement
}
// 27 multiply top two values on stack
void Mul( Instruction const & inst )
{
	// TODO: implement
}
// 28 divide top two values on stack
void Div( Instruction const & inst )
{
	// TODO: implement
}
// 29 modulus top two values on stack
void Mod( Instruction const & inst )
{
	// TODO: implement
}
// 30 dump top of stack to stdout
void Output( Instruction const & inst )
{
	// TODO: implement
}
// 31 call a function. arguments on stack
void Call( Instruction const & inst )
{
	// TODO: implement
}
// 32 pop the return address and unconditionally jump to it
void Return( Instruction const & inst )
{
	// TODO: implement
}
// 33 as return, but stack holds return value and then (at top) return address
void Returnv( Instruction const & inst )
{
	// TODO: implement
}
// 34 break out of loop, respecting scope (enter/leave)
void Break( Instruction const & inst )
{
	// TODO: implement
}
// 35 enter new scope
void Enter( Instruction const & inst )
{
	// TODO: implement
}
// 36 leave scope
void Leave( Instruction const & inst )
{
	// TODO: implement
}
// 37 no op
void Nop( Instruction const & inst )
{
	// TODO: implement
}
// 38 finish program, 0 or 1 ops (return code)
void Halt( Instruction const & inst )
{
	// TODO: implement
}

// illegal operation, if exists there was a compiler error/fault
void Illegal( Instruction const & inst )
{
	throw DevaRuntimeException( "Illegal Instruction" );
}
///////////////////////////////////////////////////////////


void DoInstr( Instruction inst )
{
	// TODO: implement
//		for( vector<DevaObject>::iterator j = inst.args.begin(); j != inst.args.end(); ++j )
//			cout << *j;
	switch( inst.op )
	{
	case op_pop:			// 0 pop top item off stack
		break;
	case op_push:		// 1 push item onto top of stack
		break;
	case op_load:		// 2 load a variable from memory to the stack
		break;
	case op_store:		// 3 store a variable from the stack to memory
		break;
	case op_defun:		// 4 define function. arg is location in instruction stream, named the fcn name
		break;
	case op_defarg:		// 5 define an argument to a fcn. argument (to opcode) is arg name
		break;
	case op_new:			// 6 create a new object and place on top of stack
		break;
	case op_new_map:		// 7 create a new map object and push onto stack
		break;
	case op_new_vec:		// 8 create a new vector object and push onto stack
		break;
	case op_vec_load:	// 9 get item from vector
		break;
	case op_vec_store:	// 10 set item in vector. args: index, value
		break;
	case op_map_load:	// 11 get item from map
		break;
	case op_map_store:	// 12 set item in map. args: index, value
		break;
	case op_jmp:			// 13 unconditional jump to the address on top of the stack
		break;
	case op_jmpf:		// 14 jump on top of stack evaluating to false 
		break;
	case op_eq:			// 15 == compare top two values on stack
		break;
	case op_neq:			// 16 != compare top two values on stack
		break;
	case op_lt:			// 17 < compare top two values on stack
		break;
	case op_lte:			// 18 <= compare top two values on stack
		break;
	case op_gt:			// 19 > compare top two values on stack
		break;
	case op_gte:			// 20 >= compare top two values on stack
		break;
	case op_or:			// 21 || the top two values
		break;
	case op_and:			// 22 && the top two values
		break;
	case op_neg:			// 23 negate the top value ('-' operator)
		break;
	case op_not:			// 24 boolean not the top value ('!' operator)
		break;
	case op_add:			// 25 add top two values on stack
		break;
	case op_sub:			// 26 subtract top two values on stack
		break;
	case op_mul:			// 27 multiply top two values on stack
		break;
	case op_div:			// 28 divide top two values on stack
		break;
	case op_mod:			// 29 modulus top two values on stack
		break;
	case op_output:		// 30 dump top of stack to stdout
		break;
	case op_call:		// 31 call a function. arguments on stack
		// TODO: special case "print" built-in for testing purposes
		break;
	case op_return:		// 32 pop the return address and unconditionally jump to it
		break;
	case op_returnv:		// 33 as return, but stack holds return value and then (at top) return address
		break;
	case op_break:		// 34 break out of loop, respecting scope (enter/leave)
		 break;
	case op_enter:		// 35 enter new scope
		 break;
	case op_leave:		// 36 leave scope
		 break;
	case op_nop:			// 37 no op
		 break;
	case op_halt:		// 38 finish program, 0 or 1 ops (return code)
		 break;
	case op_illegal:	// illegal operation, if exists there was a compiler error/fault
		break;
	}
}

bool RunFile( char const* filename )
{
	// TODO: implement
	//
	// open the file for reading
	ifstream file;
	file.open( filename, ios::binary );
	if( file.fail() )
		throw DevaRuntimeException( "Unable to open input file for read." );

	// read the header
	char deva[5] = {0};
	char ver[6] = {0};
	file.read( deva, 5 );
	if( strcmp( deva, "deva") != 0 )
		throw DevaRuntimeException( "Invalid .dvc file: header missing 'deva' tag." );
	file.read( ver, 6 );
	if( strcmp( ver, "1.0.0" ) != 0 )
		 throw DevaRuntimeException( "Invalid .dvc version number." );
	char pad[6] = {0};
	file.read( pad, 5 );
	if( pad[0] != 0 || pad[1] != 0 || pad[2] != 0 || pad[3] != 0 || pad[4] != 0 )
		throw DevaRuntimeException( "Invalid .dvc file: malformed header after version number." );

	// read the instructions
	char* name = new char[256];
	while( !file.eof() )
	{
		double num_val;
		char str_val[256] = {0};
		long bool_val;
//		map<DevaObject, DevaObject>* map_val;
//		vector<DevaObject>* vec_val;
//		Function* func_val;
		// read the byte for the opcode
		unsigned char op;
		file.read( (char*)&op, 1 );
		Instruction inst( (Opcode)op );
		// for each argument:
		unsigned char type;
		file.read( (char*)&type, 1 );
		while( type != sym_end )
		{
			// read the name of the arg
			memset( name, 0, 256 );
			file.getline( name, 256, '\0' );
			// read the value
			switch( (SymbolType)type )
			{
				case sym_number:
					{
					// must use >> op to read, since it was used to write
					file >> num_val;
					DevaObject ob( name, num_val );
					inst.args.push_back( ob );
					break;
					}
				case sym_string:
					{
					// variable length, null-terminated
					// TODO: fix. strings can be any length!
					file.getline( str_val, 256, '\0' );
					DevaObject ob( name, string( str_val ) );
					inst.args.push_back( ob );
					break;
					}
				case sym_boolean:
					{
					// must use >> op to read, since it was used to write
					file >> bool_val;
					DevaObject ob( name, (bool)bool_val );
					inst.args.push_back( ob );
					break;
					}
				case sym_map:
					// TODO: implement
					break;
				case sym_vector:
					// TODO: implement
					break;
				case sym_function:
					{
					// TODO: ???
//					DevaObject ob( name, sym_function );
//					inst.args.push_back( ob );
					break;
					}
				case sym_function_call:
					{
					// TODO: ???
					DevaObject ob( name, sym_function_call );
					inst.args.push_back( ob );
					break;
					}
				case sym_null:
					{
					// 32 bits
					long n = -1;
					file >> n;
					DevaObject ob( name, sym_null );
					inst.args.push_back( ob );
					break;
					}
				case sym_unknown:
					{
					// unknown (i.e. variable)
					DevaObject ob( name, sym_unknown );
					inst.args.push_back( ob );
					break;
					}
				default:
					// TODO: throw error
					break;
			}
			memset( name, 0, 256 );
			
			// read the type of the next arg
			//type = 'x';
			// default to sym_end to drop out of loop if we can't read a byte
			type = (unsigned char)sym_end;
			file.read( (char*)&type, sizeof( type ) );
		}

		DoInstr( inst );

//		cout << inst.op << " : ";
//		// dump args (vector of DevaObjects) too (need >> op for Objects)
//		for( vector<DevaObject>::iterator j = inst.args.begin(); j != inst.args.end(); ++j )
//			cout << *j;
//		cout << endl;
	}
	delete [] name;
	return true;
}

