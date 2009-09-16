// instructions.h
// instruction steam for the deva language intermediate language & virtual machine
// created by jcs, september 14, 2009 

// TODO:
// * 

#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include "opcodes.h"
#include "symbol.h"
#include "types.h"
#include <vector>

using namespace std;

struct Function
{
	int offset;
	// TODO: info on arguments, if it returns a value...?
};

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
		Function* func_val;
	};

	DevaObject( double n ) : SymbolInfo( sym_number ), num_val( n )
	{}
	DevaObject( char* s ) : SymbolInfo( sym_string ), str_val( s )
	{}
	DevaObject( bool b ) : SymbolInfo( sym_boolean ), bool_val( b )
	{}
	DevaObject( Function* f ) : SymbolInfo( sym_function ), func_val( f )
	{}
	DevaObject( SymbolType t )
	{
		type = t;
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
			// TODO: any better default value for fcn types?
			func_val = NULL;
			break;
		}
	}
	~DevaObject()
	{
		if( type == sym_string )
		{}
//			if( str_val ) delete [] str_val;
		else if( type == sym_map )
			if( map_val ) delete map_val;
		else if( type == sym_vector )
			if( vec_val ) delete vec_val;
		else if( type == sym_function )
			if( func_val ) delete func_val;
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
		else if( type == sym_function )
			delete func_val;

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
			// TODO: any better default value for fcn types?
			func_val = NULL;
			break;
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
};

struct InstructionStream
{
	// list of instructions
	vector<Instruction> instructions;
};

// declare IL gen functions
void gen_IL_number( iter_t const & i, InstructionStream & is );
void gen_IL_string( iter_t const & i, InstructionStream & is );
void gen_IL_boolean( iter_t const & i, InstructionStream & is );
void gen_IL_null( iter_t const & i, InstructionStream & is );
void gen_IL_func( iter_t const & i, InstructionStream & is );
void gen_IL_while_s( iter_t const & i, InstructionStream & is );
void gen_IL_for_s( iter_t const & i, InstructionStream & is );
void gen_IL_if_s( iter_t const & i, InstructionStream & is );
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
void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is );
void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is );
void gen_IL_key_exp( iter_t const & i, InstructionStream & is );
void gen_IL_const_decl( iter_t const & i, InstructionStream & is );
void gen_IL_constant( iter_t const & i, InstructionStream & is );
void gen_IL_translation_unit( iter_t const & i, InstructionStream & is );
void gen_IL_compound_statement( iter_t const & i, InstructionStream & is );
void gen_IL_break_statement( iter_t const & i, InstructionStream & is );
void gen_IL_continue_statement( iter_t const & i, InstructionStream & is );
void gen_IL_return_statement( iter_t const & i, InstructionStream & is );


#endif // __INSTRUCTIONS_H__

