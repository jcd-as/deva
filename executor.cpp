// executor.cpp
// deva language intermediate language & virtual machine execution engine
// created by jcs, september 26, 2009 

// TODO:
// * 'call stack' for tracking where errors occur (rudimentary debugging support)

#include "executor.h"
#include "builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"
#include "string_builtins.h"
#include "compile.h"
#include "fileformat.h"
#include "util.h"


// static data
///////////////////////////////////////////////////////////
int Executor::args_on_stack = -1;

// private utility functions
///////////////////////////////////////////////////////////

ostream & operator << ( ostream & os, Instruction & inst )
{
	// write the operator
	os << inst.op << " : ";
	// write each arg
	for( int i = 0; i < inst.args.size(); ++i )
	{
		os << inst.args[i] << " | ";
	}
	return os;
}

// locate a symbol
DevaObject* Executor::find_symbol( const DevaObject & ob, ScopeTable* scopes /*= NULL*/ )
{
	if( !scopes )
		scopes = current_scopes;
	// check each scope on the stack
	for( vector<Scope*>::reverse_iterator i = scopes->rbegin(); i < scopes->rend(); ++i )
	{
		// get the scope object
		Scope* p = *i;

		// check for the symbol
		if( p->count( ob.name ) != 0 )
			return p->find( ob.name )->second;
	}
	return NULL;
}

// remove a symbol, by name
void Executor::remove_symbol( const DevaObject & ob, ScopeTable* scopes /*= NULL*/ )
{
	if( !scopes )
		scopes = current_scopes;
	// check each scope on the stack
	for( vector<Scope*>::reverse_iterator i = scopes->rbegin(); i < scopes->rend(); ++i )
	{
		// get the scope object
		Scope* p = *i;

		// check for the symbol
		if( p->count( ob.name ) != 0 )
		{
			// delete the symbol
			delete p->find( ob.name )->second;

			// remove the symbol from the scope
			p->erase( ob.name );
		}
	}
}

// peek at what the next instruction is (doesn't modify ip)
Opcode Executor::PeekInstr()
{
	return (Opcode)(*((unsigned char*)ip));
}

// read a string from *ip into s
// (allocates s, which needs to be freed by the caller)
void Executor::read_string( char* & s )
{
	// determine how much space we need
	long len = strlen( (char*)(ip) );
	// allocate it
	s = new char[len+1];
	// copy the value
	memcpy( s, (char*)(ip), len );
	// null-termination
	s[len] = '\0';
	// move ip forward
	ip += len+1;
}

// read a byte
void Executor::read_byte( unsigned char & b )
{
	memcpy( (void*)&b, (char*)(ip), sizeof( unsigned char ) );
	ip += sizeof( unsigned char );
}

// read a long
void Executor::read_long( long & l )
{
	memcpy( (void*)&l, (char*)(ip), sizeof( long ) );
	ip += sizeof( long );
}

// read a size_t
void Executor::read_size_t( size_t & l )
{
	memcpy( (void*)&l, (char*)(ip), sizeof( size_t ) );
	ip += sizeof( size_t );
}

// read a double
void Executor::read_double( double & d )
{
	memcpy( (void*)&d, (char*)(ip), sizeof( double ) );
	ip += sizeof( double );
}

// helper for the comparison ops:
// returns 0 if equal, -1 if lhs < rhs, +1 if lhs > rhs
int Executor::compare_objects( DevaObject & lhs, DevaObject & rhs )
{
	// nulls, numbers, booleans, and strings can be compared
	if( lhs.Type() != sym_number && lhs.Type() != sym_boolean && lhs.Type() != sym_string && lhs.Type() != sym_null )
		throw DevaRuntimeException( "Invalid left-hand argument to comparison operation." );
	if( rhs.Type() != sym_number && rhs.Type() != sym_boolean && rhs.Type() != sym_string && rhs.Type() != sym_null )
		throw DevaRuntimeException( "Invalid right-hand argument to comparison operation." );
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Left-hand side and right-hand side of comparison operation are incompatible." );
	switch( lhs.Type() )
	{
	case sym_number:
		// NOTE: naturally this doesn't work *properly* for floating point
		// numbers. however, it really doesn't matter, as this method is only
		// used for sorting items in a map, which can be completely arbitrary
		// as long as it is consistent
		if( lhs.num_val == rhs.num_val )
			return 0;
		else if( lhs.num_val > rhs.num_val )
			return 1;
		else
			return -1;
	case sym_boolean:
		if( lhs.bool_val == rhs.bool_val )
			return 0;
		else if( lhs.bool_val == true )
			return 1;
		else
			return -1;
	case sym_string:
		{
		string lhs_s( lhs.str_val );
		string rhs_s( rhs.str_val );
		return lhs_s.compare( rhs_s );
		}
	case sym_null:
		if( rhs.Type() == sym_null )
			return 0;
		else
			return 1;
	}
}

// evaluate an object as a boolean value
// object must be evaluated already to a value (i.e. no variables)
bool Executor::evaluate_object_as_boolean( DevaObject & o )
{
	// it must be a number, string, boolean, or null
	if( o.Type() != sym_number && o.Type() != sym_string
		&& o.Type() != sym_boolean && o.Type() != sym_null )
		throw DevaRuntimeException( "Type of condition for jmpf cannot be evaluated to a true/false value." );
	switch( o.Type() )
	{
	case sym_null:
		// null is always false
		return false;
	case sym_number:
		// TODO: use epsilon diff for comparing floating point numbers
		if( o.num_val != 0 )
			return false;
		else
			return true;
	case sym_boolean:
		return o.bool_val;
	case sym_string:
		if( strlen( o.str_val ) > 0 )
			return true;
		else
			return false;
	default:
		throw DevaRuntimeException( "Invalid type in boolean conditional." );
	}
}

// load the bytecode from the file
unsigned char* Executor::LoadByteCode( const char* const filename )
{
	// open the file for reading
	ifstream file;
	file.open( filename, ios::binary );
	if( file.fail() )
		throw DevaRuntimeException( "Unable to open input file for read." );

	unsigned char* bytecode = NULL;

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
	bytecode = new unsigned char[len];

	// read the file into the byte code array
	file.read( (char*)bytecode, len );

	// close the file
	file.close();

	return bytecode;
}

void Executor::FixupOffsets()
{
	char* name;
	// read the instructions
	while( true )
	{
		name = NULL;

		// read the byte for the opcode
		unsigned char op;
		read_byte( op );
		// stop when we get to the halt instruction at the end of the buffer
		if( op == op_halt )
			break;

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
					// skip double
					ip += sizeof( double );
					break;
					}
				case sym_string:
					{
					// variable length, null-terminated
					// skip string
					long len = strlen( (char*)(ip) );
					ip += len+1;
					break;
					}
				case sym_boolean:
					{
					// skip long
					ip += sizeof( long );
					break;
					}
				case sym_offset:
					{
					// TODO: select on which ops DO need to be fixed up, not
					// which don't (jmp, jmpf
					// don't fix-up line numbers or calls (number of args)
					// or new_instances (number of args)
					// THIS IS MISSING SOMETHING (try running all tests)
//					if( op == op_defun || op == op_jmp || op == op_jmpf )
					if( op != op_line_num && op != op_call && op != op_new_instance )
					{
						// read a size_t to get the offset
						size_t func_offset;
						memcpy( (void*)&func_offset, (char*)(ip), sizeof( size_t ) );
						// calculate the actual address
						size_t address = (size_t)code + func_offset;
						// 'fix-up' the 'bytecode' with the actual address
						size_t* p_add = (size_t*)ip;
						*p_add = address;
					}
					ip += sizeof( size_t );
					break;
					}
				case sym_null:
					{
					// skip a long
					ip += sizeof( long );
					break;
					}
				default:
					// TODO: throw error
					break;
			}
			delete [] name;
			// read the type of the next arg
			// default to sym_end to drop out of loop if we can't read a byte
			type = (unsigned char)sym_end;
			read_byte( type );
		}
	}
}

///////////////////////////////////////////////////////////

// Op-code functions
///////////////////////////////////////////////////////////

// 0 pop top item off stack
void Executor::Pop( Instruction const & inst )
{
	if( stack.size() == 0 )
		throw DevaRuntimeException( "Pop operation executed on empty stack." );
	stack.pop_back();
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
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid 'load' instruction: missing operand." );
	// load arg onto top of stack
	DevaObject obj = inst.args[0];
	if( obj.Type() != sym_unknown )
		throw DevaICE( "Invalid 'load' instruction: invalid argument type; must be a variable." );
	DevaObject* var = find_symbol( obj );
	if( !var )
		throw DevaRuntimeException( "Invalid argument to 'load' instruction: variable not found." );
	stack.push_back( *var );
}
// 3 store a variable from the stack to memory
void Executor::Store( Instruction const & inst )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the rhs is a variable or function, get it from the symbol table
	if( rhs.Type() == sym_unknown || rhs.Type() == sym_offset )
	{
		DevaObject* ob = find_symbol( rhs );
		if( !ob )
			throw DevaRuntimeException( "Reference to unknown variable." );
		rhs = *ob;
	}
	// verify the lhs is a variable (sym_unknown)
	if( lhs.Type() != sym_unknown )
		throw DevaRuntimeException( "Attempting to assign a value into a non-variable l-value." );
	// get the lhs from the symbol table
	DevaObject* ob = find_symbol( lhs );
	// not found? add it to the current scope
	if( !ob )
	{
		ob = new DevaObject( lhs.name, rhs );
		current_scopes->AddObject( ob );
	}
	//  set its value to the rhs
	else
	{
		*ob = DevaObject( lhs.name, rhs );
	}
}
// 4 define function. arg is location in instruction stream, named the fcn name
void Executor::Defun( Instruction const & inst )
{
	// ensure 1st arg to instruction is a function object
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid defun opcode, no arguments." );
	if( inst.args[0].Type() != sym_offset )
		throw DevaICE( "Invalid defun opcode argument." );
	// create a new entry in the local symbol table
	// (offset to HERE (function start))
	size_t offset = ip;
	// also the same as: long offset = inst.args[0].func_offset + inst.args[0].Size() + 2;
	DevaObject* fcn = new DevaObject( inst.args[0].name, offset );
	current_scopes->AddObject( fcn );
	// skip the function body
	Opcode op = PeekInstr();
	while( op != op_return )
	{
		NextInstr();
		op = PeekInstr();
	}
	NextInstr();
}
// 5 define an argument to a fcn. argument (to opcode) is arg name
void Executor::Defarg( Instruction const & inst )
{
	// only called as the start of a fcn *call*, not definition (defun)

	// if args_on_stack is -1 then we somehow ended up here without going
	// through a 'call' instruction first!
	if( args_on_stack == -1 )
		throw DevaICE( "'defarg' instruction processed without previous 'call' instruction. Program state is corrupt." );
	// if there are no arguments, this is the "end-of-arguments" marker,
	// check to see if all the args have been removed from the stack
	if( inst.args.size() < 1 )
	{
		if( args_on_stack != 0 )
			throw DevaRuntimeException( "Too many arguments passed to function." );
		// if we processed the correct number of arguments already, do nothing
		else
			return;
	}
	// TODO: stack *can* be empty if this is a default arg
	if( stack.size() < 1 )
		throw DevaRuntimeException( "Invalid 'defarg' opcode, no data on stack." );

	// if there are no args left, use the default
	DevaObject o;
	if( args_on_stack == 0 )
	{
		// if there is no default argument
		if( inst.args.size() < 2 )
			throw DevaRuntimeException( "Not enough arguments passed to function." );
		o = inst.args[1];
	}
	// otherwise get it from the stack
	else
	{
		// pop the top of the stack and put it in the symbol table with the name of
		// the argument that is being defined
		o = stack.back();
		stack.pop_back();

		// one less arg on the stack
		--args_on_stack;
	}

	// if it's a variable, look it up in the symbol table(s)
	if( o.Type() == sym_unknown )
	{
		DevaObject *var = find_symbol( o );
		if( !var )
			throw DevaRuntimeException( "Undefined variable used as function argument." );
		o = *var;
	}
	// check for the symbol in the immediate (current) scope and make sure
	// we're not redef'ing the same argument (shouldn't happen, the semantic
	// checker looks for this)
	if( current_scopes->back()->count( inst.args[0].name ) != 0 )
		throw DevaICE( "Argument with this name already exists." );
	DevaObject* val = new DevaObject( inst.args[0].name, o );
	current_scopes->AddObject( val );
}
// 6 dup a stack item from 'arg' position to the top of the stack
// (e.g. 'dup 0' duplicates the item on top of the stack)
void Executor::Dup( Instruction const & inst )
{
	if( inst.args.size() != 1 )
		throw DevaICE( "Invalid 'dup' instruction: no argument given." );
	DevaObject arg = inst.args[0];
	if( arg.Type() != sym_number )
		throw DevaICE( "Invalid 'dup' instruction: non-numeric argument." );
	DevaObject o = stack[stack.size() - (arg.num_val+1)];
	stack.push_back( o );
}
// 7 create a new map object and push onto stack
void Executor::New_map( Instruction const & inst )
{
	// variable to store as (name) is on top of the stack
	DevaObject lhs = stack.back();
	stack.pop_back();
	// ensure it's a variable
	if( lhs.Type() != sym_unknown )
		throw DevaRuntimeException( "Invalid left-hand type for assignment to new map object." );
	// create a new map object
	DevaObject *mp = new DevaObject( lhs.name, sym_map );

	// get the lhs from the symbol table
	DevaObject* ob = find_symbol( lhs );
	// not found? add it to the current scope
	if( !ob )
	{
		current_scopes->AddObject( mp );
	}
	//  set its value to the rhs
	else
	{
		*ob = DevaObject( lhs.name, *mp );
	}
}
// 8 create a new vector object and push onto stack
void Executor::New_vec( Instruction const & inst )
{
	// variable to store as (name) is on top of the stack
	DevaObject lhs = stack.back();
	stack.pop_back();
	// ensure it's a variable
	if( lhs.Type() != sym_unknown )
		throw DevaRuntimeException( "Invalid left-hand type for assignment to new vector object." );
	// create a new vector object 
	DevaObject *vec = new DevaObject( lhs.name, sym_vector );

	// get the lhs from the symbol table
	DevaObject* ob = find_symbol( lhs );
	// not found? add it to the current scope
	if( !ob )
	{
		current_scopes->AddObject( vec );
	}
	//  set its value to the rhs
	else
	{
		*ob = DevaObject( lhs.name, *vec );
	}
}
// 9 get item from vector or map
// (can't tell at compile time what it will be)
// if there is an arg that is the boolean 'true', then we need to look up items in
// maps as if the key was a numeric index (i.e. m[0] means the 0'th item in the
// map, not the item with key '0'), for use by 'for' loops
// if there is an arg that is the boolean 'false', it indicates that this is a
// method call, NOT a normal look-up, and the 'self' arg will need to be loaded
// as well as the result of the table look-up
void Executor::Tbl_load( Instruction const & inst )
{
	// top of stack has index/key
	DevaObject idxkey = stack.back();
	stack.pop_back();
	// next-to-top of stack has name of vector/map
	DevaObject vecmap = stack.back();
	stack.pop_back();

	if( vecmap.Type() != sym_unknown && vecmap.Type() != sym_map && vecmap.Type() != sym_vector )
		throw DevaRuntimeException( "Invalid object for 'tbl_load' instruction." );

	// if the index/key is a variable, look it up
	if( idxkey.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( idxkey );
		if( !o )
			throw DevaRuntimeException( "Invalid type for index/key." );
		idxkey = *o;
	}
	// find the map/vector from the current symbol table (namespace)
	DevaObject *table;
	if( vecmap.Type() == sym_unknown )
	{
		table = find_symbol( vecmap );
		// if it wasn't found in the current scope table, see if it is a
		// namespace
		if( !table )
		{
			ScopeTable* ns = NULL;
			map<string, ScopeTable>::iterator it;
			it = namespaces.find( vecmap.name );
			// if we found the namespace
			if( it != namespaces.end() )
			{
				ns = &(it->second);
				// look up the key in it (key should be a string)
				if( idxkey.Type() != sym_string )
					throw DevaICE( "Trying to look-up a non-string key in a namespace." );
				DevaObject key( idxkey.str_val, sym_unknown );
				DevaObject* obj = find_symbol( key, ns );
				if( !obj )
					throw DevaRuntimeException( "Attempt to reference undefined object in namespace." );
				// push it onto the stack as our return value
				DevaObject o( *obj );
				stack.push_back( o );
				// done
				return;
			}
		}
		if( !table )
			throw DevaRuntimeException( "Attempt to reference undefined map, vector or namespace." );
	}
	else
		table = &vecmap;

	// ensure it is the correct type
	// UDTs
	// class (static) methods...
	if( table->Type() == sym_class )
	{
		if( idxkey.Type() != sym_string )
			throw DevaICE( "Invalid class method." );
		// method names have "@<classname>" appended to their symbol names
		string method( idxkey.str_val );
		method += "@";
		method += table->name;
		// get the value from the map
		smart_ptr<DOMap> mp( table->map_val );
		DOMap::iterator it;
		it = mp->find( DevaObject( "", method ) );
		bool is_method = true;
		// if not found, try looking for it as a field (no "@<classname>")
		if( it == mp->end() )
		{
			is_method = false;
			it = mp->find( idxkey );
			if( it == mp->end() )
				throw DevaRuntimeException( "Invalid method or field. No such item found." );
		}
		// for method calls, push 'self' (the class itself for class methods)
		// was the method-call flag passed with this instruction?
		if( is_method && inst.args.size() > 0 && inst.args[0].Type() == sym_boolean && inst.args[0].bool_val == false )
			stack.push_back( *table );
		// push it onto the stack
		pair<DevaObject, DevaObject> p = *it;
		stack.push_back( p.second );
	}
	// instance (object) methods...  
	else if( table->Type() == sym_instance )
	{
		if( idxkey.Type() != sym_string )
			throw DevaICE( "Invalid class method." );

		DOMap::iterator it;
		smart_ptr<DOMap> mp( table->map_val );
		// look up the variable '__class__' in the instance, it holds the class
		// type name
		it = mp->find( DevaObject( "", string( "__class__" ) ) );
		if( it == mp->end() )
			throw DevaRuntimeException( "Instance does not contain a '__class__' variable." );
		string classname( it->second.str_val );
		// method names have "@<classname>" appended to their symbol names
		string method( idxkey.str_val );
		method += "@";
		method += classname;
		// get the fcn value from the map
		it = mp->find( DevaObject( "", method ) );
		bool is_method = true;
		// if not found, try looking for it as a field (no "@<classname>")
		if( it == mp->end() )
		{
			is_method = false;
			it = mp->find( idxkey );
			if( it == mp->end() )
				throw DevaRuntimeException( "Invalid method or field. No such item found." );
		}
		// for method calls, push 'self'
		// was the method-call flag passed with this instruction?
		if( is_method && inst.args.size() > 0 && inst.args[0].Type() == sym_boolean && inst.args[0].bool_val == false )
			stack.push_back( *table );
		// push the method/field
		pair<DevaObject, DevaObject> p = *it;
		stack.push_back( p.second );
	}

	// vector *must* have a numeric (integer) index for actual vector look-ups,
	// or string index for method calls
	else if( table->Type() == sym_vector )
	{
		// if this is a string index and it was a vector or UDT, 
		// this is a method call on a vector builtin (or an error)
		if( idxkey.Type() == sym_string )
		{
			// built-in method call
			// top of stack contains key (fcn name as string)
			// next-to-top contains value (fcn as sym_offset)
			string key( "vector_");
		   	key += idxkey.str_val; 
			// key == string
			// table == vector
			// look up 'key' in the vector built-ins
			if( !is_vector_builtin( key ) )
				throw DevaRuntimeException( "Not a valid method on type 'vector'." );
			// push the vector as an argument
			stack.push_back( *table );
			// push a fcn with 'key' as its name (it's a builtin, so the offset
			// given is irrelevant)
			stack.push_back( DevaObject( key.c_str(), (size_t)-1 ) );
			return;
		}
		if( idxkey.Type() != sym_number )
			throw DevaRuntimeException( "Argument to '[]' operator on a vector MUST evaluate to an integral number." );
		// TODO: error on non-integral index 
		int idx = (int)idxkey.num_val;
		smart_ptr<DOVector> v( table->vec_val );
		// get the value from the vector
		if( idx < 0 || idx >= v->size() )
			throw DevaRuntimeException( "Index to vector out-of-range." );
		DevaObject o = v->at( idx );
		// push it onto the stack
		stack.push_back( o );
	}
	// maps can be indexed by number/string/user-defined-type
	else if( table->Type() == sym_map )
	{
		// if there is an arg, and it's boolean 'true', treat the key as an
		// index (for use by 'for' loops)
		if( inst.args.size() > 0 && inst.args[0].Type() == sym_boolean && inst.args[0].bool_val == true )
		{
			// key must be an integral number, as it is really an index
			if( idxkey.Type() != sym_number )
				throw DevaICE( "Index in tbl_load instruction on a map MUST evaluate to a number." );
			// TODO: error on non-integral value
			int idx = (int)idxkey.num_val;
			// get the value from the map
			smart_ptr<DOMap> mp( table->map_val );
			if( idx >= mp->size() )
				throw DevaICE( "Index out-of-range in tbl_load instruction." );
			DOMap::iterator it;
			// this loop is equivalent of "it = mp->begin() + idx;" 
			// (which is required because map iterators don't support the + op)
			it = mp->begin();
			for( int i = 0; i < idx; ++i ) ++it;

			if( it == mp->end() )
				throw DevaICE( "Index out-of-range in tbl_load instruction, after being checked. Memory corruption?" );
			// push it onto the stack
			pair<DevaObject, DevaObject> p = *it;
			stack.push_back( p.second );
			stack.push_back( p.first );
		}
		else
		{
			// key (number/string/user-defined-type)?
			// TODO: user-defined-type as key
			if( idxkey.Type() != sym_number &&  idxkey.Type() != sym_string )
				throw DevaRuntimeException( "Argument to '[]' on a map MUST evaluate to a number, string or user-defined-type." );
			// get the value from the map
			smart_ptr<DOMap> mp( table->map_val );
			DOMap::iterator it;
			it = mp->find( idxkey );
			// if not found, try the map built-in methods...
			if( it == mp->end() )
			{
				// built-in method call??
				// top of stack contains key (fcn name as string)
				// next-to-top contains value (fcn as sym_offset)
				string key( "map_");
				key += idxkey.str_val; 
				// key == string
				// table == map
				// look up 'key' in the map built-ins
				if( !is_map_builtin( key ) )
					throw DevaRuntimeException( "Invalid map key or method. No such item found." );
				// push the map as an argument
				stack.push_back( *table );
				// push a fcn with 'key' as its name (it's a builtin, so the offset
				// given is irrelevant)
				stack.push_back( DevaObject( key.c_str(), (size_t)-1 ) );
				return;
			}
			// push it onto the stack
			pair<DevaObject, DevaObject> p = *it;
			stack.push_back( p.second );
		}
	}
	else if( table->Type() == sym_string )
	{
		// if this is a string index then this is a method call on a string builtin (or an error)
		if( idxkey.Type() == sym_string )
		{
			// built-in method call
			// top of stack contains key (fcn name as string)
			// next-to-top contains value (fcn as sym_offset)
			string key( "string_");
		   	key += idxkey.str_val; 
			// key == string
			// table == string
			// look up 'key' in the string built-ins
			if( !is_string_builtin( key ) )
				throw DevaRuntimeException( "Not a valid method on type 'string'." );
			// push the string as an argument
			stack.push_back( *table );
			// push a fcn with 'key' as its name (it's a builtin, so the offset
			// given is irrelevant)
			stack.push_back( DevaObject( key.c_str(), (size_t)-1 ) );
			return;
		}
		// index to a string must be an integral number
		if( idxkey.Type() != sym_number )
			throw DevaRuntimeException( "Argument to '[]' operator on a string MUST evaluate to an integral number." );
		// TODO: error on non-integral index 
		int idx = (int)idxkey.num_val;

		// get the value from the string
		string s( table->str_val );
		if( idx < 0 || idx >= s.length() )
			throw DevaRuntimeException( "Index to string out-of-range." );
		DevaObject o( "", string( 1, s.at( idx ) ) );
		// push it onto the stack
		stack.push_back( o );
	}
	else
		throw DevaRuntimeException( "Object to which '[]' operator is applied must be map or a vector." );
}
// 10 set item in vector or map. args: index, value
void Executor::Tbl_store( Instruction const & inst )
{
    // enough data on stack
    if( stack.size() < 3 )
        throw DevaICE( "Invalid 'tbl_store' instruction: not enough data on stack." );
	// top of stack has value
	DevaObject val = stack.back();
	stack.pop_back();
	// next-to-top of stack has the index/key
	DevaObject idxkey = stack.back();
	stack.pop_back();
	// next-to-next-to-top of stack has name of vector/map
	DevaObject vecmap = stack.back();
	stack.pop_back();

	if( vecmap.Type() != sym_unknown && vecmap.Type() != sym_map && vecmap.Type() != sym_vector 
		&& vecmap.Type() != sym_class && vecmap.Type() != sym_instance )
		throw DevaRuntimeException( "Invalid object for 'tbl_store' instruction." );

	// if the value is a variable, look it up
	if( val.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Invalid type for operand to a vector store operation." );
		val = *o;
	}
	// if the index/key is a variable, look it up
	if( idxkey.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( idxkey );
		if( !o )
			throw DevaRuntimeException( "Invalid type for index/key." );
		idxkey = *o;
	}
	// find the map/vector from the symbol table
    DevaObject *table;
    if( vecmap.Type() == sym_unknown )
        table = find_symbol( vecmap );
    else
        table = &vecmap;

	// ensure it is the correct type
	// vector *must* have a numeric (integer) index
	if( table->Type() == sym_vector )
	{
		if( idxkey.Type() != sym_number )
			throw DevaRuntimeException( "Argument to '[]' operator on a vector MUST evaluate to an integral number." );
		// TODO: error on non-integral index 
		int idx = (int)idxkey.num_val;
		smart_ptr<DOVector> v( table->vec_val );
		// set the value in the vector
		if( idx < 0 || idx >= v->size() )
			throw DevaRuntimeException( "Index to vector out-of-range." );
		(*v)[idx] = val;
	}
	// maps can be indexed by number/string/user-defined-type
	else if( table->Type() == sym_map )
	{
		// key (number/string/user-defined-type)?
		// TODO: user-defined-type as key
		if( idxkey.Type() != sym_number &&  idxkey.Type() != sym_string )
			throw DevaRuntimeException( "Argument to '[]' on a map MUST evaluate to a number, string or user-defined-type." );
		// set the value in the map (whether it already exists or not)
		smart_ptr<DOMap> mp( table->map_val) ;
		mp->operator[]( idxkey ) = val;
	}
	// classes/instances members are methods or variables and must be indexed by
	// name
	else if( table->Type() == sym_class || table->Type() == sym_instance )
	{
		// key (string)
		if( idxkey.Type() != sym_string )
			throw DevaRuntimeException( "Member added to a class or instance must be accessed by name." );
		// set the value in the map (whether it already exists or not)
		smart_ptr<DOMap> mp( table->map_val) ;
		mp->operator[]( idxkey ) = val;
	}
	else
		throw DevaRuntimeException( "Object to which '[]' operator is applied must be map or a vector." );

}
// 11 swap top two items on stack
void Executor::Swap( Instruction const & inst )
{
	if( stack.size() < 2 )
		throw DevaICE( "Invalid state: 'swap' instruction called with less than two items on stack." );
	DevaObject one = stack.back();
	stack.pop_back();
	DevaObject two = stack.back();
	stack.pop_back();

	stack.push_back( one );
	stack.push_back( two );
}
// 12 line number
void Executor::Line_num( Instruction const & inst )
{
    file = inst.args[0].str_val;
    line = inst.args[1].func_offset;
}
// 13 unconditional jump to the address on top of the stack
void Executor::Jmp( Instruction const & inst )
{
	// one arg: the offset to jump to (as a 'function'/offset/address type)
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid jmp instruction: no arguments." );
	DevaObject dest = inst.args[0];
	if( dest.Type() != sym_offset )
		throw DevaICE( "Invalid jmp instruction argument: not a jump target offset." );
	// jump execution to the function offset
	ip = dest.func_offset;
}
// 14 jump on top of stack evaluating to false 
void Executor::Jmpf( Instruction const & inst )
{
	// one arg: the offset to jump to (as a 'function'/offset/address type)
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid jmp instruction: no arguments." );
	DevaObject dest = inst.args[0];
	if( dest.Type() != sym_offset )
		throw DevaICE( "Invalid jmp instruction argument: not a jump target offset." );

	// get the value on the top of the stack
	DevaObject o = stack.back();
	stack.pop_back();
	// if it is a variable, lookup the variable in the symbol table
	if( o.Type() == sym_unknown )
	{
		DevaObject* var = find_symbol( o );
		if( !var )
			throw DevaRuntimeException( "Undefined variable referenced in jmpf instruction." );
		if( var->Type() != sym_unknown && var->Type() != sym_number && var->Type() != sym_string
			&& var->Type() != sym_boolean && var->Type() != sym_null )
			throw DevaRuntimeException( "Type of condition for jmpf cannot be evaluated to a true/false value." );
		o = *var;
	}
	// if it evaluates to 'true', return
	if( evaluate_object_as_boolean( o ) )
		return;
	// else jump to the offset in the argument
	ip = dest.func_offset;
}
// 15 == compare top two values on stack
void Executor::Eq( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid eq instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for eq instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in equality comparision." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in equality comparision." );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Comparison of incompatible types." );
	int result = compare_objects( lhs, rhs );
	if( result == 0 )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 16 != compare top two values on stack
void Executor::Neq( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid neq instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for neq instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in neq comparision." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in neq comparision." );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Comparison of incompatible types." );
	int result = compare_objects( lhs, rhs );
	if( result != 0 )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 17 < compare top two values on stack
void Executor::Lt( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid lt instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for lt instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in lt comparision." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in lt comparision." );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Comparison of incompatible types." );
	int result = compare_objects( lhs, rhs );
	if( result < 0 )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 18 <= compare top two values on stack
void Executor::Lte( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid lte instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for lte instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in lte comparision." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in lte comparision." );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Comparison of incompatible types." );
	int result = compare_objects( lhs, rhs );
	if( result <= 0 )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 19 > compare top two values on stack
void Executor::Gt( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid gt instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for gt instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in gt comparision." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in gt comparision." );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Comparison of incompatible types." );
	int result = compare_objects( lhs, rhs );
	if( result > 0 )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 20 >= compare top two values on stack
void Executor::Gte( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid gte instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for gte instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in gte comparision." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in gte comparision." );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "Comparison of incompatible types." );
	int result = compare_objects( lhs, rhs );
	if( result >= 0 )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 21 || the top two values
void Executor::Or( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid 'or' instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for 'or' instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in 'or' instruction." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in 'or' instruction." );
		rhs = *o;
	}
	// if either value is true, push 'true' onto the stack
	// else push 'false'
	if( evaluate_object_as_boolean( lhs ) )
		stack.push_back( DevaObject( "", true ) );
	else if( evaluate_object_as_boolean( rhs ) )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 22 && the top two values
void Executor::And( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid 'and' instruction." );
	// get the lhs and rhs values
	if( stack.size() < 2 )
		throw DevaICE( "Not enough data on stack for 'and' instruction." );
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if they are variables, get their values from the symbol table
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in 'and' instruction." );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as right-hand operand in 'and' instruction." );
		rhs = *o;
	}
	// if both values are true, push 'true' onto the stack
	// else push 'false'
	if( evaluate_object_as_boolean( lhs ) && evaluate_object_as_boolean( rhs ) )
		stack.push_back( DevaObject( "", true ) );
	else
		stack.push_back( DevaObject( "", false ) );
}
// 23 negate the top value ('-' operator)
void Executor::Neg( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid eq instruction." );
	// get the operand values
	if( stack.size() < 1 )
		throw DevaICE( "Not enough data on stack for not instruction." );
	DevaObject op = stack.back();
	stack.pop_back();
	// if it's a variable, get its value from the symbol table
	if( op.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( op );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in equality comparision." );
		op = *o;
	}
	// if it's not a number now, error
	if( op.Type() != sym_number )
		throw DevaRuntimeException( "Negate (-) operator used on non-numeric type." );
	// push the negative of the operand
	op.num_val = -(op.num_val);
	stack.push_back( op );
}
// 24 boolean not the top value ('!' operator)
void Executor::Not( Instruction const & inst )
{
	if( inst.args.size() != 0 )
		throw DevaICE( "Invalid eq instruction." );
	// get the operand values
	if( stack.size() < 1 )
		throw DevaICE( "Not enough data on stack for not instruction." );
	DevaObject op = stack.back();
	stack.pop_back();
	// if it's a variable, get its value from the symbol table
	if( op.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( op );
		if( !o )
			throw DevaRuntimeException( "Undefined variable used as left-hand operand in equality comparision." );
		op = *o;
	}
	// push the inverse of the operand
	stack.push_back( DevaObject( "", !evaluate_object_as_boolean( op ) ) );
}
// 25 add top two values on stack
void Executor::Add( Instruction const & inst )
{
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
	if( stack.size() < 2 )
		throw DevaRuntimeException( "Not enough items on stack for subtract operation." );
	// subtract using the top two items on the stack
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the args are variables, look them up in the symbol table
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Right-hand operand for subtract operation not found in symbol table." );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Left-hand operand for subtract operation not found in symbol table." );
		lhs = *o;
	}
	// do the subtraction
	if( lhs.Type() == sym_number )
	{
		if( rhs.Type() != sym_number )
			throw DevaRuntimeException( "Invalid right-hand argument type for subtract operation." );
		double ret = lhs.num_val - rhs.num_val;
		stack.push_back( DevaObject( "", ret ) );
	}
	else
		throw DevaRuntimeException( "Invalid argument types for subtract operation." );
}
// 27 multiply top two values on stack
void Executor::Mul( Instruction const & inst )
{
	if( stack.size() < 2 )
		throw DevaRuntimeException( "Not enough items on stack for multiply operation." );
	// multiply using the top two items on the stack
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the args are variables, look them up in the symbol table
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Right-hand operand for multiply operation not found in symbol table." );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Left-hand operand for multiply operation not found in symbol table." );
		lhs = *o;
	}
	// do the multiplication
	if( lhs.Type() == sym_number )
	{
		if( rhs.Type() != sym_number )
			throw DevaRuntimeException( "Invalid right-hand argument type for multiply operation." );
		double ret = lhs.num_val * rhs.num_val;
		stack.push_back( DevaObject( "", ret ) );
	}
	else
		throw DevaRuntimeException( "Invalid argument types for multiply operation." );
}
// 28 divide top two values on stack
void Executor::Div( Instruction const & inst )
{
	if( stack.size() < 2 )
		throw DevaRuntimeException( "Not enough items on stack for divide operation." );
	// divide using the top two items on the stack
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the args are variables, look them up in the symbol table
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Right-hand operand for divide operation not found in symbol table." );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Left-hand operand for divide operation not found in symbol table." );
		lhs = *o;
	}
	// do the division
	if( lhs.Type() == sym_number )
	{
		if( rhs.Type() != sym_number )
			throw DevaRuntimeException( "Invalid right-hand argument type for divide operation." );
		double ret = lhs.num_val / rhs.num_val;
		stack.push_back( DevaObject( "", ret ) );
	}
	else
		throw DevaRuntimeException( "Invalid argument types for divide operation." );
}
// 29 modulus top two values on stack
void Executor::Mod( Instruction const & inst )
{
	if( stack.size() < 2 )
		throw DevaRuntimeException( "Not enough items on stack for modulus operation." );
	// modulus using the top two items on the stack
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the args are variables, look them up in the symbol table
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( "Right-hand operand for modulus operation not found in symbol table." );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( "Left-hand operand for modulus operation not found in symbol table." );
		lhs = *o;
	}
	// do the modulus
	if( lhs.Type() == sym_number )
	{
		if( rhs.Type() != sym_number )
			throw DevaRuntimeException( "Invalid right-hand argument type for modulus operation." );
		// TODO: modulus requires integer arguments, error on non-integers
		double ret = (int)lhs.num_val % (int)rhs.num_val;
		stack.push_back( DevaObject( "", ret ) );
	}
	else
		throw DevaRuntimeException( "Invalid argument types for modulus operation." );
}
// 30 dump top of stack to stdout
void Executor::Output( Instruction const & inst )
{
	// get the argument off the stack
	DevaObject obj = stack.back();
	stack.pop_back();
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
			{
			// dump map contents
			cout << "map: '" << o->name << "' = " << endl;
			smart_ptr<DOMap> mp( o->map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); ++it )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				cout << " - " << key << " : " << val << endl;
			}
			break;
			}
		case sym_vector:
			{
			// dump vector contents
			cout << "vector: '" << o->name << "' = " << endl;
			smart_ptr<DOVector> vec( o->vec_val );
			for( DOVector::iterator it = vec->begin(); it != vec->end(); ++it )
			{
				DevaObject val = (*it);
				cout << val << endl;
			}
			break;
			}
		case sym_offset:
			cout << "function: '" << o->name << "'";
			break;
		case sym_function_call:
			cout << "function_call: '" << o->name << "'";
			break;
		default:
			cout << "unknown: '" << o->name << "'";
	}
	// print it
//	cout << endl;
}
// 31 call a function. arguments on stack
void Executor::Call( Instruction const & inst )
{
	// check for built-in function first
	if( inst.args.size() == 2 && is_builtin( inst.args[0].name ) )
	{
		// set the static that tracks the number of args processed
		args_on_stack = inst.args[1].func_offset;
		execute_builtin( this, inst );
		// reset the static that tracks the number of args (builtins don't run
		// the 'return' instruction)
		args_on_stack = -1;
	}
	else
	{
		DevaObject* fcn;
		// if there's more than one arg, the first is the name of the fcn to call 
		// and the second is the number of args passed to it (on the stack)
		if( inst.args.size() == 2 )
		{
			// look up the name in the symbol table
			fcn = find_symbol( inst.args[0] );
			if( !fcn )
				throw DevaRuntimeException( "Call made to undefined function." );

			if( fcn->Type() != sym_offset )
				throw DevaRuntimeException( "Trying to call an object that is not a function or method." );

			// check the number of args to the fcn
			if( inst.args[1].Type() != sym_offset )
				throw DevaICE( "Function call doesn't indicate number of arguments passed." );

			// set the static that tracks the number of args processed
			args_on_stack = inst.args[1].func_offset;
		}
		// if there's one arg (the num of args to the fcn), 
		// then it's a method invokation,
		// pop the top of the stack for the fcn to call
		else if( inst.args.size() == 1 )
		{
			// check the number of args to the fcn
			if( inst.args[0].Type() != sym_offset )
				throw DevaICE( "Function call doesn't indicate number of arguments passed." );

			// get the function to call off the stack
			DevaObject o = stack.back();
			stack.pop_back();
			if( o.Type() == sym_offset )
				fcn = &o;
			else if( o.Type() == sym_unknown )
				fcn = find_symbol( o.name );
			else
				throw DevaICE( "Invalid argument (on stack) for 'call' instruction." );

			// if this is a vector builtin method, execute it
			if( is_vector_builtin( fcn->name ) )
			{
				// set the static that tracks the number of args processed
				args_on_stack = inst.args[0].func_offset;

				execute_vector_builtin( this, fcn->name );

				// reset the static that tracks the number of args (builtins don't run
				// the 'return' instruction)
				args_on_stack = -1;
				return;
			}
			// if this is a map builtin method, execute it
			if( is_map_builtin( fcn->name ) )
			{
				// set the static that tracks the number of args processed
				args_on_stack = inst.args[0].func_offset;

				execute_map_builtin( this, fcn->name );

				// reset the static that tracks the number of args (builtins don't run
				// the 'return' instruction)
				args_on_stack = -1;
				return;
			}
			// if this is a vector builtin method, execute it
			if( is_string_builtin( fcn->name ) )
			{
				// set the static that tracks the number of args processed
				args_on_stack = inst.args[0].func_offset;

				execute_string_builtin( this, fcn->name );

				// reset the static that tracks the number of args (builtins don't run
				// the 'return' instruction)
				args_on_stack = -1;
				return;
			}
			// set the static that tracks the number of args processed
			// methods on UDTs (classes and instances)
			// need to adjust arg count for the implicit 'self' (+1)
			if( fcn->name.find( '@' ) != string::npos )
				args_on_stack = inst.args[0].func_offset + 1;
			else
				args_on_stack = inst.args[0].func_offset;
		}
		else
			throw DevaICE( "Invalid number of arguments to 'call' instruction." );

		// get the offset for the function
		size_t offset = fcn->func_offset;
		// jump execution to the function offset
		ip = offset;
	}
}
// 32 stack holds return value and then (at top) return address
void Executor::Return( Instruction const & inst )
{
	// if the args_on_stack variable wasn't decremented to 0, then too many args
	// were passed, explode
	// HOWEVER, this is really TOO LATE, and another error has probably already
	// occurred
	if( args_on_stack > 0 )
		throw DevaRuntimeException( "Too many arguments passed to function." );

	// the return value is now on top of the stack, instead of the return
	// address, so we need to pop it, pop the return address and then re-push
	// the return value
	if( stack.size() < 2 )
		throw DevaRuntimeException( "Invalid 'return' instruction: not enough data on the stack." );
	DevaObject ret = stack.back();
	stack.pop_back();
	// evaluate the return value *before* leaving this scope, and add it to the
	// parent scope (to which we'll be returning)
	if( ret.Type() == sym_unknown )
	{
		DevaObject* rv = find_symbol( ret );
		if( !rv )
			throw DevaRuntimeException( "Invalid object for return value." );
		if( current_scopes->size() < 2 )
			throw DevaICE( "No scope to return to!" );
		// add this to the *parent* scope with a new ref on it
		DevaObject *r = new DevaObject( *rv );
		(*current_scopes)[current_scopes->size()-2]->AddObject( r );
	}

	// pop the return location off the top of the stack
	DevaObject ob = stack.back();
	stack.pop_back();
	if( ob.Type() != sym_offset )
		throw DevaRuntimeException( "Invalid return destination on stack when executing return instruction" );
	stack.push_back( ret );
	// jump to the return location
	size_t offset = ob.func_offset;
	ip = offset;
	// pop this scope (same as 'leave' instruction)
	current_scopes->Pop();
	// reset the static that tracks the number of args processed
	args_on_stack = -1;
}
// 33 break out of loop, respecting scope (enter/leave)
void Executor::Break( Instruction const & inst )
{
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid 'break' instruction. No arguments." );
	if( inst.args[0].Type() != sym_offset )
		throw DevaICE( "Invalid 'break' instruction. Argument is non-numeric." );

	vector<int> enter_stack;
	size_t original_ip = ip;

	// set the ip back to the start of the loop, then scan forward looking 
	// and count the enter/leave instructions so we know how many leaves we need to
	// execute to get out of the loop
	ip = inst.args[0].func_offset;
	while( ip < original_ip )
	{
		switch( PeekInstr() )
		{
		case op_enter:
			enter_stack.push_back( 1 );
			NextInstr();
			break;

		case op_leave:
			enter_stack.pop_back();
			NextInstr();
			break;

		// skip everything else
		default:
			NextInstr();
			break;
		}
	}
	// now, back at the original position,
	// scan forward, looking for, and executing, enter/leave instructions
	// until we leave the current scope (1 move leave op than enter ops)
	while( true )
	{
		switch( PeekInstr() )
		{
		case op_enter:
			{
			enter_stack.push_back( 1 );
			Instruction inst = NextInstr();
			DoInstr( inst );
			break;
			}
		case op_leave:
			{
			enter_stack.pop_back();
			Instruction inst = NextInstr();
			DoInstr( inst );
			// if we've left enough scopes to get out of the loop
			if( enter_stack.size() == 0 )
			{
				// need to skip the jmp back to loop start
				if( PeekInstr() != op_jmp )
					throw DevaICE( "Invalid loop. Final 'leave' instruction not followed by 'jmp'." );
				NextInstr();
				return;
			}
			break;
			}
		// skip everything else
		default:
			NextInstr();
			break;
		}
	}
}
// 34 enter new scope
void Executor::Enter( Instruction const & inst )
{
	current_scopes->Push();
}
// 35 leave scope
void Executor::Leave( Instruction const & inst )
{
	if( current_scopes->size() == 0 )
		throw DevaRuntimeException( "Invalid Leave operation. No scopes to exit." );
	current_scopes->Pop();
}
// 36 no op
void Executor::Nop( Instruction const & inst )
{
	// do nothing
}
// 37 finish program, 0 or 1 ops (return code)
void Executor::Halt( Instruction const & inst )
{
	// do nothing, execution engine will stop on stepping to this instruction
}
// helper function for Import
string Executor::find_module( string mod )
{
	// split the path given into it's / separated parts
	vector<string> path = split_path( mod );

	// first, look in the directory of the currently executing file
	string current_file = get_file_part( file );
	// get the cwd
	string curdir = get_cwd();
	// append the current file to the current directory (in case the current
	// file path contains directories, e.g. the compiler was called on the
	// file 'src/foo.dv')
	curdir += "/";
	curdir += current_file;
	curdir = get_dir_part( curdir );
	// 'curdir' now contains the directory of the currently executing file
	// append the mod name to get the mod path
	string modpath( curdir );
	for( vector<string>::iterator it = path.begin(); it != path.end(); ++it )
	{
		modpath += '/';
		modpath += *it;
	}
	// check for .dv/.dvc files on disk
	struct stat statbuf;
	// if we can't open the module file, error out
	string dv = modpath + ".dv";
	if( stat( dv.c_str(), &statbuf ) == -1 )
	{
		string dvc = modpath + ".dvc";
		if( stat( dvc.c_str(), &statbuf ) == -1 )
			throw DevaRuntimeException( "Unable to locate module for import." );
	}
	// otherwise it's valid, return it
	return modpath;
	// TODO:
	// - look in the paths on the 'DEVA_PATH' env var
}
// 38 import module, 1 arg: module name
void Executor::Import( Instruction const & inst )
{
	// first argument has the name of the module to import
	if( inst.args.size() < 1 )
		throw DevaICE( "No module name given in import statement." );
	string mod = inst.args[0].str_val;
	string path = find_module( mod );
	
	// prevent importing the same module more than once
	map<string, ScopeTable>::iterator it;
	it = namespaces.find( mod );
	// if we found the namespace
	if( it != namespaces.end() )
		return;

	// for now, just run the file by short name with ".dvc" extension (i.e. in
	// the current working directory)
	string dvfile( path + ".dv" );
	string dvcfile( path + ".dvc" );
	// save the ip
	size_t orig_ip = ip;
	// save the current scope table
	ScopeTable* orig_scopes = current_scopes;
	// create a new namespace and set it at the current scope
	// TODO: currently this only adds the "short" name of the module as a
	// namespace. should the full path be used somehow?? foo::bar? foo.bar?
	// foo-bar? foo/bar?
	mod = get_file_part( mod );
	namespaces[mod] = ScopeTable();
	current_scopes = &namespaces[mod];
	// create a 'file/module' level scope for the namespace
	current_scopes->Push();
	// compile the file, if needed
	CompileFile( dvfile.c_str() );
	// and then run the file
	RunFile( dvcfile.c_str() );
	// restore the ip
	ip = orig_ip;
	// restore the current scope table
	current_scopes = orig_scopes;
}
// 39 create a new class object and push onto stack
void Executor::New_class( Instruction const & inst )
{
	// top of the stack has the name of the class
	DevaObject cls_name = stack.back();
	if( cls_name.Type() != sym_unknown )
		throw DevaICE( "Invalid class name for new class definition." );

	// create a new class object
	DevaObject cls( "", sym_class );
	// merge its parents
	int num_args = inst.args.size();
	for( int c = 0; c < num_args; ++c )
	{
		DevaObject base = inst.args[c];
		// look-up the base class
		DevaObject* ob;
		if( base.Type() != sym_unknown )
			throw DevaICE( "Invalid base class for class definition." );
		ob = find_symbol( base );
		if( !ob )
			throw DevaRuntimeException( "Invalid base class name for class definition." );
		// ensure it's a class
		if( ob->Type() != sym_class )
			throw DevaRuntimeException( "Invalid base class type for class definition." );

		// merge the base class into the new class
		for( DOMap::iterator i = ob->map_val->begin(); i != ob->map_val->end(); ++i )
		{
			if( i->first.Type() != sym_string )
				throw DevaRuntimeException( "Invalid method in base class given for class definition." );
			// the methods have the name from their base-class (foo@base)
			// fix it to be from this class (foo@class)
			// (note the function object still has the base-class name, so
			// reflection/instrospection can still see where methods came from)
			string name( i->first.str_val );
			size_t pos = name.find( '@' );
			if( pos == string::npos )
				throw DevaRuntimeException( "Invalid method name in base class given for class definition." );
			name.replace( pos+1, name.length() - pos, cls_name.name );
			cls.map_val->insert( make_pair( DevaObject( "", name ), i->second ) );
		}
	}
	// push it onto the stack (subsequent instructions will add its methods,
	// overriding what it inherited)
	stack.push_back( cls );
}
// 40 create a new class instance object and push onto stack
void Executor::New_instance( Instruction const & inst )
{
	// 1 arg: an offset containing the number of arguments to 'new'
	if( inst.args.size() != 1 )
		throw DevaICE( "Invalid number of arguments in 'new_instance' instruction." );
	if( inst.args[0].Type() != sym_offset )
		throw DevaICE( "Invalid argument in 'new_instance' instruction." );
	// class to create instance of is on top of stack
	DevaObject cls = stack.back();
	stack.pop_back();

	DevaObject* ob;
	if( cls.Type() == sym_unknown )
	{
		ob = find_symbol( cls );
		if( !ob )
			throw DevaRuntimeException( "Invalid class name for new object." );
	}
	else
		*ob = cls;

	// ensure it's a class
	if( ob->Type() != sym_class )
		throw DevaRuntimeException( "Invalid class type for new object." );

	// create a new instance object as a copy of the class object
	DOMap* m = new DOMap( *(ob->map_val) );
	DevaObject instance = DevaObject::InstanceFromMap( "", m );
	// - add the __class__ member to the instance
	instance.map_val->insert( make_pair( DevaObject( "", string( "__class__" ) ), DevaObject( "", ob->name ) ) );
	// - call the constructor ('new' method)
	size_t num_args = inst.args[0].func_offset;
	// TODO: look for 'new' function even if the only arg is 'self', in case the user
	// has overridden it
	if( num_args > 1 )
	{
		// push 'self'
		stack.push_back( instance );
		// call the 'new' method for this class
		string method( "new" );
		method += "@";
		method += ob->name;
		// TODO: this doesn't work for setting fields etc - they don't persist
		ExecuteDevaFunction( method, num_args );
		// ignore the return value from 'new'. not allowed
		stack.pop_back();
	}

	// push it onto the stack
	stack.push_back( instance );
}
// illegal operation, if exists there was a compiler error/fault
void Executor::Illegal( Instruction const & inst )
{
	throw DevaRuntimeException( "Illegal Instruction" );
}
///////////////////////////////////////////////////////////


// execute single instruction
bool Executor::DoInstr( Instruction & inst )
{
	if( debug_mode )
	{
		cout << inst << endl;
	}

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
	case op_dup:			// 6 create a new object and place on top of stack
		Dup( inst );
		break;
	case op_new_map:		// 7 create a new map object and push onto stack
		New_map( inst );
		break;
	case op_new_vec:		// 8 create a new vector object and push onto stack
		New_vec( inst );
		break;
	case op_tbl_load:	// 9 get item from vector/map
		Tbl_load( inst );
		break;
	case op_tbl_store:	// 10 set item in vector/map. args: index, value
		Tbl_store( inst );
		break;
	case op_swap:		// 11 swap top two items on stack. no args
		Swap( inst );
		break;
	case op_line_num:	// 12 set item in map. args: index, value
		Line_num( inst );
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
	case op_break:		// 33 break out of loop, respecting scope (enter/leave)
		Break( inst );
		break;
	case op_enter:		// 34 enter new scope
		Enter( inst );
		break;
	case op_leave:		// 35 leave scope
		Leave( inst );
		break;
	case op_nop:			// 36 no op
		Nop( inst );
		break;
	case op_halt:		// 37 finish program, 0 or 1 ops (return code)
		Halt( inst );
		return false;
	case op_import:
		Import( inst );
		break;
	case op_new_class:	// 39 new class
		New_class( inst );
		break;
	case op_new_instance:	// 40 new instance. 1 arg: number of args to pass to class's 'new'
		New_instance( inst );
		break;
	case op_illegal:	// illegal operation, if exists there was a compiler error/fault
	default:
		Illegal( inst );
		break;
	}

	if( debug_mode )
	{
		cout << " - stack: ";
		for( vector<DevaObject>::reverse_iterator ri = stack.rbegin(); ri < stack.rend(); ++ri )
		{
			DevaObject o = *ri;
			cout << o << " | ";
		}
		cout << endl;
	}

	return true;
}

// read the next instruction (increment ip to the next instruction)
Instruction Executor::NextInstr()
{
	char* name = NULL;
	double num_val;
	char* str_val = NULL;
	long bool_val;
	size_t func_offset;
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
				// TODO: is this an error??
				break;
			case sym_vector:
				// TODO: is this an error??
				break;
			case sym_offset:
				{
				// size_t
				read_size_t( func_offset );
				DevaObject ob( name, (size_t)func_offset );
				inst.args.push_back( ob );
				break;
				}
			case sym_function_call:
				{
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
		// default to sym_end to drop out of loop if we can't read a byte
		type = (unsigned char)sym_end;
		read_byte( type );
	}
	return inst;
}

// public methods
///////////////////////////////////////////////////////////
Executor::Executor( bool dbg ) : debug_mode( dbg ), code( NULL ), ip( 0 ), file( "" ), line( 0 ), current_scopes( NULL )
{}

Executor::~Executor()
{
	for( vector<unsigned char*>::iterator i = code_blocks.begin(); i != code_blocks.end(); ++i )
	{
		delete [] *i;
		*i = NULL;
	}
}

void Executor::ExecuteDevaFunction( string fcn_name, int num_args )
{
	DevaObject* fcn;
	// look up the name in the symbol table
	fcn = find_symbol( DevaObject( fcn_name.c_str(), sym_unknown ) );
	if( !fcn )
		throw DevaRuntimeException( "Call made to undefined function." );

	// set the static that tracks the number of args processed
	args_on_stack = num_args;

	// pop the args off the stack so we can insert the return value
	vector<DevaObject> args;
	for( int i = 0; i < num_args; ++i )
	{
		DevaObject tmp = stack.back();
		stack.pop_back();
		args.push_back( tmp );
	}
	// push the return value (ip) onto the stack
	stack.push_back( DevaObject( "", (size_t)ip ) );

	// restore the args
	for( int i = num_args - 1; i >= 0; --i )
	{
		DevaObject tmp = args.back();
		args.pop_back();
		stack.push_back( tmp );
	}

	// get the offset for the function
	size_t offset = fcn->func_offset;
	// jump execution to the function offset
	ip = offset;

	// execute code until 'return' instruction
	// read the instructions
	while( true )
	{
		// get the next instruction in the byte code
		Instruction inst = NextInstr();

		// DoInstr returns false on 'halt' instruction
		if( !DoInstr( inst ) )
			break;

		// if this was a 'return', stop
		if( inst.op == op_return )
			break;
	}

	// TODO: anything? any validation we can do? stack checking?
}

void Executor::StartGlobalScope()
{
	current_scopes = &global_scopes;
	Enter( Instruction( op_enter ) );
}

void Executor::EndGlobalScope()
{
	Leave( Instruction( op_leave ) );
}

bool Executor::RunFile( const char* const filepath )
{
	executing_filepath = filepath;
	try
	{
		// load the file into memory
		code = LoadByteCode( filepath );
		code_blocks.push_back( code );

		// fix-up the offsets into actual machine addresses
		ip = (size_t)code;
		FixupOffsets();
		ip = (size_t)code;
		
		// read the instructions
		while( true )
		{
			// get the next instruction in the byte code
			Instruction inst = NextInstr();

			// DoInstr returns false on 'halt' instruction
			if( !DoInstr( inst ) )
				break;
		}
	}
	catch( DevaICE & e )
	{
        if( file.length() != 0 )
        {
            cout << file << ":";
            if( line != 0 )
                cout << line << ":";
        }
		cout << "Internal compiler error: " << e.what() << endl;
		return false;
	}
	catch( DevaRuntimeException & e )
	{
        if( file.length() != 0 )
        {
            cout << file << ":";
            if( line != 0 )
                cout << line << ":";
        }
		cout << e.what() << endl;
		return false;
	}

	return true;
}

bool Executor::RunText( const char* const text )
{
	unsigned char* orig_code = code;
	size_t orig_ip = ip;
	try
	{
		// load the file into memory
		code = CompileText( text, strlen( text ) );
		code_blocks.push_back( code );

		// fix-up the offsets into actual machine addresses
		ip = (size_t)code + FileHeader::size();
		FixupOffsets();
		ip = (size_t)code + FileHeader::size();
		
		// read the instructions
		while( true )
		{
			// get the next instruction in the byte code
			Instruction inst = NextInstr();

			// DoInstr returns false on 'halt' instruction
			if( !DoInstr( inst ) )
				break;
		}
	}
	catch( DevaICE & e )
	{
        if( file.length() != 0 )
        {
            cout << file << ":";
            if( line != 0 )
                cout << line << ":";
        }
		cout << "Internal compiler error: " << e.what() << endl;
		return false;
	}
	catch( DevaRuntimeException & e )
	{
        if( file.length() != 0 )
        {
            cout << file << ":";
            if( line != 0 )
                cout << line << ":";
        }
		cout << e.what() << endl;
		return false;
	}

	code = orig_code;
	ip = orig_ip;
	return true;
}
