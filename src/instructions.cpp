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

// instructions.cpp
// IL instruction functions for the deva language
// created by jcs, september 14, 2009 

// TODO:
// * implement short-circuiting for logical ops ('||', '&&' )

#include "instructions.h"
#include "parser_ids.h"
#include "scope.h"
#include "compile.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>


// the global scopes table
extern Scopes scopes;

// helpers:
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
	case op_tbl_load:
		os << "tbl_load";
		break;
	case op_tbl_store:
		os << "tbl_store";
		break;
	case op_swap:
		os << "swap";
		break;
	case op_line_num:
		os << "line_num";
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
	case op_import:
		os << "import";
		break;
	case op_new_class:
		os << "new_class";
		break;
	case op_new_instance:
		os << "new_instance";
		break;
	case op_endf:
		os << "endf";
		break;
	case op_roll:
		os << "roll";
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

// helper to generate line number ops, IF debugging info is turned on
// (ops indicate a change in the current line num)
bool debug_info_on;
void generate_line_num( iter_t const & i, InstructionStream & is, bool force /*= false*/  )
{
	static int line = 1;
	static string file = "";

	if( !force && !debug_info_on )
		return;

	// get the node info
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	// if the file or line number has changed since the last call, generate a
	// line num op
	if( line != ni.line || file != ni.file )
	{
		line = ni.line;
		file = ni.file;
		is.push( Instruction( op_line_num, DevaObject( "", ni.file ), DevaObject( "", (size_t)ni.line, false ) ) );
	}
}


// IL gen functions:
////////////////////////////////////////////////////////

// helper to parse numbers
double parse_number( string s )
{
	double n;
	// hex? octal? binary?
	if( s[0] == '0' && s.length() > 1 )
	{
		char* end;
		if( s[1] == 'x' )
		{
			long l = strtol( s.c_str()+2, &end, 16 );
			n = l;
			return n;
		}
		else if( s[1] == 'o' )
		{
			long l = strtol( s.c_str()+2, &end, 8 );
			n = l;
			return n;
		}
		else if( s[1] == 'b' )
		{
			long l = strtol( s.c_str()+2, &end, 2 );
			n = l;
			return n;
		}
		else
		{
			// or real?
			n = atof( s.c_str() );
			return n;
		}
	}
	// or real?
	n = atof( s.c_str() );
	return n;
}

void gen_IL_number( iter_t const & i, InstructionStream & is )
{
	// push the number onto the stack
	double n;
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	generate_line_num( i, is );
	n = parse_number( s );
	is.push( Instruction( op_push, DevaObject( "", n ) ) );
}

void gen_IL_string( iter_t const & i, InstructionStream & is )
{
	// push the string onto the stack
	// "un-escape" the string
	string s = unescape( strip_quotes( strip_symbol( string( i->value.begin(), i->value.end() ) ) ) );
	generate_line_num( i, is );
	is.push( Instruction( op_push, DevaObject( "", s ) ) );
}

void gen_IL_boolean( iter_t const & i, InstructionStream & is )
{
	// push the boolean onto the stack
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "true" )
	{
		generate_line_num( i, is );
		is.push( Instruction( op_push, DevaObject( "", true ) ) );
	}
	else
	{
		generate_line_num( i, is );
		is.push( Instruction( op_push, DevaObject( "", false ) ) );
	}
}

void gen_IL_null( iter_t const & i, InstructionStream & is )
{
	// push the null onto the stack
	generate_line_num( i, is );
	is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
}

// 'def' keyword
void gen_IL_func( iter_t const & i, InstructionStream & is )
{
	// no-op
}

static vector<size_t> loop_stack;
static vector<int> loop_scope_stack;

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
	loop_scope_stack.push_back( 0 );
}

// stack for tracking the end of loop addresses, for back-patching
static vector<size_t> while_jmpf_stack;
// stack of (list of) break statement addresses, for back-patching
static vector< vector<size_t>* > loop_break_locations;
void gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
	generate_line_num( i, is );

	// save the location for back-patching
	while_jmpf_stack.push_back( is.size() );
	// start a new loop context for back-patching 'break' statements
	loop_break_locations.push_back( new vector<size_t>() );

	// generate a place-holder op for the condition's jmpf
	is.push( Instruction( op_jmpf, DevaObject( "", (size_t)-1, true ) ) );
}

void post_gen_IL_while_s( iter_t const & i, InstructionStream & is )
{
	// add the jump-to-start
	size_t loop_loc = loop_stack.back();
	loop_stack.pop_back();

	int loop_scopes = loop_scope_stack.back();
	if( loop_scopes != 0 )
		throw DevaICE( "unclosed scopes in while loop" );
	loop_scope_stack.pop_back();

	generate_line_num( i, is );
	is.push( Instruction( op_jmp, DevaObject( "", loop_loc, true ) ) );

	// back-patch the jmpf
	// pop the last 'loop' location off the 'loop stack'
	int jmpf_loc = while_jmpf_stack.back();
	while_jmpf_stack.pop_back();
	// write the current *file* offset, not instruction stream!
	is[jmpf_loc] = Instruction( op_jmpf, DevaObject( "", is.Offset(), true ) );

	// back-patch any 'break' statements
	// pop the loop from the loop stack
	vector<size_t>* breaks = loop_break_locations.back();
	loop_break_locations.pop_back();
	for( vector<size_t>::iterator it = breaks->begin(); it != breaks->end(); ++it )
	{
		// back-patch the instruction at the break to point to the current
		// location in the instruction stream (end of this loop)
		is[*it] = Instruction( op_jmp, DevaObject( "", is.Offset(), true ) );
	}
	delete breaks;
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

	generate_line_num( i, is );

	// get the vector/map on the stack
	generate_IL_for_node( i->children[in_op_index].children.begin(), is, i->children.begin() + in_op_index, 0 );
	// push a place-holder for the return address for the call to 'rewind'
	size_t loc = is.size();
	is.push( Instruction( op_push, DevaObject( "", (size_t)-1, true ) ) );

	// 'copy' the vector/map to the top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );

	// use the 'enumerable interface' (rewind & next) 
	// call 'rewind'
	is.push( Instruction( op_push, DevaObject( "", string( "rewind" ) ) ) );
	is.push( Instruction( op_tbl_load, DevaObject( "", false ) ) );
	is.push( Instruction( op_call, DevaObject( "", (size_t)0, false ) ) );
	// back-patch the return address for the call to 'rewind'
	is[loc] = Instruction( op_push, DevaObject( "", is.Offset(), true ) );

	is.push( Instruction( op_pop ) );

	// create the vector's 'iteration' object
	is.push( Instruction( op_push, DevaObject( item_name, sym_unknown ) ) );
	is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
	is.push( Instruction( op_store, DevaObject( "", true ) ) );

	// if this is a map, create the map's 'iteration value' object too
	if( is_map )
	{
		is.push( Instruction( op_push, DevaObject( item_value_name, sym_unknown ) ) );
		is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
		is.push( Instruction( op_store, DevaObject( "", true ) ) );
	}

	// save the offset for the loop stack (of the 'start', for back-patching)
	loop_stack.push_back( is.Offset() );
	loop_scope_stack.push_back( 0 );

	// call 'next' on the vector and store its value into a vector
	loc = is.size();
	is.push( Instruction( op_push, DevaObject( "", (size_t)-1, true ) ) );

	// 'copy' the vector/map to the top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// get the 'next' method for the vector/map and call it
	is.push( Instruction( op_push, DevaObject( "", string( "next" ) ) ) );
	is.push( Instruction( op_tbl_load, DevaObject( "", false ) ) );
	is.push( Instruction( op_call, DevaObject( "", (size_t)0, false ) ) );

	// back-patch the return address for the call to 'next'
	is[loc] = Instruction( op_push, DevaObject( "", is.Offset(), true ) );

	// top of the stack now has the two-item vector returned from 'next'
	is.push( Instruction( op_dup, DevaObject( "", 0.0 ) ) );
	is.push( Instruction( op_push, DevaObject( "", 0.0 ) ) );
	is.push( Instruction( op_tbl_load, DevaObject( "", (size_t)1, false ) ) );
	// (stack now has bool on top and vector next)
	// if the first item is 'false', no loop - jump to end

	// save the instruction location for back-patching
	while_jmpf_stack.push_back( is.size() );
	// start a new loop context for back-patching 'break' statements
	loop_break_locations.push_back( new vector<size_t>() );

	// generate a place-holder op for the jmpf (which ends looping)
	is.push( Instruction( op_jmpf, DevaObject( "", (size_t)-1, true ) ) );
	// second item is the 'item_name' (or name/value pair) 'for' loop variable 
	// (with vector now on top of stack)
	is.push( Instruction( op_push, DevaObject( "", 1.0 ) ) );
	is.push( Instruction( op_tbl_load, DevaObject( "", (size_t)1, false ) ) );
	// for loop var (or pair) is now on top of stack - assign it to 'item_name'
	if( is_map )
	{
		// copy the pair (vector) with the key/value
		is.push( Instruction( op_dup, DevaObject( "", 0.0 ) ) );
		// push the index of the map key (0)
		is.push( Instruction( op_push, DevaObject( "", 0.0 ) ) );
		// load the 'key' from the pair
		is.push( Instruction( op_tbl_load, DevaObject( "", (size_t)1, false ) ) );
		// push the name of the 'key' var
		is.push( Instruction( op_push, DevaObject( item_name, sym_unknown ) ) );
		// swap the top two items for the store op
		is.push( Instruction( op_swap ) );
		// store the 'key'
		is.push( Instruction( op_store ) );
		// copy of the pair is on top of the stack again
		// push the index of the map value (1)
		is.push( Instruction( op_push, DevaObject( "", 1.0 ) ) );
		// load the 'value' from the pair
		is.push( Instruction( op_tbl_load, DevaObject( "", (size_t)1, false ) ) );
		// push the name of the 'value' var
		is.push( Instruction( op_push, DevaObject( item_value_name, sym_unknown ) ) );
		// swap the top two items for the store op
		is.push( Instruction( op_swap ) );
		// store the 'value'
		is.push( Instruction( op_store ) );
	}
	else
	{
		is.push( Instruction( op_push, DevaObject( item_name, sym_unknown ) ) );
		// swap the top two items for the store op
		is.push( Instruction( op_swap ) );
		// store
		is.push( Instruction( op_store ) );
	}
	
	// loop body happens here...
}

void gen_IL_for_s( iter_t const & i, InstructionStream & is, bool needs_leave )
{
	// if we need to add a leave op (because the loop body was a single
	// statement and not a compound statement)
	if( needs_leave )
		gen_IL_compound_statement( i->children.begin() + 1, is, i );

	// add the jump-to-start
	size_t loop_loc = loop_stack.back();
	loop_stack.pop_back();
	is.push( Instruction( op_jmp, DevaObject( "", loop_loc, true ) ) );

	int loop_scopes = loop_scope_stack.back();
	if( loop_scopes != 0 )
		throw DevaICE( "unclosed scopes in for loop" );
	loop_scope_stack.pop_back();

	// back-patch the jmpf
	// pop the last 'loop' location off the 'loop stack'
	size_t jmpf_loc = while_jmpf_stack.back();
	while_jmpf_stack.pop_back();
	// write the current *file* offset, not instruction stream!
	is[jmpf_loc] = Instruction( op_jmpf, DevaObject( "", is.Offset(), true ) );

	// pop to remove the false result from 'next' from the stack
	is.push( Instruction( op_pop ) );

	// back-patch any 'break' statements
	// pop the loop from the loop stack
	vector<size_t>* breaks = loop_break_locations.back();
	loop_break_locations.pop_back();
	for( vector<size_t>::iterator it = breaks->begin(); it != breaks->end(); ++it )
	{
		// back-patch the instruction at the break to point to the current
		// location in the instruction stream (end of this loop)
		is[*it] = Instruction( op_jmp, DevaObject( "", is.Offset(), true ) );
	}
	delete breaks;

	// pop to remove the vector/map
	is.push( Instruction( op_pop ) );
}

static vector<size_t> if_stack;

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

	generate_line_num( i, is );
	// push the current location in the instruction stream onto the 'if stack'
	if_stack.push_back( is.size() );
	// generate a jump placeholder 
	// for a jump *over* the child (statement|compound_statement)
	is.push( Instruction( op_jmpf, DevaObject( "", (size_t)-1, true ) ) );
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
	is[if_loc] = Instruction( op_jmpf, DevaObject( "", is.Offset(), true ) );
}

static vector<size_t> else_stack;
void pre_gen_IL_else_s( iter_t const & i, InstructionStream & is )
{
	generate_line_num( i, is );
	// push the current location in the instruction stream onto the 'else' stack
	// write the current *file* offset, not instruction stream!
	else_stack.push_back( is.size() );
	// generate the jump placeholder
	is.push( Instruction( op_jmp, DevaObject( "", (size_t)-1, true ) ) );
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
	is[else_loc] = Instruction( op_jmp, DevaObject( "", is.Offset(), true ) );
}

void gen_IL_import( iter_t const & i, InstructionStream & is )
{
	string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
	generate_line_num( i, is );
	is.push( Instruction( op_import, DevaObject( "", name ) ) );
}

// stack of fcn returns for back-patching return addresses in
static vector<size_t> fcn_call_stack;
void gen_IL_identifier( iter_t const & i, InstructionStream & is, iter_t const & parent, bool get_fcn_from_stack, int child_num )
{
	// simple variable and map/vector lookups handled by caller,
	// only need to handle function calls here
	if( i->children[0].value.id() == arg_list_exp_id )
	{
		// generate the call for the initial arg list (there could be more
		// lists chained together)
		string name = strip_symbol( string( i->value.begin(), i->value.end() ) );
		// force a line num for calls
		generate_line_num( i, is, true );
		int num_args = i->children[0].children.size() - 2;
		if( get_fcn_from_stack )
			// add the call instruction, passing no args to indicate it needs
			// to pull the function off the stack
			is.push( Instruction( op_call, DevaObject( "", (size_t)num_args, false ) ) );
		else
			// add the call instruction with the name of the fcn to call
			is.push( Instruction( op_call, DevaObject( name, sym_function_call ), DevaObject( "", (size_t)num_args, false ) ) );

		// back-patch the return address push op
		int ret_addr_loc = fcn_call_stack.back();
		fcn_call_stack.pop_back();
		// write the current *file* offset, not instruction stream!
		is[ret_addr_loc] = Instruction( op_push, DevaObject( "", (size_t)is.Offset(), true ) );

		// if there are no more arg lists (to consume the return value of this
		// call) AND
		// if the parent is the translation unit or a compound statment,
		// *OR* if the parent is a loop, a condition (if) *AND* this is the conditional,
		// then the return value is not being used, 
		// emit a pop instruction to discard it
		boost::spirit::classic::parser_id id = parent->value.id();
		if( (i->children.size() == 1 || i->children[1].value.id() != arg_list_exp_id )
			&& (id == translation_unit_id 
			|| id == compound_statement_id 
			// oddly, if the dot-op expression is at the global scope, there may
			// not be a translation_unit as its parent...
			|| id == dot_op_id
			// and, if this is from code that is being run dynamically, an
			// identifier may itself be the parent
			|| id == identifier_id
			|| id == func_id 
			|| id == else_s_id
			|| (id == while_s_id && child_num != 0)
			|| (id == for_s_id && child_num != 0)
			|| (id == if_s_id  && child_num != 0) ) )
		{
			is.push( Instruction( op_pop ) );
		}

		// generate the calls, back patches and (if needed)
		// return-value-pops for any chained arg lists
		for( int c = 1; c < i->children.size(); ++c )
		{
			if( i->children[c].value.id() == arg_list_exp_id )
			{
				int num_args = i->children[c].children.size() - 2;
				// force a line num for calls
				generate_line_num( i, is, true );
				// add the call instruction, passing no args to indicate it needs
				// to pull the function off the stack
				is.push( Instruction( op_call, DevaObject( "", (size_t)num_args, false ) ) );

				// back-patch the return address push op
				int ret_addr_loc = fcn_call_stack.back();
				fcn_call_stack.pop_back();
				// write the current *file* offset, not instruction stream!
				is[ret_addr_loc] = Instruction( op_push, DevaObject( "", (size_t)is.Offset(), true ) );

				// if there are no more arg lists (to consume the return value of this
				// call) AND
				// if the parent is the translation unit or a compound statment,
				// *OR* if the parent is a loop, a condition (if) *AND* this is the conditional,
				// then the return value is not being used, 
				// emit a pop instruction to discard it
				boost::spirit::classic::parser_id id = parent->value.id();
				if( (i->children.size() == c+1 || i->children[c+1].value.id() != arg_list_exp_id )
					&& (id == translation_unit_id 
					|| id == compound_statement_id 
					// oddly, if the dot-op expression is at the global scope, there may
					// not be a translation_unit as its parent...
					|| id == dot_op_id
					// and, if this is from code that is being run dynamically, an
					// identifier may itself be the parent
					|| id == identifier_id
					|| id == func_id 
					|| id == else_s_id
					|| (id == while_s_id && child_num != 0)
					|| (id == for_s_id && child_num != 0)
					|| (id == if_s_id  && child_num != 0) ) )
				{
					is.push( Instruction( op_pop ) );
				}
			}
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
	generate_line_num( i, is );
	// exclude the '{' and '}' from the count of children
	int num_children = i->children.size() - 2;
	if( num_children > 0 )
		is.push( Instruction( op_new_map, DevaObject( "", (size_t)num_children, false ) ) );
	else
		is.push( Instruction( op_new_map ) );
}

void gen_IL_vec_op( iter_t const & i, InstructionStream & is )
{
	// new vector
	generate_line_num( i, is );
	// exclude the '[' and ']' from the count of children
	int num_children = i->children.size() - 2;
	if( num_children > 0 )
		is.push( Instruction( op_new_vec, DevaObject( "", (size_t)num_children, false ) ) );
	else
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
	generate_line_num( i, is );
	// if the lhs is a local decl
	if( i->children[0].value.id() == local_decl_id )
	{
		// first child of the local_decl is the keyword 'local'
		// second child is the variable name
		is.push( Instruction( op_store, DevaObject( "", true ) ) );
	}
	else
		is.push( Instruction( op_store ) );
}

void gen_IL_add_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	
	generate_line_num( i, is );
	// dup 1 => get the lhs dup'd on top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// swap  => swap it so we have (<bottom_of_stack>, ..., lhs, lhs, rhs)
	is.push( Instruction( op_swap ) );

	// do the add
	is.push( Instruction( op_add ) );

	// do the assignment
	is.push( Instruction( op_store ) );
}

void gen_IL_sub_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	
	generate_line_num( i, is );
	// dup 1 => get the lhs dup'd on top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// swap  => swap it so we have (<bottom_of_stack>, ..., lhs, lhs, rhs)
	is.push( Instruction( op_swap ) );

	// do the sub
	is.push( Instruction( op_sub ) );

	// do the assignment
	is.push( Instruction( op_store ) );
}

void gen_IL_mul_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	
	generate_line_num( i, is );
	// dup 1 => get the lhs dup'd on top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// swap  => swap it so we have (<bottom_of_stack>, ..., lhs, lhs, rhs)
	is.push( Instruction( op_swap ) );

	// do the mul
	is.push( Instruction( op_mul ) );

	// do the assignment
	is.push( Instruction( op_store ) );
}

void gen_IL_div_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	
	generate_line_num( i, is );
	// dup 1 => get the lhs dup'd on top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// swap  => swap it so we have (<bottom_of_stack>, ..., lhs, lhs, rhs)
	is.push( Instruction( op_swap ) );

	// do the div
	is.push( Instruction( op_div ) );

	// do the assignment
	is.push( Instruction( op_store ) );
}

void gen_IL_mod_assignment_op( iter_t const & i, InstructionStream & is )
{
	// store (top of stack (rhs) into the arg (lhs), both args are already on
	// the stack)
	
	generate_line_num( i, is );
	// dup 1 => get the lhs dup'd on top of the stack
	is.push( Instruction( op_dup, DevaObject( "", 1.0 ) ) );
	// swap  => swap it so we have (<bottom_of_stack>, ..., lhs, lhs, rhs)
	is.push( Instruction( op_swap ) );

	// do the mod
	is.push( Instruction( op_mod ) );

	// do the assignment
	is.push( Instruction( op_store ) );
}


void gen_IL_logical_op( iter_t const & i, InstructionStream & is )
{
	// '||' or '&&'
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	if( s == "||" )
	{
		generate_line_num( i, is );
		is.push( Instruction( op_or ) );
	}
	else if( s == "&&" )
	{
		generate_line_num( i, is );
		is.push( Instruction( op_and ) );
	}
	// else?
}

void gen_IL_relational_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	generate_line_num( i, is );
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
	generate_line_num( i, is );
	// multiply
	if( s == "*" || s == "*=" )
		is.push( Instruction( op_mul ) );
	// divide
	else if( s == "/" || s == "/=" )
		is.push( Instruction( op_div ) );
	// modulus
	else if( s == "%" || s == "%=" )
		is.push( Instruction( op_mod ) );
	// else?
}

void gen_IL_add_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	generate_line_num( i, is );
	// add 
	if( s == "+" || s == "+=" )
		is.push( Instruction( op_add ) );
	// subract
	else if( s == "-" || s == "-=" )
		is.push( Instruction( op_sub ) );
	// else?
}

void gen_IL_unary_op( iter_t const & i, InstructionStream & is )
{
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	generate_line_num( i, is );
	// negate operator
	if( s == "-" )
		is.push( Instruction( op_neg ) );
	// not operator
	else if( s == "!" )
		is.push( Instruction( op_not ) );
}

void gen_IL_dot_op( iter_t const & i, InstructionStream & is, bool is_method_call /*= false*/ )
{
	// a.b = a["b"]  <== dot op is syntactic sugar for map lookup
	// first arg = lhs
	// second arg = rhs
	generate_line_num( i, is );
	// if this IS a method call, we need to indicate so to the tbl_load
	// instruction
	if( is_method_call )
		is.push( Instruction( op_tbl_load, DevaObject( "", false  ) ) );
	else
		is.push( Instruction( op_tbl_load ) );
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
	generate_line_num( i, is );
	// push the (offset for the) return address
	// save the location for back-patching the proper return address (address
	// *after* the call is made)
	fcn_call_stack.push_back( is.size() );
	// (prior to fcn arguments being pushed)
	is.push( Instruction( op_push, DevaObject( "", (size_t)-1, true ) ) );
}

void gen_IL_arg_list_exp( iter_t const & i, InstructionStream & is )
{
	// no op
}

void gen_IL_arg_list_decl( iter_t const & i, InstructionStream & is )
{
	// for each arg that is an identifier or an arg (i.e. not a paren)
	int num_children = i->children.size();
	for( int j = 0; j < num_children; ++j )
	{
		if( i->children[j].value.id() == identifier_id )
		{
			string name = strip_symbol( string( i->children[j].value.begin(), i->children[j].value.end() ) );
			// create an argument 
			generate_line_num( i, is );
			is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ) ) );
		}
		else if( i->children[j].value.id() == arg_id )
		{
			// first child is an identifier
			string name = strip_symbol( string( i->children[j].children[0].value.begin(), i->children[j].children[0].value.end() ) );
			// second child is the default value, a boolean, number, string or identifier
			// create an argument 
			generate_line_num( i, is );
			string s = strip_symbol( string( i->children[j].children[1].value.begin(), i->children[j].children[1].value.end() ) );
			if( i->children[j].children[1].value.id() == boolean_id )
			{
				if( s == "true" )
					is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ), DevaObject( "",  true ) ) );
				else
					is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ), DevaObject( "",  false ) ) );
			}
			else if( i->children[j].children[1].value.id() == number_id )
			{
				double n = parse_number( s );
				is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ), DevaObject( "",  n  ) ) );
			}
			else if( i->children[j].children[1].value.id() == string_id )
			{
				string str = unescape( strip_quotes( s ) );
				is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ), DevaObject( "",  str  ) ) );
			}
			else if( i->children[j].children[1].value.id() == identifier_id )
			{
				is.push( Instruction( op_defarg, DevaObject( name, sym_unknown ), DevaObject( s,  sym_unknown  ) ) );
			}
		}
	}
	// push one last "empty" defarg to mark the end-of-arguments
	is.push( Instruction( op_defarg ) );
}

void gen_IL_key_exp( iter_t const & i, InstructionStream & is )
{
	// only called to generate table loads, not stores (see assignment op handling
	// in compile.cpp, generate_IL_for_node() for tbl store code gen)
	// a[b] <= a is parent, b is child[1] ('[' is child[0] and ']' is child[2])
	generate_line_num( i, is );
	is.push( Instruction( op_tbl_load, DevaObject( "", (size_t)i->children.size()-2, false ) ) );
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

void pre_gen_IL_compound_statement( iter_t const & i, InstructionStream & is, iter_t const & parent )
{
	// don't generate an 'enter' statement for function defs, as the 'def'
	// handling code does this for you (so that the defarg's are included
	// *inside* the new scope, not outside)
	if( parent->value.id() != func_id )
	{
		// generate enter
		generate_line_num( i, is );
		gen_IL_enter_scope( is );
	}
}

void gen_IL_compound_statement( iter_t const & i, InstructionStream & is, iter_t const & parent )
{
	// don't generate a 'leave' statement for function defs, as the 'return'
	// statement accomplishes the same thing
	if( parent->value.id() != func_id )
	{
		// generate leave
		generate_line_num( i, is );
		gen_IL_leave_scope( is );
	}
}

void gen_IL_break_statement( iter_t const & i, InstructionStream & is )
{
	// generate break op with a reference to the start of the loop
	int loop_scopes = loop_scope_stack.back();

	generate_line_num( i, is );
	// generate appropriate number of leave ops
	for( int i = 0; i < loop_scopes; ++i )
		is.push( Instruction( op_leave ) );
	// save the location for back-patching
	// (to be double-safe, check to ensure we're in a loop...)
	if( loop_break_locations.size() == 0 )
		throw DevaICE( "Invalid break statement: Not inside a loop. Parser shouldn't have allowed this." );
	loop_break_locations.back()->push_back( is.size() );
	// generate a place-holder op for the jmp (which ends looping)
	is.push( Instruction( op_jmp, DevaObject( "", (size_t)-1, true ) ) );
}

void gen_IL_continue_statement( iter_t const & i, InstructionStream & is )
{
	// generate jump to beginning of loop
	size_t loop_loc = loop_stack.back();
	int loop_scopes = loop_scope_stack.back();

	generate_line_num( i, is );
	// generate appropriate number of leave ops
	// (number of scopes from if/else conditions plus one for the loop we're
	// breaking out of)
	for( int i = 0; i < loop_scopes; ++i )
		is.push( Instruction( op_leave ) );
	// write the current *file* offset, not instruction stream!
	is.push( Instruction( op_jmp, DevaObject( "", (size_t)loop_loc, true ) ) );
}

void gen_IL_return_statement( iter_t const & i, InstructionStream & is )
{
	// last child is always semi-colon
	// generate return
	generate_line_num( i, is );
	if( i->children.size() > 1 )
		is.push( Instruction( op_return ) );
	else if( i->children.size() == 1 )
	{
		is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
		is.push( Instruction( op_return ) );
	}
	// else?
}

// used to set-up scope tracking for conditionals (i.e. if/else)
// (needed for break/continue code gen)
void gen_IL_enter_scope( InstructionStream & is )
{
	// gen an enter instruction
	is.push( Instruction( op_enter ) );

	// track the scope
	if( loop_scope_stack.size() > 0 )
	{
		int loop_scopes = loop_scope_stack.back();
		loop_scope_stack.pop_back();
		loop_scopes++;
		loop_scope_stack.push_back( loop_scopes );
	}
}

// used to set-up scope tracking for conditionals (i.e. if/else)
// (needed for break/continue code gen)
void gen_IL_leave_scope( InstructionStream & is )
{
	// gen a leave instruction
	is.push( Instruction( op_leave ) );

	// track the scope
	if( loop_scope_stack.size() > 0 )
	{
		int loop_scopes = loop_scope_stack.back();
		loop_scope_stack.pop_back();
		loop_scopes--;
		loop_scope_stack.push_back( loop_scopes );
	}
}

