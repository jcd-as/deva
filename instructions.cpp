// instructions.cpp
// IL instruction functions for the deva language
// created by jcs, september 14, 2009 

// TODO:
// * 'a.b = c' doesn't work properly
// * implement constants
// * implement short-circuiting for logical ops ('||', '&&' )
// * encode location (line number) into InstructionStream via "line_num" opcodes

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
			os << "function: '" << obj.name << "', offset = " << obj.func_offset;
			break;
		case sym_function_call:
			os << "function_call: '" << obj.name << "'";
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
	case op_push:			// push item onto top of stack
		os << "push";
		break;
	case op_load:
		os << "load";
		break;
	case op_store:
		os << "store";
		break;
	case op_defun:
		os << "defun";
		break;
	case op_defarg:
		os << "defarg";
		break;
	case op_dup:
		os << "dup";
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
	case op_swap:
		os << "swap";
		break;
	case op_map_store:
		os << "map_store";
		break;
	case op_jmp:			// unconditional jump to the address on top of the stack
		os << "jmp";
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
	case op_break:			// break out of loop, respecting scope (enter/leave)
		os << "break";
		break;
	case op_enter:
		os << "enter";
		break;
	case op_leave:
		os << "leave";
		break;
	case op_nop:			// no op
		os << "nop";
		break;
	case op_halt:
		os << "halt";
		break;
	case op_illegal:
		os << "ILLEGAL OPCODE. COMPILER ERROR";
		break;
	default:
		os << "INVALID OPCODE";
		break;
	}
	return os;
}


// IL gen functions:
void gen_IL_number( iter_t const & i, InstructionStream & is )
{
	// push the number onto the stack
	double n;
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	n = atof( s.c_str() );
	is.push( Instruction( op_push, DevaObject( "", n ) ) );
}

void gen_IL_string( iter_t const & i, InstructionStream & is )
{
	// push the string onto the stack
	string s = strip_quotes( strip_symbol( string( i->value.begin(), i->value.end() ) ) );
	is.push( Instruction( op_push, DevaObject( "", s ) ) );
}

void gen_IL_boolean( iter_t const & i, InstructionStream & is )
{
	// push the boolean onto the stack
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "true" )
		is.push( Instruction( op_push, DevaObject( "", true ) ) );
	else
		is.push( Instruction( op_push, DevaObject( "", false ) ) );
}

void gen_IL_null( iter_t const & i, InstructionStream & is )
{
	// push the null onto the stack
	is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
}

// 'def' keyword
void gen_IL_func( iter_t const & i, InstructionStream & is )
{
	// no-op
}

static vector<int> loop_stack;

void pre_gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
	// general format of while statement
	// while( a < 10 ){ do_stuff(); }
	// AST:
	// while_s
	// 		- relational_op <
	// 			- identifier a
	// 			- number 10
	// 		- compound_statement
	// 			- call do_stuff
	// IL:
	// start:
	// (relational_op IL)
	// jmpf done 
	// call do_stuff
	// jmp start
	// done:
	// push the current location in the instruction stream onto the 'loop stack'
	loop_stack.push_back( is.Offset() );
}

static vector<int> while_jmpf_stack;
void gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
	// save the location for back-patching
	while_jmpf_stack.push_back( is.size() );
	// generate a place-holder op for the condition's jmpf
	is.push( Instruction( op_jmpf, DevaObject( "", -1.0 ) ) );
}

void post_gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
	// add the jump-to-start
	int loop_loc = loop_stack.back();
	loop_stack.pop_back();
	is.push( Instruction( op_jmp, DevaObject( "", (double)loop_loc ) ) );
	// back-patch the jmpf
	// pop the last 'loop' location off the 'loop stack'
	int jmpf_loc = while_jmpf_stack.back();
	while_jmpf_stack.pop_back();
	// write the current *file* offset, not instruction stream!
	is[jmpf_loc] = Instruction( op_jmpf, DevaObject( "", (double)is.Offset() ) );
}

void pre_gen_IL_for_s( iter_t const & i, InstructionStream & is )
{
	// general format of 'for' statement
	// for( item in table ){ do_stuff(); }
	// AST:
	// for_s
	//		- identifier 'item' (variable)
	// 		- in_op
	// 			- identifier 'table' (variable naming a vector/map)
	// 		- statement|compound_statement
	// 			- call do_stuff
	// IL:
	// push 'table'
	// call 'length'
	// push 0 				<== 'index' start value
	// start:
	//
	// dup 2
	// dup 2				<== dups get the top two items (index & length) duplicated
	// neq					<== and neq removes the first pair
	// jmpf done 			<== if index equals length, jump to 'done'
	//
	// push 'table'
	// dup 2				<== copies 'index' onto the top of the stack
	// vec_load				<== puts next item on the stack, null if no more items
	// push 'item'			<== stack+0 now the value, stack+1 the index
	// swap					<== swap them for the store op
	// store				<== store value into 'item'
	//
	// <statement/compound_statement IL>
	//
	// jmp start
	// done:
	// pop					<== pop index

	// if there are 3 children, this is a vector look-up ( 'for( i in t )' )
	// if there are 4 children, this is a map look-up	( 'for( key,val in t )' )
	bool is_map = false;
	int in_op_index = 1;
	int num_children = i->children.size();
	if( i->children.size() > 3 )
	{
		is_map = true;
		in_op_index = 2;
	}
	// get the name of the vector/map item
	if( i->children[0].value.id() != identifier_id )
		throw DevaICE( "Invalid loop variable in 'for' loop." );
	string item_name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
	string item_value_name;
	if( is_map )
		item_value_name = strip_symbol( string( i->children[1].value.begin(), i->children[1].value.end() ) );
	// get the name of the vector/map
	if( i->children[in_op_index].value.id() != in_op_id || i->children[in_op_index].children.size() != 1 )
		throw DevaICE( "Invalid 'in' statement in 'for' loop." );
	string table_name = strip_symbol( string( i->children[in_op_index].children[0].value.begin(), i->children[in_op_index].children[0].value.end() ) );

	// push the return address for the call to 'length'
	is.push( Instruction( op_push, DevaObject( "", is.Offset() ) ) );
	// push the vector/map
	is.push( Instruction( op_push, DevaObject( table_name, sym_unknown ) ) );
	// call 'length' builtin
	is.push( Instruction( op_call, DevaObject( "length", sym_function_call ) ) );
	// push 0
	is.push( Instruction( op_push, DevaObject( "", 0.0 ) ) );
	// save the offset for the loop stack (of the 'start', for back-patching)
	loop_stack.push_back( is.Offset() );
	// dup 1
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// dup 1
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// neq
	is.push( Instruction( op_neq ) );
	// save the instruction location for back-patching
	while_jmpf_stack.push_back( is.size() );
	// generate a place-holder op for the jmpf (which ends looping)
	is.push( Instruction( op_jmpf, DevaObject( "", -1.0 ) ) );
	// push 'table'
	is.push( Instruction( op_push, DevaObject( table_name, sym_unknown ) ) );
	// dup 1
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	if( is_map )
	{
		// vec_load, with a 'true' arg to indicate that maps should treat the value
		// as an index (not a key)
		is.push( Instruction( op_vec_load, DevaObject( "", true ) ) );
		// push the 'item' (key)
		is.push( Instruction( op_push, DevaObject( item_name, sym_unknown ) ) );
		// swap the top two items for the store op
		is.push( Instruction( op_swap ) );
		// store
		is.push( Instruction( op_store ) );
		// push the 'value'
		is.push( Instruction( op_push, DevaObject( item_value_name, sym_unknown ) ) );
		// swap the top two items for the store op
		is.push( Instruction( op_swap ) );
		// store
		is.push( Instruction( op_store ) );
	}
	else
	{
		// vec_load, with a 'true' arg to indicate that maps should treat the value
		// as an index (not a key)
		is.push( Instruction( op_vec_load ) );
		// push the 'item'
		is.push( Instruction( op_push, DevaObject( item_name, sym_unknown ) ) );
		// swap the top two items for the store op
		is.push( Instruction( op_swap ) );
		// store
		is.push( Instruction( op_store ) );
	}
}

void gen_IL_for_s( iter_t const & i, InstructionStream & is )
{
	// push 1
	is.push( Instruction( op_push, DevaObject( "", 1.0 ) ) );
	// add
	is.push( Instruction( op_add ) );
	// add the jump-to-start
	int loop_loc = loop_stack.back();
	loop_stack.pop_back();
	is.push( Instruction( op_jmp, DevaObject( "", (double)loop_loc ) ) );

	// back-patch the jmpf
	// pop the last 'loop' location off the 'loop stack'
	int jmpf_loc = while_jmpf_stack.back();
	while_jmpf_stack.pop_back();
	// write the current *file* offset, not instruction stream!
	is[jmpf_loc] = Instruction( op_jmpf, DevaObject( "", (double)is.Offset() ) );

	// pop to remove 'index' from the stack
	is.push( Instruction( op_pop ) );
}

static vector<int> if_stack;

void pre_gen_IL_if_s( iter_t const & i, InstructionStream & is )
{
	// general format of an if/else, AST, and IL:
	// if( x ) y(); else z(); =>
	// AST:
	// -if
	// 		-x
	// 		-y()
	// 		-else
	// 			-z()
	// IL:
	// push x
	// jumpf else_label
	// y()
	// jump end_else_label
	// else_label: 
	// z()
	// end_else_label:

	// push the current location in the instruction stream onto the 'if stack'
	if_stack.push_back( is.size() );
	// generate a jump placeholder 
	// for a jump *over* the child (statement|compound_statement)
	is.push( Instruction( op_jmpf, DevaObject( "", -1.0 ) ) );
}

void gen_IL_if_s( iter_t const & i, InstructionStream & is )
{
	// generate jump destination, back-patching the placeholder from the 'if'
	// with the correct (current) offset in the instruction stream
	// pop the last 'if' location off the 'if stack'
	int if_loc = if_stack.back();
	if_stack.pop_back();
	// back-patch the jumpf instruction
	// write the current *file* offset, not instruction stream!
	is[if_loc] = Instruction( op_jmpf, DevaObject( "", (double)is.Offset() ) );
}

static vector<int> else_stack;
void pre_gen_IL_else_s( iter_t const & i, InstructionStream & is )
{
	// push the current location in the instruction stream onto the 'else' stack
	// write the current *file* offset, not instruction stream!
	else_stack.push_back( is.size() );
	// generate the jump placeholder
	is.push( Instruction( op_jmp, DevaObject( "", -1.0 ) ) );
}

void gen_IL_else_s( iter_t const & i, InstructionStream & is )
{
	// generate the jump destination, back-patching the placeholder from
	// 'pre-gen' with the correct (current) offset in the instruction stream
	// pop the last 'else' location off the 'else stack'
	int else_loc = else_stack.back();
	else_stack.pop_back();
	// back-patch the jumpf instruction
	// write the current *file* offset, not instruction stream!
	is[else_loc] = Instruction( op_jmp, DevaObject( "", (double)is.Offset() ) );
}

// stack of fcn returns for back-patching return addresses in
static vector<int> fcn_call_stack;
void gen_IL_identifier( iter_t const & i, InstructionStream & is, iter_t const & parent, bool get_fcn_from_stack )
{
	// simple variable and map/vector lookups handled by caller,
	// only need to handle function calls here
	if( i->children[0].value.id() == arg_list_exp_id )
	{
		// TODO: validate
		// this may be incorrect when reference-counting semantics/variable
		// tracking is added...
		string name = strip_symbol( string( i->value.begin(), i->value.end() ) );
		if( get_fcn_from_stack )
			// add the call instruction, passing a null arg to indicate it needs
			// to pull the function off the stack
			is.push( Instruction( op_call ) );
		else
			// add the call instruction with the name of the fcn to call
			is.push( Instruction( op_call, DevaObject( name, sym_function_call ) ) );


		// back-patch the return address push op
		int ret_addr_loc = fcn_call_stack.back();
		fcn_call_stack.pop_back();
		// write the current *file* offset, not instruction stream!
		is[ret_addr_loc] = Instruction( op_push, DevaObject( "", (long)is.Offset() ) );

		// if the parent is the translation unit, a loop, or a compound_statement, the return
		// value is not being used, emit a pop instruction to discard it
		if( parent->value.id() == translation_unit_id 
			|| parent->value.id() == while_s_id
			|| parent->value.id() == for_s_id
			|| parent->value.id() == compound_statement_id )
		{
			is.push( Instruction( op_pop ) );
		}
	}
}

void gen_IL_in_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_map_op( iter_t const & i, InstructionStream & is )
{
	// new map
	is.push( Instruction( op_new_map ) );
}

void gen_IL_vec_op( iter_t const & i, InstructionStream & is )
{
	// new vector
	is.push( Instruction( op_new_vec ) );
}

void gen_IL_semicolon_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	// unless the rhs (child #2 is a map/vector op, in which case a 'new
	// vec/map' instruction will be generated instead
	if( i->children[1].value.id() != vec_op_id && i->children[1].value.id() != map_op_id )
		is.push( Instruction( op_store ) );
}

void gen_IL_logical_op( iter_t const & i, InstructionStream & is )
{
	// '||' or '&&'
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "||" )
		is.push( Instruction( op_or ) );
	else if( s == "&&" )
		is.push( Instruction( op_and ) );
	// else?
}

void gen_IL_relational_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "==" )
		is.push( Instruction( op_eq ) );
	else if( s == "!=" )
		is.push( Instruction( op_neq ) );
	else if( s == "<" )
		is.push( Instruction( op_lt ) );
	else if( s == "<=" )
		is.push( Instruction( op_lte ) );
	else if( s == ">" )
		is.push( Instruction( op_gt ) );
	else if( s == ">=" )
		is.push( Instruction( op_gte ) );
	// else?
}

void gen_IL_mult_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	// multiply
	if( s == "*" )
		is.push( Instruction( op_mul ) );
	// divide
	else if( s == "/" )
		is.push( Instruction( op_div ) );
	// modulus
	else if( s == "%" )
		is.push( Instruction( op_mod ) );
	// else?
}

void gen_IL_add_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	// add 
	if( s == "+" )
		is.push( Instruction( op_add ) );
	// subract
	else if( s == "-" )
		is.push( Instruction( op_sub ) );
	// else?
}

void gen_IL_unary_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	// negate operator
	if( s == "-" )
		is.push( Instruction( op_neg ) );
	// not operator
	else if( s == "!" )
		is.push( Instruction( op_not ) );
}

void gen_IL_dot_op( iter_t const & i, InstructionStream & is )
{
	// TODO: dot operator also has to do the parent check, as in 
	// identifier above, so that 'foo.bar()' generates a pop instruction too
	//
	// a.b = a["b"]  <== dot op is syntactic sugar for map lookup
	// first arg = lhs
	// second arg = rhs
	is.push( Instruction( op_vec_load ) );
}

void gen_IL_paren_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_bracket_op( iter_t const & i, InstructionStream & is )
{
	// no op
}

void pre_gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is )
{
	// push the (offset for the) return address
	// save the location for back-patching the proper return address (address
	// *after* the call is made)
	fcn_call_stack.push_back( is.size() );
	// (prior to fcn arguments being pushed)
	is.push( Instruction( op_push, DevaObject( "", (long)-1 ) ) );
}

void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is )
{
	// for each arg that is an identifier,
	int num_children = i->children.size();
	for( int j = 0; j < num_children; ++j )
	{
		if( i->children[j].value.id() == identifier_id )
		{
			string name = strip_symbol( string( i->children[j].value.begin(), i->children[j].value.end() ) );
			// create an argument 
			is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ) ) );
		}
	}
}

void gen_IL_key_exp( iter_t const & i, InstructionStream & is )
{
	// only called to generate vec loads, not stores (see assignment op handling
	// in compile.cpp, generate_IL_for_node() for vec store code gen)
	// TODO: consolidate map/vec load into a single 'key lookup' op
	// (no way to determine if it's a string or a number until run-time)
	// a[b] <= a is parent, b is child[1] ('[' is child[0] and ']' is child[2])
	is.push( Instruction( op_vec_load ) );
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
	is.push( Instruction( op_enter ) );
}

void gen_IL_compound_statement( iter_t const & i, InstructionStream & is )
{
	// generate leave
	is.push( Instruction( op_leave ) );
}

void gen_IL_break_statement( iter_t const & i, InstructionStream & is )
{
	// generate break op with a reference to the start of the loop
	int loop_loc = loop_stack.back();
	is.push( Instruction( op_break, DevaObject( "", (double)loop_loc ) ) );
}

void gen_IL_continue_statement( iter_t const & i, InstructionStream & is )
{
	// generate jump to beginning of loop
	int loop_loc = loop_stack.back();
	// write the current *file* offset, not instruction stream!
	is.push( Instruction( op_jmp, DevaObject( "", (double)loop_loc ) ) );
}

void gen_IL_return_statement( iter_t const & i, InstructionStream & is )
{
	// last child is always semi-colon
	// generate return
	if( i->children.size() > 1 )
		is.push( Instruction( op_return ) );
	else if( i->children.size() == 1 )
	{
		is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
		is.push( Instruction( op_return ) );
	}
	// else?
}


