// instructions.h
// instruction steam for the deva language intermediate language & virtual machine
// created by jcs, september 14, 2009 

// TODO:
// * 

#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include "dobject.h"

using namespace std;

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
	Instruction( Opcode o, vector<DevaObject> objects ) : op( o )
	{
		for( vector<DevaObject>::iterator i = objects.begin(); i != objects.end(); ++i )
		{
			args.push_back( *i );
		}
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
	size_t offset;

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
	size_t Offset(){ return offset; }
};


// helper fcn to generate op_line_num instructions
void generate_line_num( iter_t const & i, InstructionStream & is );

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

void pre_gen_IL_if_s( iter_t const & i, InstructionStream & is );
void gen_IL_if_s( iter_t const & i, InstructionStream & is );

void pre_gen_IL_else_s( iter_t const & i, InstructionStream & is );
void gen_IL_else_s( iter_t const & i, InstructionStream & is );

void gen_IL_identifier( iter_t const & i, InstructionStream & is, iter_t const & parent, bool get_fcn_from_stack );

void gen_IL_import( iter_t const & i, InstructionStream & is );
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
void gen_IL_dot_op( iter_t const & i, InstructionStream & is, bool is_method_call = false );
void gen_IL_paren_op( iter_t const & i, InstructionStream & is );
void gen_IL_bracket_op( iter_t const & i, InstructionStream & is );

void pre_gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is );
void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is );

void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is );

void gen_IL_key_exp( iter_t const & i, InstructionStream & is );

void gen_IL_const_decl( iter_t const & i, InstructionStream & is );
void gen_IL_constant( iter_t const & i, InstructionStream & is );
void gen_IL_translation_unit( iter_t const & i, InstructionStream & is );

void pre_gen_IL_compound_statement( iter_t const & i, InstructionStream & is, iter_t const & parent );
void gen_IL_compound_statement( iter_t const & i, InstructionStream & is, iter_t const & parent );

void gen_IL_break_statement( iter_t const & i, InstructionStream & is );
void gen_IL_continue_statement( iter_t const & i, InstructionStream & is );
void gen_IL_return_statement( iter_t const & i, InstructionStream & is );


#endif // __INSTRUCTIONS_H__

