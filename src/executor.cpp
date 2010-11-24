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

// executor.cpp
// deva language intermediate language & virtual machine execution engine
// created by jcs, september 26, 2009 

// TODO:
// * 

#include "executor.h"
#include "builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"
#include "string_builtins.h"
#include "compile.h"
#include "fileformat.h"
#include "util.h"
#include "module_bit.h"
#include "module_math.h"
#include "module_os.h"
#include "module_re.h"


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
DevaObject* Executor::find_symbol( const DevaObject & ob, ScopeTable* scopes /*= NULL*/, bool search_all_modules /*= false*/ )
{
	if( !scopes )
		scopes = current_scopes;
	
	// first see if this file is an imported module, 
	// and look in its scopes if it is
	string filepart = get_file_part( file );
	string ext = get_extension( file );
	string mod( filepart, 0, filepart.length() - ext.length() );
	ScopeTable* ns = NULL;
	vector< pair<string, ScopeTable*> >::iterator it;
	it = find_namespace( mod );
	// if we found the namespace
	if( it != namespaces.end() )
	{
		ScopeTable* ns_scopes = it->second;
		for( vector<Scope*>::reverse_iterator i = ns_scopes->rbegin(); i != ns_scopes->rend(); ++i )
		{
			// get the scope object
			Scope* p = *i;

			// check for the symbol
			if( p->count( ob.name ) != 0 )
				return p->find( ob.name )->second;
		}
	}

	// next, look in the current scopes:
	// check each scope on the stack
	for( vector<Scope*>::reverse_iterator i = scopes->rbegin(); i != scopes->rend(); ++i )
	{
		// get the scope object
		Scope* p = *i;

		// check for the symbol
		if( p->count( ob.name ) != 0 )
			return p->find( ob.name )->second;
	}
	// if this scope table isn't the 'global' scope table, 
	// we need to look in the global scope too 
	// (which is available to all scopes/modules always)
	if( scopes != global_scopes )
	{
		Scope* p = (*global_scopes)[0];
		if( p->count( ob.name ) != 0 )
			return p->find( ob.name )->second;
	}

	// finally, if we're searching all modules/namespaces, check them all
	if( search_all_modules )
	{
		for( vector< pair<string, ScopeTable*> >::iterator it = namespaces.begin(); it != namespaces.end(); ++it )
		{
			ScopeTable* ns_scopes = it->second;
			for( vector<Scope*>::reverse_iterator i = ns_scopes->rbegin(); i != ns_scopes->rend(); ++i )
			{
				// get the scope object
				Scope* p = *i;

				// check for the symbol
				if( p->count( ob.name ) != 0 )
					return p->find( ob.name )->second;
			}
		}
	}

	return NULL;
}

DevaObject* Executor::find_symbol_in_current_scope( const DevaObject & ob )
{
	// get the scope object
	Scope* p = *(current_scopes->rbegin());

	// check for the symbol
	if( p->count( ob.name ) != 0 )
		return p->find( ob.name )->second;
	else
		return NULL;
}

// helper function to locate a namespace by (module) name
// helper (functor) class
class equal_to_first
{
	string value;
public:
	equal_to_first( string val ) : value( val ) {}
	bool operator()(pair<string, Executor::ScopeTable*> n ) { return n.first == value; }
};
// find a namespace
vector< pair<string, Executor::ScopeTable*> >::iterator Executor::find_namespace( string mod )
{
	return find_if( namespaces.begin(), namespaces.end(), equal_to_first( mod ) );
}

// peek at what the next instruction is (doesn't modify ip)
Opcode Executor::PeekInstr()
{
	return (Opcode)(*((unsigned char*)ip));
}

// read a string from *ip into s
// (allocates s, which needs to be freed by the caller)
unsigned char* Executor::read_string( char* & s, unsigned char* ip /*=0*/ )
{
	if( ip == 0 )
		ip = (unsigned char*)this->ip;
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

	return (unsigned char*)ip;
}

// read a byte
unsigned char* Executor::read_byte( unsigned char & b, unsigned char* ip /*=0*/ )
{
	if( ip == 0 )
		ip = (unsigned char*)this->ip;
	memcpy( (void*)&b, (char*)(ip), sizeof( unsigned char ) );
	ip += sizeof( unsigned char );

	return (unsigned char*)ip;
}

// read a long
unsigned char* Executor::read_long( long & l, unsigned char* ip /*=0*/ )
{
	if( ip == 0 )
		ip = (unsigned char*)this->ip;
	memcpy( (void*)&l, (char*)(ip), sizeof( long ) );
	ip += sizeof( long );

	return (unsigned char*)ip;
}

// read a size_t
unsigned char* Executor::read_size_t( size_t & l, unsigned char* ip /*=0*/ )
{
	if( ip == 0 )
		ip = (unsigned char*)this->ip;
	memcpy( (void*)&l, (char*)(ip), sizeof( size_t ) );
	ip += sizeof( size_t );

	return (unsigned char*)ip;
}

// read a double
unsigned char* Executor::read_double( double & d, unsigned char* ip /*=0*/ )
{
	if( ip == 0 )
		ip = (unsigned char*)this->ip;
	memcpy( (void*)&d, (char*)(ip), sizeof( double ) );
	ip += sizeof( double );

	return (unsigned char*)ip;
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
	switch( o.Type() )
	{
	case sym_null:
		// null is always false
		return false;
	case sym_number:
		if( o.num_val != 0 )
			return true;
		else
			return false;
	case sym_boolean:
		return o.bool_val;
	case sym_string:
		if( strlen( o.str_val ) > 0 )
			return true;
		else
			return false;
	case sym_class:
	case sym_instance:
	case sym_map:
		return (void*)o.map_val->size() != 0;
	case sym_vector:
		return (void*)o.vec_val->size() != 0;
	case sym_address:
	case sym_size:
		return o.sz_val != 0;
	case sym_native_obj:
		return o.nat_obj_val != 0;
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
		throw DevaRuntimeException( boost::format( "Unable to open input file '%1%' for read." ) % filename );

	unsigned char* bytecode = NULL;

	// read the header
	char deva[5] = {0};
	char ver[6] = {0};
	file.read( deva, 5 );
	if( strcmp( deva, "deva") != 0 )
		throw DevaRuntimeException( "Invalid .dvc file: header missing 'deva' tag." );
	file.read( ver, 6 );
	if( strcmp( ver, "1.0.0" ) != 0 )
		 throw DevaRuntimeException( boost::format( "Invalid .dvc version number: %1%." ) % ver );
	char pad[6] = {0};
	file.read( pad, 5 );
	if( pad[0] != 0 || pad[1] != 0 || pad[2] != 0 || pad[3] != 0 || pad[4] != 0 )
		throw DevaRuntimeException( "Invalid .dvc file: malformed header after version number." );

	// allocate memory for the byte code array
	filebuf* buf = file.rdbuf();
	// size of the byte code array is file length minus the header size 
	// (16 bytes)
	long len = buf->pubseekoff( 0, ios::end, ios::in );
	len -= FileHeader::size();
	// (seek back to the end of the header)
	buf->pubseekpos( FileHeader::size(), ios::in );
	bytecode = new unsigned char[len];

	// read the file into the byte code array
	file.read( (char*)bytecode, len );

	// close the file
	file.close();

	return bytecode;
}

// fixup the addresses (offsets) of the instructions
// from the code block 'cd'
void Executor::FixupOffsets( unsigned char* cd /*= 0*/ )
{
	// save the current code block
	unsigned char* orig_code = code;
	code = cd;

	char* name;
	// read the instructions
	while( true )
	{
		name = NULL;

		// read the byte for the opcode
		unsigned char op;
		cd = read_byte( op, cd );
		// stop when we get to the halt instruction at the end of the buffer
		if( op == op_halt )
			break;

		// for each argument:
		unsigned char type;
		cd = read_byte( type, cd );
		while( type != sym_end )
		{
			// read the name of the arg
			cd = read_string( name, cd );
			// read the value
			switch( (SymbolType)type )
			{
				case sym_number:
					{
					// skip double
					cd += sizeof( double );
					break;
					}
				case sym_string:
					{
					// variable length, null-terminated
					// skip string
					long len = strlen( (char*)(cd) );
					cd += len+1;
					break;
					}
				case sym_boolean:
					{
					// skip long
					cd += sizeof( long );
					break;
					}
				case sym_address:
					{
						// read a size_t to get the offset
						size_t sz_val;
						memcpy( (void*)&sz_val, (char*)(cd), sizeof( size_t ) );
						// calculate the actual address
						size_t address;
						address = (size_t)code + sz_val;
						// 'fix-up' the 'bytecode' with the actual address
						size_t* p_add = (size_t*)cd;
						*p_add = address;
					} // fall-through to 'sym_size' for the ip increment
				case sym_size:
					cd += sizeof( size_t );
					break;
				case sym_null:
					{
					// skip a long
					cd += sizeof( long );
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
			cd = read_byte( type, cd );
		}
	}
	// restore the current code block
	code = orig_code;
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
		throw DevaRuntimeException( boost::format( "Invalid argument to 'load' instruction: variable '%1% not found." ) % obj.name );
	stack.push_back( *var );
}
// 3 store a variable from the stack to memory
// if there is a boolean arg with the value 'true', this is a store into a local
// variable, so a new variable should always be added
void Executor::Store( Instruction const & inst )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	DevaObject rhs = stack.back();
	stack.pop_back();
	DevaObject lhs = stack.back();
	stack.pop_back();
	// if the rhs is a variable 
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* ob = find_symbol( rhs );
		if( !ob )
			throw DevaRuntimeException( boost::format( "Reference to unknown variable '%1%'." ) % rhs.name );
		rhs = *ob;
	}
	// if the rhs is an offset, it could be a function, try to get it from the
	// symbol table
	else if( rhs.Type() == sym_address )
	{
		DevaObject* ob = find_symbol( rhs );
		// if we couldn't find it, assume it's just an offset (integral number)
		if( ob )
			rhs = *ob;
	}

	// verify the lhs is a variable (sym_unknown)
	if( lhs.Type() != sym_unknown )
		throw DevaRuntimeException( boost::format( "Attempting to assign a value into a non-variable l-value '%1%." ) % lhs.name );
	// get the lhs from the symbol table
	DevaObject* ob;

	// one boolean arg (set to 'true') means this is a store into a 'local' variable
	if( inst.args.size() == 1 && inst.args[0].Type() == sym_boolean && inst.args[0].bool_val == true )
		// look only in this scope
		ob = find_symbol_in_current_scope( lhs );
	// otherwise do a normal lookup
	else
		ob = find_symbol( lhs );

	// not found? add it to the current scope
	if( !ob )
	{
		ob = new DevaObject( lhs.name, rhs );
		current_scopes->AddObject( ob );
	}
	// lhs variable already exists, set its value to the rhs and mark the
	// original variable for later destruction (at scope exit)
	else
	{
		// if the lhs is an instance, we need to add a new object that
		// references it to the current scope, so that its destructor will be
		// called when that scope exits
		if( ob->Type() == sym_instance )
		{
			static int s_orphan_counter;
			char s[33+7] = {0};
			sprintf( s, "orphan_%d", s_orphan_counter++ );
			DevaObject* o = new DevaObject( s, *ob );
			current_scopes->AddObject( o );
		}
		*ob = DevaObject( lhs.name, rhs );
	}
}
// 4 define function. arg is location in instruction stream, named the fcn name
void Executor::Defun( Instruction const & inst )
{
	// ensure 1st arg to instruction is a function object
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid defun opcode, no arguments." );
	if( inst.args[0].Type() != sym_address )
		throw DevaICE( "Invalid defun opcode argument." );
	// create a new entry in the local symbol table
	// (offset to HERE (function start))
	size_t offset = ip;
	// also the same as: long offset = inst.args[0].sz_val + inst.args[0].Size() + 2;
	DevaObject* fcn = new DevaObject( inst.args[0].name, offset, true );
	current_scopes->AddObject( fcn );
	// skip the function body
	Opcode op = PeekInstr();
	while( op != op_endf )
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
		if( stack.size() < 1 )
			throw DevaRuntimeException( "Invalid 'defarg' opcode, no data on stack." );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as function argument." ) % o.name );
		o = *var;
	}
	// check for the symbol in the immediate (current) scope and make sure
	// we're not redef'ing the same argument (shouldn't happen, the semantic
	// checker looks for this)
	if( current_scopes->back()->count( inst.args[0].name ) != 0 )
		throw DevaICE( boost::format( "Argument with the name '%1%' already exists." ) % inst.args[0].name );
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
	static int s_map_counter = 0;

	DOMap* m = new DOMap();
	if( inst.args.size() == 1 )
	{
		if( inst.args[0].Type() != sym_size )
			throw DevaICE( "Invalid argument to 'op_new_map' instruction." );

		for( int i = 0; i < inst.args[0].sz_val; i += 2 )
		{
			DevaObject key = stack.back();
			stack.pop_back();
			DevaObject val = stack.back();
			stack.pop_back();
			DevaObject *o;
			if( key.Type() == sym_unknown )
			{
				o = find_symbol( key );
				if( !o )
					throw DevaRuntimeException( boost::format( "Invalid object '%1%' used in map initializer." ) % key.name );
				key = *o;
			}
			if( val.Type() == sym_unknown )
			{
				o = find_symbol( val );
				if( !o )
					throw DevaRuntimeException( boost::format( "Invalid object '%1%' used in map initializer." ) % val.name );
				val = *o;
			}
//			m->insert( make_pair( key, val ) );
			m->operator[]( key ) = val;
		}
	}
	// create a new object for the map
	char s[33+8] = {0};
	sprintf( s, "new_map_%d", s_map_counter++ );
	DevaObject *mp = new DevaObject( s, m );
	// add it to the current scope so that it will be properly ref-counted
	current_scopes->AddObject( mp );
	// put it onto the stack
	stack.push_back( *mp );
}
// 8 create a new vector object and push onto stack
void Executor::New_vec( Instruction const & inst )
{
	static int s_vec_counter = 0;

	// if there is an arg, it's a 'size' type that has the number of items on
	// the stack to be added to the new vector
	DOVector* v = new DOVector();
	if( inst.args.size() == 1 )
	{
		if( inst.args[0].Type() != sym_size )
			throw DevaICE( "Invalid argument to 'op_new_vec' instruction." );

		for( int i = 0; i < inst.args[0].sz_val; ++i )
		{
			DevaObject ob = stack.back();
			DevaObject *o;
			if( ob.Type() == sym_unknown )
			{
				o = find_symbol( ob );
				if( !o )
					throw DevaRuntimeException( boost::format( "Invalid object '%1%' used in vector initializer." ) % ob.name );
				ob = *o;
			}
			stack.pop_back();
			v->push_back( ob );
		}
	}
	// create a new object for the vector
	char s[33+8] = {0};
	sprintf( s, "new_vec_%d", s_vec_counter++ );
	DevaObject *vec = new DevaObject( s, v );
	// add it to the current scope so that it will be properly ref-counted
	current_scopes->AddObject( vec );
	// put it onto the stack
	stack.push_back( *vec );
}

// helper for slicing in steps, for Tbl_load instruction
static size_t s_step = 1;
static size_t s_i = 0;
bool tbl_load_slice_if_step( DevaObject )
{
	if( s_i++ % s_step == 0 )
		return false;
	else
		return true;
}
// 9 get item from vector or map
// (can't tell at compile time what it will be)
// if there is an arg that is the boolean 'true', then we need to look up items in
// maps as if the key was a numeric index (i.e. m[0] means the 0'th item in the
// map, not the item with key '0'), for use by 'for' loops
// if there is an arg that is the boolean 'false', it indicates that this is a
// method call, NOT a normal look-up, and the 'self' arg will need to be loaded
// as well as the result of the table look-up
// if there is an arg that is of sym_size type, it is an index or slice
// operation and that's how many indices are on the stack
void Executor::Tbl_load( Instruction const & inst )
{
	int num_indices = 1;
	if( inst.args.size() > 0 && inst.args[0].Type() == sym_size )
		num_indices = inst.args[0].sz_val;

	// if this is a slice
	if( num_indices > 1 )
	{
		if( num_indices > 3 )
			throw DevaICE( "Too many arguments to tbl_load instruction." );

		DevaObject start_idx = stack.back();
		stack.pop_back();
		DevaObject end_idx, step_val, table;
		if( num_indices == 2 )
		{
			end_idx = stack.back();
			stack.pop_back();
			step_val = DevaObject( "", 1.0 );
			table = stack.back();
			stack.pop_back();
		}
		else
		{
			end_idx = stack.back();
			stack.pop_back();
			step_val = stack.back();
			stack.pop_back();
			table = stack.back();
			stack.pop_back();
		}
		// ensure the args are numeric
		if( start_idx.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( start_idx );
			if( !o )
				throw DevaRuntimeException( "Invalid object used in slice 'start' index." );
			start_idx = *o;
		}
		if( start_idx.Type() != sym_number )
			throw DevaRuntimeException( "Invalid type for slice 'start' index: must be numeric." );
		if( end_idx.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( end_idx );
			if( !o )
				throw DevaRuntimeException( "Invalid object used in slice 'end' index." );
			end_idx = *o;
		}
		if( end_idx.Type() != sym_number )
			throw DevaRuntimeException( "Invalid type for slice 'end' index: must be numeric." );
		if( step_val.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( step_val );
			if( !o )
				throw DevaRuntimeException( "Invalid object used in slice 'step' value." );
			step_val = *o;
		}
		if( step_val.Type() != sym_number )
			throw DevaRuntimeException( "Invalid type for slice 'step' value: must be numeric." );

		// ensure the table is a vector or string
		if( table.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( table );
			if( !o )
				throw DevaRuntimeException( boost::format( "Invalid object '%1%' for slice operation." ) % table.name );
			table = *o;
		}
		if( table.Type() != sym_vector && table.Type() != sym_string )
				throw DevaRuntimeException( "Invalid type for slice operation: must be a string or vector." );

		// convert to integer values
		// TODO: error on non-integer numbers
		size_t start = (size_t)start_idx.num_val;
		size_t end = (size_t)end_idx.num_val;
		size_t step = (size_t)step_val.num_val;

		size_t sz;
		if( table.Type() == sym_vector )
			sz = table.vec_val->size();
		else
			sz = strlen( table.str_val );

		// convert values of "-1" to an end slice
		if( start == -1 )
			start = sz - 1;
		if( end == -1 )
			end = sz;

		// check the indices & step value
		if( start >= sz || start < 0 )
			throw DevaRuntimeException( "Invalid 'start' index slice." );
		if( end > sz || end < 0 )
			throw DevaRuntimeException( "Invalid 'end' index in slice." );
		if( end < start )
			throw DevaRuntimeException( "Invalid slice indices in slice: 'start' is greater than 'end'." );
		if( step < 1 )
			throw DevaRuntimeException( "Invalid 'step' argument in slice: 'step' is less than one." );

		// perform the slice
		if( table.Type() == sym_vector )
		{
			DevaObject ret;
			// 'step' is '1' (the default)
			if( step == 1 )
			{
				// create a new vector object that is a copy of the 'sub-vector' we're
				// looking for
				DOVector* v = new DOVector( *(table.vec_val), start, end );
				ret = DevaObject( "", v );
			}
			// otherwise the vector class doesn't help us, have to do it manually
			else
			{
				DOVector* v = new DOVector();
				s_i = 0;
				s_step = step;
				remove_copy_if( table.vec_val->begin() + start, table.vec_val->begin() + end, back_inserter( *v ), tbl_load_slice_if_step );
				ret = DevaObject( "", v );
			}
			stack.push_back( ret );
			return;
		}
		else
		{
			DevaObject ret;
			string s( table.str_val );
			// 'step' is '1' (the default)
			if( step == 1 )
			{
				string r = s.substr( start, end - start );
				ret = DevaObject( "", r );
			}
			// otherwise the string class doesn't help us, have to do it manually
			else
			{
				// first get the substring from start to end positions
				string r = s.substr( start, end - start );
				// TODO: call 'reserve' on the string to reduce allocations?
				// then walk it grabbing every 'nth' character
				string slice;
				for( int i = 0; i < r.length(); i += step )
				{
					slice += r[i];
				}
				ret = DevaObject( "", slice );
			}
			stack.push_back( ret );
			return;
		}
	}

	// top of stack has indices/key
	DevaObject idxkey = stack.back();
	stack.pop_back();
	// next-to-top of stack has name of vector/map
	DevaObject vecmap = stack.back();
	stack.pop_back();

	if( vecmap.Type() != sym_unknown && 
		vecmap.Type() != sym_map && 
		vecmap.Type() != sym_vector &&
		vecmap.Type() != sym_class &&
		vecmap.Type() != sym_instance &&
		vecmap.Type() != sym_string )
		throw DevaRuntimeException( "Invalid object for 'tbl_load' instruction." );

	// if the index/key is a variable, look it up
	if( idxkey.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( idxkey );
		if( !o )
			throw DevaRuntimeException( boost::format( "'%1%' Invalid type for index/key." ) % idxkey.name );
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
			vector< pair<string, ScopeTable*> >::iterator it;
			it = find_namespace( vecmap.name );
			// if we found the namespace
			if( it != namespaces.end() )
			{
				ns = it->second;
				// look up the key in it (key should be a string)
				if( idxkey.Type() != sym_string )
					throw DevaICE( "Trying to look-up a non-string key in a namespace." );
				DevaObject key( idxkey.str_val, sym_unknown );
				DevaObject* obj = find_symbol( key, ns );
				if( !obj )
				{
					// try looking it up as a fcn in a built-in module 
					// (i.e. 'fcn@module')
					string fcn_mod( idxkey.str_val );
					fcn_mod += "@";
					fcn_mod += vecmap.name;
					obj = find_symbol( DevaObject( fcn_mod, sym_unknown ), ns );
					if( !obj )
						throw DevaRuntimeException( boost::format( "Attempt to reference undefined object '%1%' in namespace '%2%'." ) % fcn_mod % vecmap.name );
				}
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
			// if not found, try the map built-in methods...
			if( it == mp->end() )
			{
				// built-in method call??
				// top of stack contains key (fcn name as string)
				// next-to-top contains value (fcn as sym_address)
				string key( "map_");
				key += idxkey.str_val; 
				// key == string
				// table == map
				// look up 'key' in the map built-ins
				if( !is_map_builtin( key ) )
					throw DevaRuntimeException( boost::format( "Invalid method or field '%1%'. No such item found." ) % idxkey.str_val );
				// push the map as an argument
				stack.push_back( *table );
				// push a fcn with 'key' as its name (it's a builtin, so the offset
				// given is irrelevant)
				stack.push_back( DevaObject( key.c_str(), (size_t)-1, true ) );
				return;
			}
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
			// if not found, try the map built-in methods...
			if( it == mp->end() )
			{
				// built-in method call??
				// top of stack contains key (fcn name as string)
				// next-to-top contains value (fcn as sym_address)
				string key( "map_");
				key += idxkey.str_val; 
				// key == string
				// table == map
				// look up 'key' in the map built-ins
				if( !is_map_builtin( key ) )
					throw DevaRuntimeException( boost::format( "Invalid method or field '%1%'. No such item found." ) % idxkey.str_val );
				// push the map as an argument
				stack.push_back( *table );
				// push a fcn with 'key' as its name (it's a builtin, so the offset
				// given is irrelevant)
				stack.push_back( DevaObject( key.c_str(), (size_t)-1, true ) );
				return;
			}
		}
		// for method calls, push 'self'
		// was the method-call flag passed with this instruction?
		if( is_method && inst.args.size() > 0 && inst.args[0].Type() == sym_boolean && inst.args[0].bool_val == false )
		{
			stack.push_back( *table );
		}
		// push the field/method
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
			// next-to-top contains value (fcn as sym_address)
			string key( "vector_");
		   	key += idxkey.str_val; 
			// key == string
			// table == vector
			// look up 'key' in the vector built-ins
			if( !is_vector_builtin( key ) )
				throw DevaRuntimeException( boost::format( "'%1%' is not a valid method on type 'vector'." ) % idxkey.str_val );
			// push the vector as an argument
			stack.push_back( *table );
			// push a fcn with 'key' as its name (it's a builtin, so the offset
			// given is irrelevant)
			stack.push_back( DevaObject( key.c_str(), (size_t)-1, true ) );
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
			if( idxkey.Type() != sym_number &&  
				idxkey.Type() != sym_string &&
				idxkey.Type() != sym_class &&
				idxkey.Type() != sym_instance )
				throw DevaRuntimeException( "Argument to '[]' on a map MUST evaluate to a number, string or user-defined-type." );
			// get the value from the map
			smart_ptr<DOMap> mp( table->map_val );
			DOMap::iterator it;
			it = mp->find( idxkey );

			// if not found, try the map built-in methods...
			// (must be a string)
			if( it == mp->end() )
			{
				if( idxkey.Type() == sym_string )
				{
					// built-in method call??
					// top of stack contains key (fcn name as string)
					// next-to-top contains value (fcn as sym_address)
					string key( "map_");
					key += idxkey.str_val; 
					// key == string
					// table == map
					// look up 'key' in the map built-ins
					if( !is_map_builtin( key ) )
						throw DevaRuntimeException( boost::format( "'%1%': invalid map key or method. No such item found." ) % idxkey.str_val );
					// push the map as an argument
					stack.push_back( *table );
					// push a fcn with 'key' as its name (it's a builtin, so the offset
					// given is irrelevant)
					stack.push_back( DevaObject( key.c_str(), (size_t)-1, true ) );
					return;
				}
				else
					throw DevaRuntimeException( "Invalid map key or method. No such item found." );
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
			// next-to-top contains value (fcn as sym_address)
			string key( "string_");
		   	key += idxkey.str_val; 
			// key == string
			// table == string
			// look up 'key' in the string built-ins
			if( !is_string_builtin( key ) )
				throw DevaRuntimeException( boost::format( "'%1%' is not a valid method on type 'string'." ) % idxkey.str_val );
			// push the string as an argument
			stack.push_back( *table );
			// push a fcn with 'key' as its name (it's a builtin, so the offset
			// given is irrelevant)
			stack.push_back( DevaObject( key.c_str(), (size_t)-1, true ) );
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
	// enough data on stack?
	if( stack.size() < 3 )
		throw DevaICE( "Invalid 'tbl_store' instruction: not enough data on stack." );

	int num_indices = 1;
	if( inst.args.size() > 0 && inst.args[0].Type() == sym_size )
		num_indices = inst.args[0].sz_val;

	// if this is a slice
	if( num_indices > 1 )
	{
		if( num_indices > 3 )
			throw DevaICE( "Too many arguments to tbl_load instruction." );

		// top of stack has value
		DevaObject val = stack.back();
		stack.pop_back();
		// if the value is a variable, look it up
		if( val.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( val );
			if( !o )
				throw DevaRuntimeException( boost::format( "'%1%': invalid operand to a vector store operation." ) % val.name );
			val = *o;
		}

		DevaObject start_idx = stack.back();
		stack.pop_back();
		DevaObject end_idx, step_val, table;
		if( num_indices == 2 )
		{
			end_idx = stack.back();
			stack.pop_back();
			step_val = DevaObject( "", 1.0 );
			table = stack.back();
			stack.pop_back();
		}
		else
		{
			end_idx = stack.back();
			stack.pop_back();
			step_val = stack.back();
			stack.pop_back();
			table = stack.back();
			stack.pop_back();
		}
		// ensure the args are numeric
		if( start_idx.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( start_idx );
			if( !o )
				throw DevaRuntimeException( "Invalid object used in slice 'start' index." );
			start_idx = *o;
		}
		if( start_idx.Type() != sym_number )
			throw DevaRuntimeException( "Invalid type for slice 'start' index: must be numeric." );
		if( end_idx.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( end_idx );
			if( !o )
				throw DevaRuntimeException( "Invalid object used in slice 'end' index." );
			end_idx = *o;
		}
		if( end_idx.Type() != sym_number )
			throw DevaRuntimeException( "Invalid type for slice 'end' index: must be numeric." );
		if( step_val.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( step_val );
			if( !o )
				throw DevaRuntimeException( "Invalid object used in slice 'step' value." );
			step_val = *o;
		}
		if( step_val.Type() != sym_number )
			throw DevaRuntimeException( "Invalid type for slice 'step' value: must be numeric." );

		// ensure the table is a vector
		if( table.Type() == sym_unknown )
		{
			DevaObject* o = find_symbol( table );
			if( !o )
				throw DevaRuntimeException( boost::format( "Invalid object '%1%' for slice assignment." ) % table.name );
			table = *o;
		}
		if( table.Type() != sym_vector )
				throw DevaRuntimeException( "Invalid type for slice assignment: must be a vector." );

		// convert to integer values
		// TODO: error on non-integer numbers
		size_t start = (size_t)start_idx.num_val;
		size_t end = (size_t)end_idx.num_val;
		size_t step = (size_t)step_val.num_val;

		size_t sz = table.vec_val->size();

		// check the indices & step value
		if( start >= sz || start < 0 )
			throw DevaRuntimeException( "Invalid 'start' index slice." );
		if( end > sz || end < 0 )
			throw DevaRuntimeException( "Invalid 'end' index in slice." );
		if( end < start )
			throw DevaRuntimeException( "Invalid slice indices in slice: 'start' is greater than 'end'." );
		if( step < 1 )
			throw DevaRuntimeException( "Invalid 'step' argument in slice: 'step' is less than one." );

		// perform the slice
		// 'step' is '1' (the default) is a simple insertion
		if( step == 1 )
		{
			// first erase the destination range
			table.vec_val->erase( table.vec_val->begin() + start, table.vec_val->begin() + end );
			// if the source is a vector, insert its contents
			if( val.Type() == sym_vector )
				table.vec_val->insert( table.vec_val->begin() + start, val.vec_val->begin(), val.vec_val->end() );
			// otherwise insert whatever the object is
			table.vec_val->insert( table.vec_val->begin() + start, val );
		}
		// other steps are separate deletions and insertions
		else
		{
			// ensure the source is a vector
			if( val.Type() != sym_vector )
				throw DevaRuntimeException( "Source in slice assignment must be a vector." );
			// then ensure the destination and source lengths are identical
			if( val.vec_val->size() != (int)((end - start + 1)/step) )
				throw DevaRuntimeException( "Source in slice assignment must be the same size as the destination." );
			int j = 0;
			for( int i = 0; i < end - start; ++i )
			{
				if( i % step == 0 )
					table.vec_val->at( start + i ) = val.vec_val->at( j++ );
			}
		}
		return;
	}

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
			throw DevaRuntimeException( boost::format( "'%1%': invalid type for operand to a vector store operation." ) % val.name );
		val = *o;
	}
	// if the index/key is a variable, look it up
	if( idxkey.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( idxkey );
		if( !o )
			throw DevaRuntimeException( boost::format( "'%1%': invalid type for index/key." ) % idxkey.name );
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
		if( idxkey.Type() != sym_number &&  
			idxkey.Type() != sym_string &&
			idxkey.Type() != sym_class &&
			idxkey.Type() != sym_instance )
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
	line = inst.args[1].sz_val;
}
// 13 unconditional jump to the address on top of the stack
void Executor::Jmp( Instruction const & inst )
{
	// one arg: the offset to jump to (as a 'function'/offset/address type)
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid jmp instruction: no arguments." );
	DevaObject dest = inst.args[0];
	if( dest.Type() != sym_address )
		throw DevaICE( "Invalid jmp instruction argument: not a jump target offset." );
	// jump execution to the function offset
	ip = dest.sz_val;
}
// 14 jump on top of stack evaluating to false 
void Executor::Jmpf( Instruction const & inst )
{
	// one arg: the offset to jump to (as a 'function'/offset/address type)
	if( inst.args.size() < 1 )
		throw DevaICE( "Invalid jmp instruction: no arguments." );
	DevaObject dest = inst.args[0];
	if( dest.Type() != sym_address )
		throw DevaICE( "Invalid jmp instruction argument: not a jump target offset." );

	// get the value on the top of the stack
	DevaObject o = stack.back();
	stack.pop_back();
	// if it is a variable, lookup the variable in the symbol table
	if( o.Type() == sym_unknown )
	{
		DevaObject* var = find_symbol( o );
		if( !var )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' referenced in jmpf instruction." ) % o.name );
		o = *var;
	}
	// if it evaluates to 'true', return
	if( evaluate_object_as_boolean( o ) )
		return;
	// else jump to the offset in the argument
	ip = dest.sz_val;
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in equality comparision." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in equality comparision." ) % rhs.name );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
	{
		stack.push_back( DevaObject( "", false ) );
		return;
	}
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in neq comparision." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in neq comparision." ) % rhs.name );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
	{
		stack.push_back( DevaObject( "", true ) );
		return;
	}
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in lt comparision." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in lt comparision." ) % rhs.name );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "'Less than' comparison of incompatible types." );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in lte comparision." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in lte comparision." ) % rhs.name );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "'Less than or equals' comparison of incompatible types." );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in gt comparision." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in gt comparision." ) % rhs.name );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "'Greater than' comparison of incompatible types." );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in gte comparision." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in gte comparision." ) % rhs.name );
		rhs = *o;
	}
	// if they are the same type, compare them and push the (boolean) result
	// onto the stack
	if( lhs.Type() != rhs.Type() )
		throw DevaRuntimeException( "'Greater than or equals' comparison of incompatible types." );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'or' instruction." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'or' instruction." ) % rhs.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'and' instruction." ) % lhs.name );
		lhs = *o;
	}
	if( rhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( rhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'and' instruction." ) % rhs.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as operand in 'negate' operation." ) % op.name  );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as operand in 'not' operation." ) % op.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'add' instruction." ) % rhs.name );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'add' instruction." ) % lhs.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'subtract' instruction." ) % rhs.name );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'subtract' instruction." ) % lhs.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'multiply' instruction." ) % rhs.name );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'multiply' instruction." ) % lhs.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'divide' instruction." ) % rhs.name );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'divide' instruction." ) % lhs.name );
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
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as right-hand operand in 'modulus' instruction." ) % rhs.name );
		rhs = *o;
	}
	if( lhs.Type() == sym_unknown )
	{
		DevaObject* o = find_symbol( lhs );
		if( !o )
			throw DevaRuntimeException( boost::format( "Undefined variable '%1%' used as left-hand operand in 'modulus' instruction." ) % lhs.name );
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
			throw DevaRuntimeException( boost::format( "Symbol '%1%' not found in function call." ) % obj.name );
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
		case sym_address:
		case sym_size:
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
static int next_enter_is_from_call = -1;
static vector<int> stack_sizes;
// 31 call a function. arguments on stack
void Executor::Call( Instruction const & inst )
{
	// check for built-in function first
	if( inst.args.size() == 2 && is_builtin( inst.args[0].name ) )
	{
		// set the static that tracks the number of args processed
		args_on_stack = inst.args[1].sz_val;
		// built-ins will clear the error flag themselves (or not, in the case
		// of the error/geterror/seterror fcns)
		execute_builtin( this, inst );
		// reset the static that tracks the number of args (builtins don't run
		// the 'return' instruction)
		args_on_stack = -1;
	}
	else
	{
		bool is_destructor = false;

		DevaObject* fcn;
		// if there's more than one arg, the first is the name of the fcn to call 
		// and the second is the number of args passed to it (on the stack)
		if( inst.args.size() == 2 )
		{
			// look up the name in the symbol table
			fcn = find_symbol( inst.args[0] );
			if( !fcn )
				throw DevaRuntimeException( boost::format( "Call made to undefined function '%1%'." ) % inst.args[0].name );

			if( fcn->Type() == sym_class )
				throw DevaRuntimeException( "Trying to call a class as a function. Missing 'new' keyword?" );
			else if( fcn->Type() != sym_address )
				throw DevaRuntimeException( "Trying to call an object that is not a function or method." );

			// check the number of args to the fcn
			if( inst.args[1].Type() != sym_size )
				throw DevaICE( "Function call doesn't indicate number of arguments passed." );

			// set the static that tracks the number of args processed
			// methods on UDTs (classes and instances)
			// need to adjust arg count for the implicit 'self' (+1)
			if( fcn->name.find( '@' ) != string::npos )
				args_on_stack = inst.args[1].sz_val + 1;
			else
				args_on_stack = inst.args[1].sz_val;

			if( fcn->name.find( "delete@" ) != string::npos )
				is_destructor = true;
		}
		// if there's one arg (the num of args to the fcn), 
		// then it's a method invokation,
		// pop the top of the stack for the fcn to call
		else if( inst.args.size() == 1 )
		{
			// check the number of args to the fcn
			if( inst.args[0].Type() != sym_size )
				throw DevaICE( "Function call doesn't indicate number of arguments passed." );

			// get the function to call off the stack
			DevaObject o = stack.back();
			stack.pop_back();
			if( o.Type() == sym_address )
				fcn = &o;
			else if( o.Type() == sym_unknown )
				fcn = find_symbol( o.name );
			else if( o.Type() == sym_class )
				throw DevaRuntimeException( "Trying to call a class as a function. Missing 'new' keyword?" );
			else
				throw DevaRuntimeException( "Trying to call an object that is not a function or method." );

			if( !fcn )
				throw DevaRuntimeException( boost::format( "Unable to resolve function '%1%'." ) % o.name );

			// if this is a built-in module function, execute it
			if( builtin_module_fcns.find( fcn->name ) != builtin_module_fcns.end() )
			{
				// set the static that tracks the number of args processed
				args_on_stack = inst.args[0].sz_val;

				builtin_module_fcns[fcn->name]( this );

				// reset the static that tracks the number of args (builtins don't run
				// the 'return' instruction)
				args_on_stack = -1;
				return;
			}
			// if this is a vector builtin method, execute it
			if( is_vector_builtin( fcn->name ) )
			{
				// set the static that tracks the number of args processed
				args_on_stack = inst.args[0].sz_val;

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
				args_on_stack = inst.args[0].sz_val;

				execute_map_builtin( this, fcn->name );

				// reset the static that tracks the number of args (builtins don't run
				// the 'return' instruction)
				args_on_stack = -1;
				return;
			}
			// if this is a string builtin method, execute it
			if( is_string_builtin( fcn->name ) )
			{
				// set the static that tracks the number of args processed
				args_on_stack = inst.args[0].sz_val;

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
				args_on_stack = inst.args[0].sz_val + 1;
			else
				args_on_stack = inst.args[0].sz_val;
		}
		else
			throw DevaICE( "Invalid number of arguments to 'call' instruction." );

		// clear the global error flag before making any call to deva code
		// (built-ins clear it themselves, so that the error/geterror/seterror
		// built-ins can _not_ clear it, avoiding the situation where it can
		// never actually be retrieved)
		if( !is_destructor )
			SetError( false );

		// save the stack size
		stack_sizes.push_back( stack.size() - args_on_stack );

		// get the offset for the function
		size_t offset = fcn->sz_val;
		// jump execution to the function offset
		ip = offset;
		// set function tracker
		function = fcn->name;
		// set the static that tracks whether an enter instruction is due to a
		// fcn call or not
		next_enter_is_from_call = line;
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
	// evaluate the return value *before* leaving this scope,
	// and push the evaluated version, not the variable
	if( ret.Type() == sym_unknown )
	{
		DevaObject* rv = find_symbol( ret );
		if( !rv )
			throw DevaRuntimeException( "Invalid object for return value." );

		ret = DevaObject( *rv );
	}

	// clear the stack of any local variables
	int stack_size = stack_sizes.back();
	stack_sizes.pop_back();
	int num_locals = stack.size() - stack_size;
	while( num_locals > 0 ) // > 1 because the return address is on there
	{
		DevaObject o = stack.back();
		stack.pop_back();
		num_locals--;
	}

	// pop the return location off the top of the stack
	DevaObject ob = stack.back();
	stack.pop_back();
	if( ob.Type() != sym_address )
		throw DevaICE( "Invalid return destination on stack when executing return instruction." );
	stack.push_back( ret );
	// jump to the return location
	size_t offset = ob.sz_val;
	ip = offset;
	// keep performing 'Leave's until the top Frame of the stack trace is
	// a call site??
	vector<Frame>::reverse_iterator i = trace.rbegin();
	while( i != trace.rend() && i->call_site == -1 )
	{
		Leave( inst );
		i = trace.rbegin();
	}
	// and leave the call site too
	Leave( inst );
	// current function is now the top of the stack trace
	i = trace.rbegin();
	if( i != trace.rend() )
		function = i->function;
	// (or, if we're at the end of the trace & thus in the shutdown process, 'none')
	else
		function = "<None>";
	// reset the static that tracks the number of args processed
	args_on_stack = -1;
}
// 33 break out of loop, respecting scope (enter/leave)
void Executor::Break( Instruction const & inst )
{
	throw DevaICE( "Invalid instruction: 'break' op is deprecated." );
}
// 34 enter new scope
void Executor::Enter( Instruction const & inst )
{
	// add scope
	current_scopes->Push();
	// add frame
	trace.Push( *this, next_enter_is_from_call );
	// reset the static that tracks whether an enter instruction is due to a
	// fcn call or not (only the first enter can be due to the fcn call)
	next_enter_is_from_call = -1;
}

// 35 leave scope
void Executor::Leave( Instruction const & inst )
{
	if( current_scopes->size() == 0 )
		throw DevaICE( "Invalid 'Leave' operation. No scopes to exit." );
	
	// pop scope
	current_scopes->Pop();
	// pop frame
	trace.Pop();
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
		modpath = join_paths( modpath, *it );
	}
	// check for .dv/.dvc files on disk
	struct stat statbuf;
	// if we can open the module file, return it
	string dv = modpath + ".dv";
	if( stat( dv.c_str(), &statbuf ) != -1 )
		return modpath;
	string dvc = modpath + ".dvc";
	if( stat( dvc.c_str(), &statbuf ) != -1 )
		return modpath;
	// otherwise check the paths in the DEVA env var
	else
	{
		// get the DEVA env var
		string devapath( getenv( "DEVA" ) );
		// split it into separate paths (on the ":" char in un*x)
		vector<string> paths = split_env_var_paths( devapath );
		// for each of the paths, append the mod
		// and see if it exists
		for( vector<string>::iterator it = paths.begin(); it != paths.end(); ++it )
		{
			modpath = join_paths( *it, mod );
			// check for .dv/.dvc files on disk
			struct stat statbuf;
			// if we can't open the module file, error out
			string dv = modpath + ".dv";
			if( stat( dv.c_str(), &statbuf ) != -1 )
				return modpath;
			string dvc = modpath + ".dvc";
			if( stat( dvc.c_str(), &statbuf ) != -1 )
				return modpath;
		}
	}
	// not found, error
	throw DevaRuntimeException( "Unable to locate module for import." );
}
// 38 import module, 1 arg: module name
void Executor::Import( Instruction const & inst )
{
	// first argument has the name of the module to import
	if( inst.args.size() < 1 )
		throw DevaICE( "No module name given in import statement." );

	string mod = inst.args[0].str_val;

	// check the list of builtin modules first
	if( ImportBuiltinModule( mod ) )
		return;

	// prevent importing the same module more than once
	vector< pair<string, ScopeTable*> >::iterator it;
	it = find_namespace( mod );
	// if we found the namespace
	if( it != namespaces.end() )
		return;

	// otherwise look for the .dv/.dvc file to import
	string path = find_module( mod );

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
	ScopeTable* st = new ScopeTable( this );
	namespaces.push_back( pair<string, ScopeTable*>(mod, st) );
	current_scopes = st;
	// create a 'file/module' level scope for the namespace
	current_scopes->Push();
	// compile the file, if needed
	CompileAndWriteFile( dvfile.c_str(), mod.c_str() );
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
	// merge and store its parents
	int num_args = inst.args.size();
	DOVector* bases = new DOVector();
	bases->reserve( num_args );
	for( int c = 0; c < num_args; ++c )
	{
		DevaObject base = inst.args[c];
		// look-up the base class
		DevaObject* ob;
		if( base.Type() != sym_unknown )
			throw DevaICE( "Invalid base class for class definition." );
		ob = find_symbol( base );
		if( !ob )
			throw DevaRuntimeException( boost::format( "Invalid base class name '%1%' for class definition." ) % base.name );
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
			// ignore non-method things (namely "__bases__")
			if( pos == string::npos )
				continue;
			if( pos != string::npos )
				name.replace( pos+1, name.length() - pos, cls_name.name );
			cls.map_val->insert( make_pair( DevaObject( "", name ), i->second ) );
		}

		// add it to the list of parents
		bases->push_back( DevaObject( ob->name, *ob ) );
	}
	// add the "__bases__" member
	cls.map_val->insert( make_pair( DevaObject( "", string( "__bases__" ) ), DevaObject( "", bases ) ) );

	// - add the __name__ member
	cls.map_val->insert( make_pair( DevaObject( "", string( "__name__" ) ), DevaObject( "", cls_name.name ) ) );

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
	if( inst.args[0].Type() != sym_size )
		throw DevaICE( "Invalid argument in 'new_instance' instruction." );
	// class to create instance of is on top of stack
	DevaObject cls = stack.back();
	stack.pop_back();

	DevaObject* ob;
	if( cls.Type() == sym_unknown )
	{
		ob = find_symbol( cls );
		if( !ob )
			throw DevaRuntimeException( boost::format( "Invalid class name '%1%' for new object." ) % cls.name );
	}
	else
		ob = &cls;

	// ensure it's a class
	if( ob->Type() != sym_class )
		throw DevaRuntimeException( "Invalid class type for new object." );

	// create a new instance object as a copy of the class object
	DOMap* m = new DOMap( *(ob->map_val) );
	DevaObject instance = DevaObject::InstanceFromMap( "", m );
	// - add the __class__ member to the instance
	instance.map_val->insert( make_pair( DevaObject( "", string( "__class__" ) ), DevaObject( "", ob->name ) ) );

	// push it onto the stack
	stack.push_back( instance );
}

// 42 roll the stack from a given position. 1 arg: the position to roll from
void Executor::Roll( Instruction const & inst )
{
	// first argument has the position to roll from
	if( inst.args.size() < 1 )
		throw DevaICE( "No stack position given in roll instruction." );

	if( inst.args[0].Type() != sym_size )
		throw DevaICE( "Invalid stack position given in roll instruction." );
	
	size_t pos = inst.args[0].sz_val;

	stack.roll( pos );
}

// illegal operation, if exists there was a compiler error/fault
void Executor::Illegal( Instruction const & inst )
{
	throw DevaICE( "Illegal Instruction." );
}
///////////////////////////////////////////////////////////


// execute single instruction
// returns false on halt or breakpoint
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
		// check for breakpoints
		{
			// try full paths
			pair<string, int> p = make_pair( file, line );
			if( find( breakpoints.begin(), breakpoints.end(), p ) != breakpoints.end() )
				return false;
			// then file names
			pair<string, int> p2 = make_pair( get_file_part( file ), line );
			if( find( breakpoints.begin(), breakpoints.end(), p2 ) != breakpoints.end() )
				return false;
		}
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
	case op_endf:			// 41 endf, no-op marking end of defun
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
	case op_roll:
		Roll( inst );
		break;
	case op_illegal:	// illegal operation, if exists there was a compiler error/fault
	default:
		Illegal( inst );
		break;
	}

	if( debug_mode )
	{
		cout << " - stack: ";
		for( int c = stack.size()-1; c >= 0; --c )
		{
			DevaObject o = stack[c];
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
	size_t sz_val;
	// read the byte for the opcode
	unsigned char op;
	ip = (size_t)read_byte( op );
	Instruction inst( (Opcode)op );
	// for each argument:
	unsigned char type;
	ip = (size_t)read_byte( type );
	while( type != sym_end )
	{
		// read the name of the arg
		ip = (size_t)read_string( name );
		// read the value
		switch( (SymbolType)type )
		{
			case sym_number:
				{
				// 64 bit double
				ip = (size_t)read_double( num_val );
				DevaObject ob( name, num_val );
				inst.args.push_back( ob );
				break;
				}
			case sym_string:
				{
				// variable length, null-terminated
				ip = (size_t)read_string( str_val );
				DevaObject ob( name, string( str_val ) );
				delete [] str_val;
				inst.args.push_back( ob );
				break;
				}
			case sym_boolean:
				{
				// 32 bit long
				ip = (size_t)read_long( bool_val );
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
			case sym_address:
				{
				// size_t
				ip = (size_t)read_size_t( sz_val );
				DevaObject ob( name, (size_t)sz_val, true );
				inst.args.push_back( ob );
				break;
				}
			case sym_size:
				{
				// size_t
				ip = (size_t)read_size_t( sz_val );
				DevaObject ob( name, (size_t)sz_val, false );
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
				ip = (size_t)read_long( n );
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
		ip = (size_t)read_byte( type );
	}
	return inst;
}

// public methods
///////////////////////////////////////////////////////////
Executor::Executor( bool dbg ) : debug_mode( dbg ), code( NULL ), ip( 0 ), file( "" ), line( 0 ), is_error( false ), error_data( NULL ), global_scopes( NULL ), current_scopes( NULL ) 
{
	global_scopes = new ScopeTable( this );
}

Executor::~Executor()
{
	// free the global scopes object
	delete global_scopes;
	global_scopes = NULL;
}

void Executor::ExecuteDevaFunction( string fcn_name, int num_args, ScopeTable* ns /*= NULL*/ )
{
	DevaObject* fcn;
	// look up the name in the symbol table
	fcn = find_symbol( DevaObject( fcn_name.c_str(), sym_unknown ), ns );
	if( !fcn )
		throw DevaRuntimeException( boost::format( "Call made to undefined function '%1%'." ) % fcn_name );

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
	stack.push_back( DevaObject( "", (size_t)ip, true ) );

	// restore the args
	for( int i = num_args - 1; i >= 0; --i )
	{
		DevaObject tmp = args.back();
		args.pop_back();
		stack.push_back( tmp );
	}

	// save the stack size
	stack_sizes.push_back( stack.size() - args_on_stack );

	size_t orig_ip = ip;
	// get the offset for the function
	size_t offset = fcn->sz_val;
	// jump execution to the function offset
	ip = offset;

	// set function tracker
	function = fcn->name;
	// set the static that tracks whether an enter instruction is due to a
	// fcn call or not
	next_enter_is_from_call = line;

	// read the instructions
	bool done = false;
	while( true )
	{
		// get the next instruction in the byte code
		Instruction inst = NextInstr();

		if( inst.op == op_return )
		{
			// are we jumping back to the original ip?
			if( stack.size() < 2 )
				throw DevaICE( "Not enough values on stack for 'return' instruction." );
			DevaObject addr = stack[stack.size()-2];
			if( addr.Type() != sym_address )
				throw DevaICE( "Return address not correct type." );
			if( addr.sz_val == orig_ip )
				done = true;
		}

		// DoInstr returns false on 'halt' instruction or breakpoint
		if( !DoInstr( inst ) )
		{
			// do not stop on breakpoints in fcns that need to return to c++
			// code!
			if( inst.op == op_halt )
				break;
		}

		// if this was the matching 'return', stop
		if( inst.op == op_return && done )
			break;
	}
}

void Executor::StartGlobalScope()
{
	current_scopes = global_scopes;
	Enter( Instruction( op_enter ) );
}

void Executor::EndGlobalScope()
{
	// tear down the global scope and namespaces

	if( current_scopes->size() == 0 )
		throw DevaICE( "Invalid exit from the global scope. All scopes have already exited!." );

	// delete the instances in the global scope
	current_scopes->back()->DeleteInstances();

	// delete the namespaces
	for( vector< pair<string, ScopeTable*> >::iterator i = namespaces.begin(); i != namespaces.end(); ++i )
	{
		delete i->second;
		i->second = NULL;
	}

	// delete the rest of the objects in the global scope
	current_scopes->back()->DeleteNonInstances();

	// delete (pop) the global namespace scope
	current_scopes->Pop();
	// and its frame
	trace.Pop();

	// free the code blocks
	for( vector<unsigned char*>::iterator i = code_blocks.begin(); i != code_blocks.end(); ++i )
	{
		delete [] *i;
		*i = NULL;
	}
}

// add a compiled byte-code block and fix-up its addresses
void Executor::AddCodeBlock( unsigned char* cd )
{
	FixupOffsets( cd );
	code_blocks.push_back( cd );
}

// run a code block (that is, a block of bytecode that has already had
// its addresses fixed up)
void Executor::RunCode( unsigned char* cd, bool stop_at_breakpoints /*= true*/ )
{
	// save the current code & ip, if any
	unsigned char* orig_code = code;
	size_t orig_ip = ip;

	// set the new code & ip
	code = cd;
	ip = (size_t)code;

	// execute the instructions
	Run( stop_at_breakpoints );

	// restore the old code & ip
	code = orig_code;
	ip = orig_ip;
}

void Executor::RunFile( const char* const filepath )
{
	executing_filepath = filepath;
	// load the file into memory
	unsigned char* cd = LoadByteCode( filepath );
	code_blocks.push_back( cd );

	// fix-up the offsets into actual machine addresses
	FixupOffsets( cd );

	// run the code
	RunCode( cd );
}

// TODO: RunText should return the index of the code block 
void Executor::RunText( const char* const text )
{
	// save the current file, code & ip, if any
	string old_file = file;
	unsigned char* orig_code = code;
	size_t orig_ip = ip;

	int stack_depth = stack.size();
	stack.SetLimit();
	try
	{
		// load the code into memory
		unsigned char* cd = CompileText( text, strlen( text ) );
		if( !cd )
			throw DevaRuntimeException( "Unable to compile text." );
		code_blocks.push_back( cd );

		// fix-up the offsets into actual machine addresses
		FixupOffsets( cd );
		
		// run the code, ignoring breakpoints
		RunCode( cd, false );

		if( stack.size() != stack_depth )
			throw DevaStackException( "Evaluated code has compromised the stack. Program state is bad." );
	}
	catch( DevaRuntimeException & e )
	{
		stack.UnsetLimit();
		// ensure the file gets set back
		file = old_file;
		// restore the old code & ip
		code = orig_code;
		ip = orig_ip;

		// try to clean up the stack
		while( stack.size() > stack_depth )
		{
			stack.pop_back();
		}

		// propagate the exception
		throw;
	}

	stack.UnsetLimit();
	file = old_file;
}

void Executor::Exit( int val )
{
	exit( val );
}

void Executor::AddBuiltinModule( string mod, import_module_fcn fcn )
{
	builtin_module_names.insert( make_pair( mod, fcn ) );
}

// import a built-in module (calls the import_module_fcn for this module)
bool Executor::ImportBuiltinModule( string name )
{
	// find the import_module_fcn for this module
	map<string, import_module_fcn>::iterator it;
	it = builtin_module_names.find( name );
	if( it == builtin_module_names.end() )
		return false;
	// call it
	(it->second)( this );
	return true;
}

bool Executor::ImportBuiltinModuleFunctions( string mod, map<string, builtin_fcn> & fcns )
{
	// prevent importing the same module more than once
	vector< pair<string, ScopeTable*> >::iterator it;
	it = find_namespace( mod );
	// if we found the namespace
	if( it != namespaces.end() )
		return false;

	// create a new namespace
	ScopeTable* st = new ScopeTable( this );
	st->Push();
	namespaces.push_back( pair<string, ScopeTable*>( mod, st ) );
	// add an entry in the builtin modules map
	builtin_modules[mod] = vector<string>();

	// add the fcns to the namespace, the global scope
	// and to the builtin module map
	for( map<string, builtin_fcn>::iterator i = fcns.begin(); i != fcns.end(); ++i )
	{
		st->AddObject( new DevaObject( i->first, sym_unknown ) );
		global_scopes->AddObject( new DevaObject( i->first, sym_unknown ) );
		builtin_modules[mod].push_back( i->first );
	}
	// merge the fcn-name to fcn-ptr map with the execution engine's map
	builtin_module_fcns.insert( fcns.begin(), fcns.end() );

	return true;
}

void Executor::AddAllKnownBuiltinModules()
{
	AddBuiltinModule( string( "os" ), AddOsModule );
	AddBuiltinModule( string( "bit" ), AddBitModule );
	AddBuiltinModule( string( "math" ), AddMathModule );
	AddBuiltinModule( string( "_re" ), AddReModule );
}

// dump the stack trace to a output stream
void Executor::DumpTrace( ostream & os, bool show_all_scopes /*= false*/ )
{
	os << "Traceback (most recent first):" << endl;
	string fcn;
	int idx = -1;
	os << " file: " << file << ", line: " << line << ", in " << function << endl;
	for( vector<Frame>::reverse_iterator i = trace.rbegin(); i != trace.rend() - 1; ++i )
	{
		// print the file, function, line number
		if( show_all_scopes || (!show_all_scopes && (i->call_site != -1) ) )
		{
			// not a call site
			if( i->call_site == -1 )
				os << "  file: " << i->file << ", line: " << i->line << ", in " << i->function << endl;
			// call site
			else
				os << "  file: " << i->file << ", line: " << i->call_site << ", call to " << i->function << endl;
		}
		fcn = i->function;
		idx = i->scope_idx;
	}
}

// dump (at most the top ten items from) the data stack to stdout
void Executor::PrintDataStack()
{
	int num_items = stack.size() < 10 ? stack.size() : 10;
	for( int c = 0; c < num_items; ++c )
	{
		DevaObject o = stack[c];
		cout << "#" << c << ": " << o << endl;
	}
}

// start executing a file, stopping before the first instruction
void Executor::StartExecutingCode( unsigned char* cd )
{
	// make sure static data is reset
	args_on_stack = -1;

	// set the new code & ip
	code = cd;
	ip = (size_t)code;
}

// execute one line
// returns line number or -1 on halt
int Executor::StepOver()
{
	size_t return_address = 0;
	// keep executing instructions until we reach the next line_num op,
	// skipping calls
	while( true )
	{
		// get the next instruction in the byte code
		Instruction inst = NextInstr();

		// if this is a "top level" call, keep going until we get to the matching return
		if( inst.op == op_call && return_address == 0 )
		{
			return_address = ip;
		}

		// DoInstr returns false on 'halt' instruction
		if( !DoInstr( inst ) )
		{
			if( inst.op == op_halt )
				return -1;
			else
				return line;
		}

		if( ip == return_address )
		{
			return_address = 0;
		}

		if( !return_address && inst.op == op_line_num )
		{
			// ignore enter and leave instructions
			Opcode op = PeekInstr();
			if( op != op_enter && op != op_leave )
				return (int)inst.args[1].sz_val;
		}
	}
}

// execute one line or into one call
// returns line number or -1 on halt
int Executor::StepInto()
{
	// keep executing instructions until we reach a new line_num instruction
	while( true )
	{
		// get the next instruction in the byte code
		Instruction inst = NextInstr();

		// DoInstr returns false on 'halt' instruction
		if( !DoInstr( inst ) )
		{
			if( inst.op == op_halt )
				return -1;
			else
				return line;
		}

		if( inst.op == op_line_num )
		{
			// ignore enter and leave instructions
			Opcode op = PeekInstr();
			if( op != op_enter && op != op_leave )
				return (int)inst.args[1].sz_val;
		}
	}
}

// execute one instruction
// returns line number or -1 on halt
int Executor::StepInst( Instruction & inst )
{
	// get the next instruction in the byte code
	inst = NextInstr();

	// DoInstr returns false on 'halt' instruction or breakpoint
	if( !DoInstr( inst ) )
	{
		if( inst.op == op_halt )
			return -1;
		else
			return line;
	}

	if( inst.op == op_line_num )
		return (int)inst.args[1].sz_val;
	else
		return 0;
}

// execute (until breakpoint or exit)
// returns line number or -1 on halt
int Executor::Run( bool stop_at_breakpoints /*= true*/ )
{
	// keep executing instructions until we hit a breakpoint or halt
	while( true )
	{
		// get the next instruction in the byte code
		Instruction inst = NextInstr();

		// DoInstr returns false on 'halt' instruction or breakpoint
		if( !DoInstr( inst ) )
		{
			if( inst.op == op_halt )
				return -1;
			else if( stop_at_breakpoints )
				return line;
		}
	}
}

// add a breakpoint
void Executor::AddBreakpoint( string file, int line )
{
	breakpoints.push_back( make_pair(file, line) );
}
// enumerate breakpoints
const vector<pair<string, int> > & Executor::GetBreakpoints()
{
	return breakpoints;
}
// remove a breakpoint
void Executor::RemoveBreakpoint( int idx )
{
	breakpoints.erase( breakpoints.begin() + idx );
}
