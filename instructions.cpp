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
			os << "num: '" << obj.name << "' = " << obj.num_val;
			break;
		case sym_string:
			os << "string: '" << obj.name << "' = " << obj.str_val;
			break;
		case sym_boolean:
			os << "boolean: '" << obj.name << "' = " << obj.bool_val;
			break;
		case sym_null:
			os << "null";
			break;
		case sym_map:
			// TODO: dump some map contents?
			os << "map: '" << obj.name << "' = ";
			break;
		case sym_vector:
			// TODO: dump some vector contents?
			os << "vector: '" << obj.name << "' = ";
			break;
		case sym_function:
			os << "function: '" << obj.name << "' = ";
			break;
		default:
			os << "unknown: '" << obj.name << "'";
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
	case op_load:
		os << "load";
		break;
	case op_store:
		os << "store";
		break;
	case op_new:
		os << "new";
		break;
	case op_new_map:
		os << "new_map:";
		break;
	case op_new_vec:
		os << "new_vec:";
		break;
	case op_vec_load:
		os << "vec_load";
		break;
	case op_vec_store:
		os << "vec_store";
		break;
	case op_map_load:
		os << "map_load";
		break;
	case op_map_store:
		os << "map_store";
		break;
	case op_jmp:			// unconditional jump to the address on top of the stack
		os << "jump";
		break;
	case op_jmpf:			// jump on false
		os << "jmpf";
		break;
	case op_eq:
		os << "eq";
		break;
	case op_neq:
		os << "neq";
		break;
	case op_lt:
		os << "lt";
		break;
	case op_lte:
		os << "lte";
		break;
	case op_gt:
		os << "gt";
		break;
	case op_gte:
		os << "gte";
		break;
	case op_or:
		os << "or";
		break;
	case op_and:
		os << "and";
		break;
	case op_neg:			// negate the top value
		os << "neg";
		break;
	case op_not:
		os << "not";
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
	return os;
}

void gen_IL_number( iter_t const & i, InstructionStream & is )
{
	// push the number onto the stack
	double n;
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	n = atof( s.c_str() );
	is.instructions.push_back( Instruction( op_push, DevaObject( "", n ) ) );
}

void gen_IL_string( iter_t const & i, InstructionStream & is )
{
	// push the string onto the stack
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	is.instructions.push_back( Instruction( op_push, DevaObject( "", s ) ) );
}

void gen_IL_boolean( iter_t const & i, InstructionStream & is )
{
	// push the boolean onto the stack
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "true" )
		is.instructions.push_back( Instruction( op_push, DevaObject( "", true ) ) );
	else
		is.instructions.push_back( Instruction( op_push, DevaObject( "", false ) ) );
}

void gen_IL_null( iter_t const & i, InstructionStream & is )
{
	// push the null onto the stack
	is.instructions.push_back( Instruction( op_push, DevaObject( "", sym_null ) ) );
}

// 'def' keyword
void gen_IL_func( iter_t const & i, InstructionStream & is )
{
	// TODO: enter a function into the scope symbol tbl
}

void gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
	// TODO
}

void gen_IL_for_s( iter_t const & i, InstructionStream & is )
{
	// TODO
}

void gen_IL_if_s( iter_t const & i, InstructionStream & is )
{
	// TODO
}

void gen_IL_else_s( iter_t const & i, InstructionStream & is )
{
	// TODO
}

void gen_IL_identifier( iter_t const & i, InstructionStream & is )
{
	// if no children, simple variable
	if(i->children.size() == 0 )
	{
		string name = strip_symbol( string( i->value.begin(), i->value.end() ) );
		is.instructions.push_back( Instruction( op_push , DevaObject( name, sym_unknown ) ) );
	}
	// TODO: if it has children it can be a function call, a map/vector lookup,
	// or part of a '.' operator look-up (syntactic sugar for a map lookup)
}

void gen_IL_in_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_map_op( iter_t const & i, InstructionStream & is )
{
	// new map
	is.instructions.push_back( Instruction( op_new_map ) );
}

void gen_IL_vec_op( iter_t const & i, InstructionStream & is )
{
	// new vector
	is.instructions.push_back( Instruction( op_new_vec ) );
}

void gen_IL_semicolon_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	is.instructions.push_back( Instruction( op_store ) );
}

void gen_IL_logical_op( iter_t const & i, InstructionStream & is )
{
	// '||' or '&&'
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "||" )
		is.instructions.push_back( Instruction( op_or ) );
	else if( s == "&&" )
		is.instructions.push_back( Instruction( op_and ) );
	// else?
}

void gen_IL_relational_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "==" )
		is.instructions.push_back( Instruction( op_eq ) );
	else if( s == "!=" )
		is.instructions.push_back( Instruction( op_neq ) );
	else if( s == "<" )
		is.instructions.push_back( Instruction( op_lt ) );
	else if( s == "<=" )
		is.instructions.push_back( Instruction( op_lte ) );
	else if( s == ">" )
		is.instructions.push_back( Instruction( op_gt ) );
	else if( s == ">=" )
		is.instructions.push_back( Instruction( op_gte ) );
	// else?
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
	// no op
}

void gen_IL_bracket_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is )
{
	// no op, parent handles
}

void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is )
{
	// no op, parent handles
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
	// no op
}

void pre_gen_IL_compound_statement( iter_t const & i, InstructionStream & is )
{
	// generate enter
	is.instructions.push_back( Instruction( op_enter ) );
}

void gen_IL_compound_statement( iter_t const & i, InstructionStream & is )
{
	// generate leave
	is.instructions.push_back( Instruction( op_leave ) );
}

void gen_IL_break_statement( iter_t const & i, InstructionStream & is )
{
	// TODO: generate jump
}

void gen_IL_continue_statement( iter_t const & i, InstructionStream & is )
{
	// TODO: generate jump
}

void gen_IL_return_statement( iter_t const & i, InstructionStream & is )
{
	// last child is always semi-colon
	// TODO: generate return/returnv
	if( i->children.size() > 1 )
		is.instructions.push_back( Instruction( op_returnv ) );
	else if( i->children.size() == 1 )
		is.instructions.push_back( Instruction( op_return ) );
	// else?
}


