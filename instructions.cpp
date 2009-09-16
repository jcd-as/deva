// instuctions.cpp
// IL instruction functions for the deva language
// created by jcs, september 14, 2009 

// TODO:
// * 

#include "instructions.h"
#include "parser_ids.h"
#include "scope.h"
#include "compile.h"
#include <iostream>
#include <cstdlib>
#include <cstring>


// the global scopes table
extern Scopes scopes;

// helpers:
// operator to dump an DevaObject to an iostreams stream
ostream & operator << ( ostream & os, DevaObject & obj )
{
	switch( obj.Type() )
	{
		case sym_number:
			os << "num: " << obj.num_val;
			break;
		case sym_string:
			os << "string: " << obj.str_val;
			break;
		case sym_boolean:
			os << "boolean: " << obj.bool_val;
			break;
		case sym_null:
			os << "null";
			break;
		case sym_map:
			// TODO: dump some map contents?
			os << "map";
			break;
		case sym_vector:
			// TODO: dump some vector contents?
			os << "vector";
			break;
		case sym_function:
			os << "function";
			break;
		default:
			os << "UNKNOWN";
	}
	return os;
}

// operator to dump an Opcode to an iostreams stream
ostream & operator << ( ostream & os, Opcode & op )
{
	switch( op )
	{
	case op_pop:			// pop top item off stack
		os << "pop";
		break;
//	case op_peek:			// look at top item on stack without removing it
//		os << "peek";
//		break;
	case op_push:			// push item onto top of stack
		os << "push";
		break;
	case op_jmp:			// unconditional jump to the address on top of the stack
		os << "jump";
		break;
	case op_jmpf:			// jump on false
		os << "jmpf";
		break;
	case op_cmp:			// compare top two values on stack
		os << "cmp";
		break;
	case op_neg:			// negate the top value
		os << "neg";
		break;
	case op_add:			// add top two values on stack
		os << "add";
		break;
	case op_sub:			// subtract top two values on stack
		os << "sub";
		break;
	case op_mul:			// multiply top two values on stack
		os << "mul";
		break;
	case op_div:			// divide top two values on stack
		os << "div";
		break;
	case op_mod:			// modulus top two values on stack
		os << "mod";
		break;
	case op_output:			// dump top of stack to stdout
		os << "output";
		break;
	case op_call:			// call a function. arguments on stack
		os << "call";
		break;
	case op_return:			// pop the return address and unconditionally jump to it
		os << "return";
		break;
	case op_returnv:		// as return, but stack holds return value and then (at top) return address
		os << "returnv";
		break;
	case op_nop:			// no op
		os << "nop";
		break;
	}
}

void gen_IL_number( iter_t const & i, InstructionStream & is )
{
	// push the number onto the stack
	double n;
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	n = atof( s.c_str() );
	is.instructions.push_back( Instruction( op_push, DevaObject( n ) ) );
}

void gen_IL_string( iter_t const & i, InstructionStream & is )
{
	// push the string onto the stack
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	char* cstr = new char[s.size() + 1];
	strcpy( cstr, s.c_str() );
	is.instructions.push_back( Instruction( op_push, DevaObject( cstr ) ) );
}

void gen_IL_boolean( iter_t const & i, InstructionStream & is )
{
	// push the boolean onto the stack
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "true" )
		is.instructions.push_back( Instruction( op_push, DevaObject( true ) ) );
	else
		is.instructions.push_back( Instruction( op_push, DevaObject( false ) ) );
}

void gen_IL_null( iter_t const & i, InstructionStream & is )
{
	// push the null onto the stack
	is.instructions.push_back( Instruction( op_push, DevaObject( sym_null ) ) );
}

// 'def' keyword
void gen_IL_func( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_for_s( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_if_s( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_else_s( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_identifier( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_in_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_map_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_vec_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_semicolon_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store top of stack (child 2) into variable (child 1)
}

void gen_IL_logical_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_relational_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_mult_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	// multiply
	if( s == "*" )
		is.instructions.push_back( Instruction( op_mul ) );
	// divide
	else if( s == "/" )
		is.instructions.push_back( Instruction( op_div ) );
	// modulus
	else if( s == "%" )
		is.instructions.push_back( Instruction( op_mod ) );
	// else?
}

void gen_IL_add_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	// add 
	if( s == "+" )
		is.instructions.push_back( Instruction( op_add ) );
	// subract
	else if( s == "-" )
		is.instructions.push_back( Instruction( op_sub ) );
	// else?
}

void gen_IL_unary_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	// negate operator
	if( s == "-" )
		is.instructions.push_back( Instruction( op_neg ) );
	// not operator
	else if( s == "!" )
		is.instructions.push_back( Instruction( op_not ) );
}

void gen_IL_dot_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_paren_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_bracket_op( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_key_exp( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_const_decl( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_constant( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_translation_unit( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_compound_statement( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_break_statement( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_continue_statement( iter_t const & i, InstructionStream & is )
{
}

void gen_IL_return_statement( iter_t const & i, InstructionStream & is )
{
}


