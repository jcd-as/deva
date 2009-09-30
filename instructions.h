// instructions.h
// instruction steam for the deva language intermediate language & virtual machine
// created by jcs, september 14, 2009 

// TODO:
// * encode location (line number) into InstructionStream via "line_num" opcodes

#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include "opcodes.h"
#include "symbol.h"
#include "types.h"
#include <vector>
#include <cstring>

using namespace std;

// the basic piece of data stored on the data stack
struct DevaObject : public SymbolInfo
{
	// TODO: types for C function and UserCode (C "void*")block?
	union 
	{
		double num_val;
		char* str_val;
		bool bool_val;
		map<DevaObject, DevaObject>* map_val;
		vector<DevaObject>* vec_val;
		long func_offset;	// offset into instruction stream to function start
	};

	// name that the object (variable) is referred to with
	// (empty string for constants)
	string name;

	// copy constructor needed to ensure each object has a separate copy of data
	DevaObject( const DevaObject & o )
	{
		if( &o == this )
			return;
		name = o.name;
		type = o.type;
		is_const = o.is_const;
		is_argument = o.is_argument;
		switch( type )
		{
		case sym_number:
			num_val = o.num_val;
			break;
		case sym_string:
			str_val = new char[strlen( o.str_val ) + 1];
			strcpy( str_val, o.str_val );
			break;
		case sym_boolean:
			bool_val = o.bool_val;
			break;
		case sym_map:
			// TODO: verify this
			map_val = new map<DevaObject, DevaObject>();
			*map_val = *(o.map_val);
			break;
		case sym_vector:
			// TODO: verify this
			vec_val = new vector<DevaObject>();
			*vec_val = *(o.vec_val);
			break;
		case sym_function:
			// TODO: any better default value for fcn types?
			// TODO: need to create copy??
			func_offset = o.func_offset;
			break;
		case sym_function_call:
			// TODO: anything???
			break;
		case sym_null:
			// nothing to do, null has/needs no value
		case sym_unknown:
			// nothing to do, no known value/type
			break;
		default:
			// TODO: throw error
			break;
		}

	}
	// copy construct, but with a different name
	DevaObject( string nm, const DevaObject & o )
	{
		if( &o == this )
			return;
		name = nm;
		type = o.type;
		is_const = o.is_const;
		is_argument = o.is_argument;
		switch( type )
		{
		case sym_number:
			num_val = o.num_val;
			break;
		case sym_string:
			str_val = new char[strlen( o.str_val ) + 1];
			strcpy( str_val, o.str_val );
			break;
		case sym_boolean:
			bool_val = o.bool_val;
			break;
		case sym_map:
			// TODO: verify this
			map_val = new map<DevaObject, DevaObject>();
			*map_val = *(o.map_val);
			break;
		case sym_vector:
			// TODO: verify this
			vec_val = new vector<DevaObject>();
			*vec_val = *(o.vec_val);
			break;
		case sym_function:
			// TODO: any better default value for fcn types?
			// TODO: need to create copy??
			func_offset = o.func_offset;
			break;
		case sym_function_call:
			// TODO: anything???
			break;
		case sym_null:
			// nothing to do, null has/needs no value
		case sym_unknown:
			// nothing to do, no known value/type
			break;
		default:
			// TODO: throw error
			break;
		}

	}
	DevaObject( string nm ) : SymbolInfo( sym_end ), name( nm )
	{}
	DevaObject( string nm, double n ) : SymbolInfo( sym_number ), num_val( n ), name( nm )
	{}
	DevaObject( string nm, string s ) : SymbolInfo( sym_string ), name( nm )
	{
		// make a copy of the string passed to us, DON'T take ownership of it!
		str_val = new char[s.size() + 1];
		strcpy( str_val, s.c_str() );
	}
	DevaObject( string nm, bool b ) : SymbolInfo( sym_boolean ), bool_val( b ), name( nm )
	{}
	DevaObject( string nm, long offs ) : SymbolInfo( sym_function ), func_offset( offs ), name( nm )
	{}
	DevaObject( string nm, SymbolType t )
	{
		type = t;
		name = nm;
		is_const = false;
		is_argument = false;
		switch( t )
		{
		case sym_number:
			num_val = 0.0;
			break;
		case sym_string:
			str_val = NULL;
			break;
		case sym_boolean:
			bool_val = false;
			break;
		case sym_map:
			map_val = new map<DevaObject, DevaObject>();
			break;
		case sym_vector:
			vec_val = new vector<DevaObject>();
			break;
		case sym_function:
			func_offset = -1;
			break;
		case sym_function_call:
			// TODO: anything???
			break;
		case sym_null:
			// nothing to do, null has/needs no value
		case sym_unknown:
			// nothing to do, no known value/type
			break;
		default:
			// TODO: throw error, invalid
			break;
		}
	}
	~DevaObject()
	{
		if( type == sym_string )
			if( str_val ) delete [] str_val;
		else if( type == sym_map )
			if( map_val ) delete map_val;
		else if( type == sym_vector )
			if( vec_val ) delete vec_val;
	}
	DevaObject& operator = ( const DevaObject & o )
	{
		if( &o == this )
			return *this;
		name = o.name;
		type = o.type;
		is_const = o.is_const;
		is_argument = o.is_argument;
		switch( type )
		{
		case sym_number:
			num_val = o.num_val;
			break;
		case sym_string:
			str_val = new char[strlen( o.str_val ) + 1];
			strcpy( str_val, o.str_val );
			break;
		case sym_boolean:
			bool_val = o.bool_val;
			break;
		case sym_map:
			// TODO: verify this
			map_val = new map<DevaObject, DevaObject>();
			*map_val = *(o.map_val);
			break;
		case sym_vector:
			// TODO: verify this
			vec_val = new vector<DevaObject>();
			*vec_val = *(o.vec_val);
			break;
		case sym_function:
			// TODO: any better default value for fcn types?
			// TODO: need to create copy??
			func_offset = o.func_offset;
			break;
		case sym_function_call:
			// TODO: anything???
			break;
		case sym_null:
			// nothing to do, null has/needs no value
		case sym_unknown:
			// nothing to do, no known value/type
			break;
		default:
			// TODO: throw error
			break;
		}
		return *this;
	}

	// equivalent of destroying and re-creating
	// TODO: should this be done by creating new object and make this class
	// immutable?? how does this work with ref-counting??
	void ChangeType( SymbolType t )
	{
		if( type == sym_string )
			delete [] str_val;
		else if( type == sym_map )
			delete map_val;
		else if( type == sym_vector )
			delete vec_val;

		type = t;

		switch( t )
		{
		case sym_number:
			num_val = 0.0;
			break;
		case sym_string:
			// TODO: any better default value for string types?
			str_val = NULL;
			break;
		case sym_boolean:
			bool_val = false;
			break;
		case sym_map:
			map_val = new map<DevaObject, DevaObject>();
			break;
		case sym_vector:
			vec_val = new vector<DevaObject>();
			break;
		case sym_function:
			func_offset = -1;
			break;
		case sym_null:
			// nothing to do, null has/needs no value
		case sym_unknown:
			// nothing to do, no known value/type
			break;
		case sym_function_call:
		default:
			// TODO: throw error, can't change type
			break;
		}
	}
	// set the value from another object, without changing the name
	// fails (returns false) if this is a const object
	bool SetValue( const DevaObject & o )
	{
		if( is_const )
			return false;

		ChangeType( o.type );

		switch( type )
		{
		case sym_number:
			num_val = o.num_val;
			break;
		case sym_string:
			// TODO: any better default value for string types?
			// make a copy of the string passed to us, DON'T take ownership of it!
			str_val = new char[strlen( o.str_val ) + 1];
			strcpy( str_val, o.str_val );
			break;
		case sym_boolean:
			bool_val = o.bool_val;
			break;
		case sym_map:
			map_val = new map<DevaObject, DevaObject>();
			// TODO: copy the map
			break;
		case sym_vector:
			vec_val = new vector<DevaObject>();
			// TODO: copy the vector
			break;
		case sym_function:
			func_offset = o.func_offset;
			break;
		case sym_null:
			// nothing to do, null has/needs no value
		case sym_unknown:
			// nothing to do, variable with no known value
			break;
		case sym_function_call:
		default:
			// TODO: throw error, can't change
			break;
		}
	}
	// size of the object on *disk*
	long Size() const
	{
		// length of name (plus null terminator), plus 'type' byte
		long sz = name.length() + 2;
		switch( type )
		{
		case sym_number:
			return sz + sizeof( double );
		case sym_string:
			return sz + strlen( str_val ) + 1;
		case sym_boolean:
			return sz + sizeof( long );
		case sym_map:
			// TODO: implement
			return 0;
		case sym_vector:
			// TODO: implement
			return 0;
		case sym_function:
			return sz + sizeof( long );
		case sym_null:
			return sz + sizeof( long );
		case sym_unknown:
		case sym_function_call:
		default:
			return sz;
		}
	}
};
// operator to dump an DevaObject to an iostreams stream
ostream & operator << ( ostream & os, DevaObject & obj );
// operator to dump an Opcode to an iostreams stream
ostream & operator << ( ostream & os, Opcode & op );

struct Instruction
{
	Opcode op;
	vector<DevaObject> args;

	Instruction( Opcode o ) : op( o )
	{}
	Instruction( Opcode o, DevaObject ob1 ) : op( o )
	{
		args.push_back( ob1 );
	}
	Instruction( Opcode o, DevaObject ob1, DevaObject ob2 ) : op( o )
	{
		args.push_back( ob1 );
		args.push_back( ob2 );
	}
	long Size() const
	{
		// size of the args
		long offs = 0;
		for( vector<DevaObject>::const_iterator i = args.begin(); i != args.end(); ++i )
		{
			offs += i->Size();
		}
		// plus the size of the opcode AND the 'argument end' (255) byte
		return offs + 2;
	}
};

class InstructionStream
{
	// list of instructions
	vector<Instruction> instructions;

	// file offset
	long offset;

public:
	InstructionStream() : offset( 0 )
	{}
	void push( const Instruction & i )
	{
		instructions.push_back( i );
		offset += i.Size();
	}
	size_t size(){ return instructions.size(); }
	Instruction & operator[]( size_t idx ){ return instructions[idx]; }
	long Offset(){ return offset; }
};

// declare IL gen functions
void gen_IL_number( iter_t const & i, InstructionStream & is );
void gen_IL_string( iter_t const & i, InstructionStream & is );
void gen_IL_boolean( iter_t const & i, InstructionStream & is );
void gen_IL_null( iter_t const & i, InstructionStream & is );
void gen_IL_func( iter_t const & i, InstructionStream & is );

void pre_gen_IL_while_s( iter_t const & i, InstructionStream & is );
void gen_IL_while_s( iter_t const & i, InstructionStream & is );
void post_gen_IL_while_s( iter_t const & i, InstructionStream & is );

void pre_gen_IL_for_s( iter_t const & i, InstructionStream & is );
void gen_IL_for_s( iter_t const & i, InstructionStream & is );
void post_gen_IL_for_s( iter_t const & i, InstructionStream & is );

void pre_gen_IL_if_s( iter_t const & i, InstructionStream & is );
void gen_IL_if_s( iter_t const & i, InstructionStream & is );

void pre_gen_IL_else_s( iter_t const & i, InstructionStream & is );
void gen_IL_else_s( iter_t const & i, InstructionStream & is );

void gen_IL_identifier( iter_t const & i, InstructionStream & is );
void gen_IL_in_op( iter_t const & i, InstructionStream & is );
void gen_IL_map_op( iter_t const & i, InstructionStream & is );
void gen_IL_vec_op( iter_t const & i, InstructionStream & is );
void gen_IL_semicolon_op( iter_t const & i, InstructionStream & is );
void gen_IL_assignment_op( iter_t const & i, InstructionStream & is );
void gen_IL_logical_op( iter_t const & i, InstructionStream & is );
void gen_IL_relational_op( iter_t const & i, InstructionStream & is );
void gen_IL_mult_op( iter_t const & i, InstructionStream & is );
void gen_IL_add_op( iter_t const & i, InstructionStream & is );
void gen_IL_unary_op( iter_t const & i, InstructionStream & is );
void gen_IL_dot_op( iter_t const & i, InstructionStream & is );
void gen_IL_paren_op( iter_t const & i, InstructionStream & is );
void gen_IL_bracket_op( iter_t const & i, InstructionStream & is );

void pre_gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is );
void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is );

void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is );
void gen_IL_key_exp( iter_t const & i, InstructionStream & is );
void gen_IL_const_decl( iter_t const & i, InstructionStream & is );
void gen_IL_constant( iter_t const & i, InstructionStream & is );
void gen_IL_translation_unit( iter_t const & i, InstructionStream & is );

void pre_gen_IL_compound_statement( iter_t const & i, InstructionStream & is );
void gen_IL_compound_statement( iter_t const & i, InstructionStream & is );

void gen_IL_break_statement( iter_t const & i, InstructionStream & is );
void gen_IL_continue_statement( iter_t const & i, InstructionStream & is );
void gen_IL_return_statement( iter_t const & i, InstructionStream & is );


#endif // __INSTRUCTIONS_H__

