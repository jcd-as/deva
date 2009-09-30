// executor.cpp
// deva language intermediate language & virtual machine execution engine
// created by jcs, september 26, 2009 

// TODO:
// * clean up & unify format of exceptions thrown
// * maps & vectors, including the dot operator and 'for' loops

#include "executor.h"


// private utility functions
///////////////////////////////////////////////////////////

// locate a symbol
DevaObject* Executor::find_symbol( DevaObject ob )
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

// peek at what the next instruction is (doesn't modify ip)
Opcode Executor::PeekInstr()
{
	return (Opcode)(*(code + ip));
}

// read a string from *ip into s
// (allocates s, which needs to be freed by the caller)
void Executor::read_string( char* & s )
{
	// determine how much space we need
	long len = strlen( (char*)(code + ip) );
	// allocate it
	s = new char[len+1];
	// copy the value
	memcpy( s, (char*)(code + ip), len );
	// null-termination
	s[len] = '\0';
	// move ip forward
	ip += len+1;
}

// read a byte
void Executor::read_byte( unsigned char & b )
{
	memcpy( (void*)&b, (char*)(code + ip), sizeof( unsigned char ) );
	ip += sizeof( unsigned char );
}

// read a long
void Executor::read_long( long & l )
{
	memcpy( (void*)&l, (char*)(code + ip), sizeof( long ) );
	ip += sizeof( long );
}

// read a double
void Executor::read_double( double & d )
{
	memcpy( (void*)&d, (char*)(code + ip), sizeof( double ) );
	ip += sizeof( double );
}

// load the bytecode from the file
void Executor::LoadByteCode()
{
	// open the file for reading
	ifstream file;
	file.open( filename.c_str(), ios::binary );
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

	// allocate memory for the byte code array
	filebuf* buf = file.rdbuf();
	// size of the byte code array is file length minus the header size (16
	// bytes)
	long len = buf->pubseekoff( 0, ios::end, ios::in );
	len -= 16;
	// (seek back to the end of the header)
	buf->pubseekpos( 16, ios::in );
	code = new unsigned char[len];

	// read the file into the byte code array
	file.read( (char*)code, len );

	// close the file
	file.close();
}

///////////////////////////////////////////////////////////

// Op-code functions
///////////////////////////////////////////////////////////

// 0 pop top item off stack
DevaObject Executor::Pop( Instruction const & inst )
{
	if( stack.size() == 0 )
		throw DevaRuntimeException( "Pop operation executed on empty stack." );
	DevaObject temp = stack.back();
	stack.pop_back();
	return temp;
}
// 1 push item onto top of stack
void Executor::Push( Instruction const & inst )
{
	if( inst.args.size() > 1 )
		throw DevaRuntimeException( "Push instruction contains too many arguments." );
	stack.push_back( inst.args[0] );
}
// 2 load a variable from memory to the stack
void Executor::Load( Instruction const & inst )
{
	// TODO: implement. so far, this instruction is never generated
}
// 3 store a variable from the stack to memory
void Executor::Store( Instruction const & inst )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	DevaObject rhs = Pop( inst );
	DevaObject lhs = Pop( inst );
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
	ob->SetValue( rhs );
}
// 4 define function. arg is location in instruction stream, named the fcn name
void Executor::Defun( Instruction const & inst )
{
	// TODO: implement
	// ensure 1st arg to instruction is a function object
	if( inst.args.size() < 1 )
		throw DevaRuntimeException( "Internal compiler error: Invalid defun opcode, no arguments" );
	if( inst.args[0].Type() != sym_function )
		throw DevaRuntimeException( "Internal compiler error: Invalid defun opcode argument" );
	// create a new entry in the local symbol table
	// (offset to HERE (function start))
	long offset = ip;
	// also the same as: long offset = inst.args[0].func_offset + inst.args[0].Size() + 2;
	DevaObject* fcn = new DevaObject( inst.args[0].name, offset );
	scopes.back()->insert( pair<string, DevaObject*>( fcn->name, fcn ) );
	// skip the function body
	Opcode op = PeekInstr();
	while( op != op_return && op != op_returnv )
	{
		NextInstr();
		op = PeekInstr();
	}
	NextInstr();
}
// 5 define an argument to a fcn. argument (to opcode) is arg name
void Executor::Defarg( Instruction const & inst )
{
	// TODO: implement
	// ??? when called as the start of a fcn, needs to operate differently than
	// when called to define the fcn... ???
	if( inst.args.size() < 1 )
		throw DevaRuntimeException( "Internal compiler error: Invalid defarg opcode, no arguments" );
	if( stack.size() < 1 )
		throw DevaRuntimeException( "Invalid defarg opcode, no data on stack" );
	// TODO: doh! return address is on top of the stack here, not the args
	// pop the top of the stack and put it in the symbol table with the name of
	DevaObject o = stack.back();
	stack.pop_back();
	// the argument that is being defined
	DevaObject* val = new DevaObject( inst.args[0].name, o );
	scopes.back()->insert( pair<string, DevaObject*>( val->name, val ) );
}
// 6 create a new object and place on top of stack
void Executor::New( Instruction const & inst )
{
	// TODO: implement
}
// 7 create a new map object and push onto stack
void Executor::New_map( Instruction const & inst )
{
	// TODO: implement
}
// 8 create a new vector object and push onto stack
void Executor::New_vec( Instruction const & inst )
{
	// TODO: implement
}
// 9 get item from vector
void Executor::Vec_load( Instruction const & inst )
{
	// TODO: implement
}
// 10 set item in vector. args: index, value
void Executor::Vec_store( Instruction const & inst )
{
	// TODO: implement
}
// 11 get item from map
void Executor::Map_load( Instruction const & inst )
{
	// TODO: implement
}
// 12 set item in map. args: index, value
void Executor::Map_store( Instruction const & inst )
{
	// TODO: implement
}
// 13 unconditional jump to the address on top of the stack
void Executor::Jmp( Instruction const & inst )
{
	// TODO: implement
}
// 14 jump on top of stack evaluating to false 
void Executor::Jmpf( Instruction const & inst )
{
	// TODO: implement
}
// 15 == compare top two values on stack
void Executor::Eq( Instruction const & inst )
{
	// TODO: implement
}
// 16 != compare top two values on stack
void Executor::Neq( Instruction const & inst )
{
	// TODO: implement
}
// 17 < compare top two values on stack
void Executor::Lt( Instruction const & inst )
{
	// TODO: implement
}
// 18 <= compare top two values on stack
void Executor::Lte( Instruction const & inst )
{
	// TODO: implement
}
// 19 > compare top two values on stack
void Executor::Gt( Instruction const & inst )
{
	// TODO: implement
}
// 20 >= compare top two values on stack
void Executor::Gte( Instruction const & inst )
{
	// TODO: implement
}
// 21 || the top two values
void Executor::Or( Instruction const & inst )
{
	// TODO: implement
}
// 22 && the top two values
void Executor::And( Instruction const & inst )
{
	// TODO: implement
}
// 23 negate the top value ('-' operator)
void Executor::Neg( Instruction const & inst )
{
	// TODO: implement
}
// 24 boolean not the top value ('!' operator)
void Executor::Not( Instruction const & inst )
{
	// TODO: implement
}
// 25 add top two values on stack
void Executor::Add( Instruction const & inst )
{
	// TODO: implement
	if( stack.size() < 2 )
		throw DevaRuntimeException( "Not enough items on stack for Add operation." );
	// add the top two items on the stack
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the args are variables, look them up in the symbol table
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Right-hand operand for Add operation not found in symbol table." );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Left-hand operand for Add operation not found in symbol table." );
		lhs = *o;
	}
	// do the add
	if( lhs.Type() == sym_number )
	{
		if( rhs.Type() != sym_number )
			throw DevaRuntimeException( "Invalid right-hand argument type for Add operation." );
		double ret = lhs.num_val + rhs.num_val;
		stack.push_back( DevaObject( "", ret ) );
	}
	// (or, if they are strings concatenate them)
	else if( lhs.Type() == sym_string )
	{
		if( rhs.Type() != sym_string )
			throw DevaRuntimeException( "Invalid right-hand argument type for Add operation." );
		string ret( lhs.str_val );
		ret += rhs.str_val;
		stack.push_back( DevaObject( "", ret ) );
	}
	else
		throw DevaRuntimeException( "Invalid argument types for Add operation." );
}
// 26 subtract top two values on stack
void Executor::Sub( Instruction const & inst )
{
	// TODO: implement
}
// 27 multiply top two values on stack
void Executor::Mul( Instruction const & inst )
{
	// TODO: implement
}
// 28 divide top two values on stack
void Executor::Div( Instruction const & inst )
{
	// TODO: implement
}
// 29 modulus top two values on stack
void Executor::Mod( Instruction const & inst )
{
	// TODO: implement
}
// 30 dump top of stack to stdout
void Executor::Output( Instruction const & inst )
{
	// TODO: implement
}
// 31 call a function. arguments on stack
void Executor::Call( Instruction const & inst )
{
	// TODO: implement
	// TEMPORARY HACK!!! REMOVE!!!
	// (and replace with importing the built-ins)
	if( inst.args[0].name == "print" )
	{
		// get the argument off the stack
		DevaObject obj = Pop( inst );
		// if it's a variable, locate it in the symbol table
		DevaObject* o = NULL;
		if( obj.Type() == sym_unknown )
		{
			o = find_symbol( obj );
			if( !o )
				throw DevaRuntimeException( "Symbol not found in function call" );
			
		}
		if( !o )
			o = &obj;
		// evaluate it
		switch( o->Type() )
		{
			case sym_number:
				cout << o->num_val;
				break;
			case sym_string:
				cout << o->str_val;
				break;
			case sym_boolean:
				if( o->bool_val )
					cout << "true";
				else
					cout << "false";
				break;
			case sym_null:
				cout << "null";
				break;
			case sym_map:
				// TODO: dump some map contents?
				cout << "map: '" << o->name << "' = ";
				break;
			case sym_vector:
				// TODO: dump some vector contents?
				cout << "vector: '" << o->name << "' = ";
				break;
			case sym_function:
				cout << "function: '" << o->name << "'";
				break;
			case sym_function_call:
				cout << "function_call: '" << o->name << "'";
				break;
			default:
				cout << "unknown: '" << o->name << "'";
		}
		// print it
		cout << endl;
	}
	else
	{
		// look up the name in the symbol table
		DevaObject* fcn = find_symbol( inst.args[0] );
		if( !fcn )
			throw DevaRuntimeException( "Call made to undefined function." );
		// get the offset for the function
		long offset = fcn->func_offset;
		// TODO:
		// push the current location onto the stack
//		stack.push_back( DevaObject( "", ip ) );
		// jump execution to the function offset
		ip = offset;
	}
}
// 32 pop the return address and unconditionally jump to it
void Executor::Return( Instruction const & inst )
{
	// TODO: validate
	// pop the return location off the top of the stack
	DevaObject ob = stack.back();
	stack.pop_back();
	if( ob.Type() != sym_function )
		throw DevaRuntimeException( "Invalid return destination on stack when executing return instruction" );
	// jump to the return location
	long offset = ob.func_offset;
	ip = offset;
}
// 33 as return, but stack holds return value and then (at top) return address
void Executor::Returnv( Instruction const & inst )
{
	// TODO: validate
	// TODO: ??? deal with the return value ???
	// the return value is now on top of the stack, instead of the return
	// address, so we need to pop it, pop the return address and then re-push
	// the return value
	DevaObject ret = stack.back();
	stack.pop_back();
	// pop the return location off the top of the stack
	DevaObject ob = stack.back();
	stack.pop_back();
	if( ob.Type() != sym_function )
		throw DevaRuntimeException( "Invalid return destination on stack when executing return instruction" );
	stack.push_back( ret );
	// jump to the return location
	long offset = ob.func_offset;
	ip = offset;
}
// 34 break out of loop, respecting scope (enter/leave)
void Executor::Break( Instruction const & inst )
{
	// TODO: implement
}
// 35 enter new scope
void Executor::Enter( Instruction const & inst )
{
	// TODO: ???
	scopes.push_back( new Scope() );
}
// 36 leave scope
void Executor::Leave( Instruction const & inst )
{
	// TODO: ???
	scopes.pop_back();
}
// 37 no op
void Executor::Nop( Instruction const & inst )
{
	// TODO: implement
}
// 38 finish program, 0 or 1 ops (return code)
void Executor::Halt( Instruction const & inst )
{
	// TODO: implement
}
// illegal operation, if exists there was a compiler error/fault
void Executor::Illegal( Instruction const & inst )
{
	throw DevaRuntimeException( "Illegal Instruction" );
}
///////////////////////////////////////////////////////////


// execute single instruction
bool Executor::DoInstr( Instruction inst )
{
	// TODO: implement
//		for( vector<DevaObject>::iterator j = inst.args.begin(); j != inst.args.end(); ++j )
//			cout << *j;
	switch( inst.op )
	{
	case op_pop:			// 0 pop top item off stack
		Pop( inst );
		break;
	case op_push:		// 1 push item onto top of stack
		Push( inst );
		break;
	case op_load:		// 2 load a variable from memory to the stack
		Load( inst );
		break;
	case op_store:		// 3 store a variable from the stack to memory
		Store( inst );
		break;
	case op_defun:		// 4 define function. arg is location in instruction stream, named the fcn name
		Defun( inst );
		break;
	case op_defarg:		// 5 define an argument to a fcn. argument (to opcode) is arg name
		Defarg( inst );
		break;
	case op_new:			// 6 create a new object and place on top of stack
		New( inst );
		break;
	case op_new_map:		// 7 create a new map object and push onto stack
		New_map( inst );
		break;
	case op_new_vec:		// 8 create a new vector object and push onto stack
		New_vec( inst );
		break;
	case op_vec_load:	// 9 get item from vector
		Vec_load( inst );
		break;
	case op_vec_store:	// 10 set item in vector. args: index, value
		Vec_store( inst );
		break;
	case op_map_load:	// 11 get item from map
		Map_load( inst );
		break;
	case op_map_store:	// 12 set item in map. args: index, value
		Map_store( inst );
		break;
	case op_jmp:			// 13 unconditional jump to the address on top of the stack
		Jmp( inst );
		break;
	case op_jmpf:		// 14 jump on top of stack evaluating to false 
		Jmpf( inst );
		break;
	case op_eq:			// 15 == compare top two values on stack
		Eq( inst );
		break;
	case op_neq:			// 16 != compare top two values on stack
		Neq( inst );
		break;
	case op_lt:			// 17 < compare top two values on stack
		Lt( inst );
		break;
	case op_lte:			// 18 <= compare top two values on stack
		Lte( inst );
		break;
	case op_gt:			// 19 > compare top two values on stack
		Gt( inst );
		break;
	case op_gte:			// 20 >= compare top two values on stack
		Gte( inst );
		break;
	case op_or:			// 21 || the top two values
		Or( inst );
		break;
	case op_and:			// 22 && the top two values
		And( inst );
		break;
	case op_neg:			// 23 negate the top value ('-' operator)
		Neg( inst );
		break;
	case op_not:			// 24 boolean not the top value ('!' operator)
		Not( inst );
		break;
	case op_add:			// 25 add top two values on stack
		Add( inst );
		break;
	case op_sub:			// 26 subtract top two values on stack
		Sub( inst );
		break;
	case op_mul:			// 27 multiply top two values on stack
		Mul( inst );
		break;
	case op_div:			// 28 divide top two values on stack
		Div( inst );
		break;
	case op_mod:			// 29 modulus top two values on stack
		Mod( inst );
		break;
	case op_output:		// 30 dump top of stack to stdout
		Output( inst );
		break;
	case op_call:		// 31 call a function. arguments on stack
		Call( inst );
		break;
	case op_return:		// 32 pop the return address and unconditionally jump to it
		Return( inst );
		break;
	case op_returnv:		// 33 as return, but stack holds return value and then (at top) return address
		Returnv( inst );
		break;
	case op_break:		// 34 break out of loop, respecting scope (enter/leave)
		Break( inst );
		break;
	case op_enter:		// 35 enter new scope
		Enter( inst );
		break;
	case op_leave:		// 36 leave scope
		Leave( inst );
		break;
	case op_nop:			// 37 no op
		Nop( inst );
		break;
	case op_halt:		// 38 finish program, 0 or 1 ops (return code)
		Halt( inst );
		return false;
	case op_illegal:	// illegal operation, if exists there was a compiler error/fault
	default:
		Illegal( inst );
		break;
	}
	return true;
}

// skip the next instruction (increment ip to the next instruction)
Instruction Executor::NextInstr()
{
	char* name = NULL;
	double num_val;
	char* str_val = NULL;
	long bool_val;
//		map<DevaObject, DevaObject>* map_val;
//		vector<DevaObject>* vec_val;
	long func_offset;
	// read the byte for the opcode
	unsigned char op;
	read_byte( op );
	Instruction inst( (Opcode)op );
	// for each argument:
	unsigned char type;
	read_byte( type );
	while( type != sym_end )
	{
		// read the name of the arg
		read_string( name );
		// read the value
		switch( (SymbolType)type )
		{
			case sym_number:
				{
				// 64 bit double
				read_double( num_val );
				DevaObject ob( name, num_val );
				inst.args.push_back( ob );
				break;
				}
			case sym_string:
				{
				// variable length, null-terminated
				read_string( str_val );
				DevaObject ob( name, string( str_val ) );
				delete [] str_val;
				inst.args.push_back( ob );
				break;
				}
			case sym_boolean:
				{
				// 32 bit long
				read_long( bool_val );
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
				// 32 bit long
				read_long( func_offset );
				DevaObject ob( name, (long)func_offset );
				inst.args.push_back( ob );
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
				// 32 bit long
				long n = -1;
				read_long( n );
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
		delete [] name;
		
		// read the type of the next arg
		//type = 'x';
		// default to sym_end to drop out of loop if we can't read a byte
		type = (unsigned char)sym_end;
		read_byte( type );
	}
	return inst;
}

// public methods
///////////////////////////////////////////////////////////
Executor::Executor( string fname ) : filename( fname ), code( NULL ), ip( 0 )
{}

Executor::~Executor()
{
	if( code )
		delete [] code;
}

bool Executor::RunFile()
{
	// load the file into memory
	LoadByteCode();
	
	// create a file-level ("global") scope
	Enter( Instruction( op_enter ) );

	// read the instructions
	while( true )
	{
		// get the next instruction in the byte code
		Instruction inst = NextInstr();

		// DoInstr returns false on 'halt' instruction
		if( !DoInstr( inst ) )
			break;
	}

	// exit the file-level ("global") scope
	Leave( Instruction( op_leave ) );

	return true;
}

