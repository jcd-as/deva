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

// compile.cpp
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * change asserts into (non-fatal) errors
// * semantic checking functions for all valid node types that can have children

#include <iomanip>
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "compile.h"
#include "semantics.h"
#include "fileformat.h"


// helpers
//////////////////////////////////////////////////////////////////////

// stack of scopes, for building the global scope table
ScopeBuilder scope_bldr;

void add_symbol( iterator_t start, iterator_t end )
{
	string s( start, end );
	s = strip_symbol( s );

	// don't add keywords
	if( is_keyword( s ) )
		return;
	// only add to the current scope if it can't be found in a parent scope
	if( find_identifier_in_parent_scopes( s, scope_bldr.back().second, scopes ) )
		return;

	// the current scope is the top of the scope_bldr stack
	scope_bldr.back().second->insert( make_pair( s, SymbolInfo( sym_unknown ) ) );
}

// emit an error message
void emit_error( NodeInfo ni, string msg )
{
	// format = filename:linenum: msg
	cout << ni.file << ":" << ni.line << ":" << " error: " << msg << endl;
}

void emit_error( string filename, string msg, int line )
{
	// format = filename:linenum: msg
	cout << filename << ":" << line << ":" << " error: " << msg << endl;
}

void emit_error( DevaSemanticException & e )
{
	// format = filename:linenum: msg
	cout << e.node.file << ":" << e.node.line << ":" << " error: " << e.what() << endl;
}

// emit a warning message
void emit_warning( NodeInfo ni, string msg )
{
	// format = filename:linenum: msg
	cout << ni.file << ":" << ni.line << ":" << " warning: " << msg << endl;
}

void emit_warning( string filename, string msg, int line )
{
	// format = filename:linenum: msg
	cout << filename << ":" << line << ":" << " warning: " << msg << endl;
}

void emit_warning( DevaSemanticException & e )
{
	// format = filename:linenum: msg
	cout << e.node.file << ":" << e.node.line << ":" << " warning: " << e.what() << endl;
}


//////////////////////////////////////////////////////////////////////
// compile/parse/check/code-gen functions
//////////////////////////////////////////////////////////////////////


static bool global_sym_tab_created = false;

// parse a buffer containing deva code
// returns the AST tree
tree_parse_info<iterator_t, factory_t> ParseText( string filename, const char* const input, long input_len )
{
	// create our grammar parser
	DevaGrammar deva_p;

	// create the position iterator for the parser
	iterator_t begin( input, input + input_len, filename.c_str() );
	iterator_t end;

	// create our initial (global) scope
	if( !global_sym_tab_created )
	{
		SymbolTable* globals = new SymbolTable();
		pair<int, SymbolTable*> sym( 0, globals );
		scope_bldr.push_back( sym );
		scopes[sym.first] = sym.second;
		global_sym_tab_created = true;
	}

	// parse 
	tree_parse_info<iterator_t, factory_t> info;
	info = ast_parse<factory_t>( begin, end, deva_p, (space_p | comment_p( "#" )) );

	return info;
}

// parse a deva file
// returns the AST tree
tree_parse_info<iterator_t, factory_t> ParseFile( string filename, istream & file )
{
	// get the length of the file
	file.seekg( 0, ios::end );
	int length = file.tellg();
	file.seekg( 0, ios::beg );
	// allocate memory to read the file into
	char* buf = new char[length+1];
	// zero it out
	memset( buf, 0, length+1 );
	// read the file
	file.read( buf, length );

	// parse the input text
	tree_parse_info<iterator_t, factory_t> ret = ParseText( filename, buf, length );

	// free the buffer with the code in it
	delete[] buf;

	return ret;
}

// check the semantics of a parsed AST
// returns true on correct semantics, false on incorrect
void eval_node( iter_t const & i );
bool CheckSemantics( tree_parse_info<iterator_t, factory_t> info )
{
	// walk the tree, checking the semantics of each node
	iter_t i = info.trees.begin();
	try
	{
		eval_node( i );
	}
	catch( DevaSemanticException & e )
	{
		emit_error( e );
		return false;
	}
	return true;
}

void walk_children( iter_t const & i )
{
//	cout << "in scope " << i->value.value().scope << ", at line: " << i->value.value().line << endl;
//	string s( i->value.begin(), i->value.end() );
//	cout << strip_symbol( s ) << endl;

	// walk children
	for( int c = 0; c < i->children.size(); c++ )
	{
		eval_node( i->children.begin() + c );
	}
}

void eval_node( iter_t const & i )
{
	// set the symbol text for the node
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.sym = s;
	i->value.value( ni );

//	cout << "In eval_node. i->value = " << s << 
//		" i->children.size() = " << i->children.size() << endl;

	//////////////////////////////////////////////////////////////////
	// valid node types:
	//////////////////////////////////////////////////////////////////
	// number
	if( i->value.id() == parser_id( number_id ) )
	{
		// self and possibly semi-colon
		assert( i->children.size() == 0 || i->children.size() == 1 );
		walk_children( i );
		check_number( i );
	}
	// string
	else if( i->value.id() == parser_id( string_id ) )
	{
		// self and possibly semi-colon
		assert( i->children.size() == 0 || i->children.size() == 1 );
		walk_children( i );
		check_string( i );
	}
	// boolean
	else if( i->value.id() == parser_id( boolean_id ) )
	{
		// self and possibly semi-colon
		assert( i->children.size() == 0 || i->children.size() == 1 );
		walk_children( i );
		check_boolean( i );
	}
	// null
	else if( i->value.id() == parser_id( null_id ) )
	{
		// self and possibly semi-colon
		assert( i->children.size() == 0 || i->children.size() == 1 );
		walk_children( i );
		check_null( i );
	}
	// func (keyword 'def' for defining functions)
	else if( i->value.id() == parser_id( func_id ) )
	{
		// children: id, arg_list, compound_statement | statement
		//assert( i->children.size() == 3 );
		walk_children( i );
		check_func( i );
	}
	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		pre_check_while_s( i );
		walk_children( i );
		check_while_s( i );
	}
	// for_s (keyword 'for')
	else if( i->value.id() == parser_id( for_s_id ) )
	{
		pre_check_for_s( i );
		walk_children( i );
		check_for_s( i );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		walk_children( i );
		check_if_s( i );
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		walk_children( i );
		check_else_s( i );
	}
	else if( i->value.id() == parser_id( import_statement_id ) )
	{
		walk_children( i );
		check_import_s( i );
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		// can have multiple arg_lists & semi-colon
		walk_children( i );
		check_identifier( i );
	}
	// module_name
	else if( i->value.id() == parser_id( module_name_id ) )
	{
		// can have semi-colon
		assert( i->children.size() < 2 );
		walk_children( i );
		check_module_name( i );
	}
	// in op ('in' keyword in for loops)
	else if( i->value.id() == parser_id( in_op_id ) )
	{
		walk_children( i );
		check_in_op( i );
	}
	// map construction op
	else if( i->value.id() == parser_id( map_op_id ) )
	{
		assert( i->children.size() == 0 );
		walk_children( i );
		check_map_op( i );
	}
	// vector construction op
	else if( i->value.id() == parser_id( vec_op_id ) )
	{
		walk_children( i );
		check_vec_op( i );
	}
	// semicolon op
	else if( i->value.id() == parser_id( semicolon_op_id ) )
	{
		assert( i->children.size() == 0 );
		walk_children( i );
		check_semicolon_op( i );
	}
	// assignment op
	else if( i->value.id() == parser_id( assignment_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_assignment_op( i );
	}
	// add/sub/mul/div/mod assignment op
	else if( i->value.id() == parser_id( add_assignment_op_id ) ||
			 i->value.id() == parser_id( sub_assignment_op_id ) ||
			 i->value.id() == parser_id( mul_assignment_op_id ) ||
			 i->value.id() == parser_id( div_assignment_op_id ) ||
			 i->value.id() == parser_id( mod_assignment_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_op_assignment_op( i );
	}
	// logical op
	else if( i->value.id() == parser_id( logical_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_logical_op( i );
	}
	// relational op
	else if( i->value.id() == parser_id( relational_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_relational_op( i );
	}
	// mult_op
	else if( i->value.id() == parser_id( mult_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_mult_op( i );
	}
	// add_op
	else if( i->value.id() == parser_id( add_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_add_op( i );
	}
	// unary_op
	else if( i->value.id() == parser_id( unary_op_id ) )
	{
		// operand and possibly semi-colon
		assert( i->children.size() == 1 || i->children.size() == 2 );
		walk_children( i );
		check_unary_op( i );
	}
	// dot op
	else if( i->value.id() == parser_id( dot_op_id ) )
	{
		// operands (lhs & rhs) and possibly semi-colon
		assert( i->children.size() == 2 || i->children.size() == 3 );
		walk_children( i );
		check_dot_op( i );
	}
	// paren ops
	else if( i->value.id() == parser_id( open_paren_op_id )
		  || i->value.id() == parser_id( close_paren_op_id ) )
	{
		assert( i->children.size() == 0 );
		walk_children( i );
		check_paren_op( i );
	}
	// bracket ops
	else if( i->value.id() == parser_id( open_bracket_op_id ) 
			|| i->value.id() == parser_id( close_bracket_op_id ) )
	{
		assert( i->children.size() == 0 );
		walk_children( i );
		check_bracket_op( i );
	}
	// arg list exp
	else if( i->value.id() == parser_id( arg_list_exp_id ) )
	{
		walk_children( i );
		check_arg_list_exp( i );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
		walk_children( i );
		check_arg_list_decl( i );
	}
	else if( i->value.id() == parser_id( arg_id ) )
	{
		check_arg( i );
	}
	// key exp
	else if( i->value.id() == parser_id( key_exp_id ) )
	{
		walk_children( i );
		check_key_exp( i );
	}
	// const decl
	else if( i->value.id() == parser_id( const_decl_id ) )
	{
		pre_check_const_decl( i );
		walk_children( i );
		check_const_decl( i );
	}
	// local decl
	else if( i->value.id() == parser_id( local_decl_id ) )
	{
		walk_children( i );
		check_local_decl( i );
	}
	// new decl ('new' operator)
	else if( i->value.id() == parser_id( new_decl_id ) )
	{
		walk_children( i );
		check_new_decl( i );
	}
	// constant (keyword 'const')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		walk_children( i );
		check_constant( i );
	}
	// local (keyword 'local')
	else if( i->value.id() == parser_id( local_id ) )
	{
		walk_children( i );
		check_local( i );
	}
	// translation unit
	else if( i->value.id() == parser_id( translation_unit_id ) )
	{
		walk_children( i );
		check_translation_unit( i );
	}
	// compound statement
	else if( i->value.id() == parser_id( compound_statement_id ) )
	{
		walk_children( i );
		check_compound_statement( i );
	}
	// break statement
	else if( i->value.id() == parser_id( break_statement_id ) )
	{
		walk_children( i );
		check_break_statement( i );
	}
	// continue statement
	else if( i->value.id() == parser_id( continue_statement_id ) )
	{
		walk_children( i );
		check_continue_statement( i );
	}
	// return statement
	else if( i->value.id() == parser_id( return_statement_id ) )
	{
		walk_children( i );
		check_return_statement( i );
	}
	// class decl
	else if( i->value.id() == parser_id( class_decl_id ) )
	{
		walk_children( i );
	}
	//////////////////////////////////////////////////////////////////
	// nodes of these types shouldn't be created:
	//////////////////////////////////////////////////////////////////
	// comma_op
	else if( i->value.id() == parser_id( comma_op_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// factor_exp
	else if( i->value.id() == parser_id( factor_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// postfix only exp
	else if( i->value.id() == parser_id( postfix_only_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// primary_exp
	else if( i->value.id() == parser_id( primary_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// mult_exp
	else if( i->value.id() == parser_id( mult_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// add_exp
	else if( i->value.id() == parser_id( add_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// relational exp
	else if( i->value.id() == parser_id( relational_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// logical exp
	else if( i->value.id() == parser_id( logical_exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// exp
	else if( i->value.id() == parser_id( exp_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// statement
	else if( i->value.id() == parser_id( statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// top-level statement
	else if( i->value.id() == parser_id( top_level_statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// function decl
	else if( i->value.id() == parser_id( func_decl_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// while statement
	else if( i->value.id() == parser_id( while_statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// for statement
	else if( i->value.id() == parser_id( for_statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// if statement
	else if( i->value.id() == parser_id( if_statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// else statement
	else if( i->value.id() == parser_id( else_statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	// jump statement
	else if( i->value.id() == parser_id( jump_statement_id ) )
	{
		emit_error( i->value.value(), "Semantic error: Encountered invalid node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
	else
	{
		// error, invalid node type
		emit_error( i->value.value(), "Semantic error: Encountered unknown node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
	}
}


void generate_IL_for_node( iter_t const & i, InstructionStream & is, iter_t const & parent, int child_num );
extern bool debug_info_on;
// generate IL stream
bool GenerateIL( tree_parse_info<iterator_t, factory_t> info, InstructionStream & is, bool debug_info )
{
	// set global flag (shared with instructions.cpp, where it is defined)
	debug_info_on = debug_info;
	try
	{
		// if we're NOT in debug mode, we at least need one "line num" op to say
		// what file we're in (in order for 'import' and module-finding to work
		// properly)
		if( !debug_info_on )
		{
			// get the node info
			// (the 'translation_unit' node doesn't have the file/line num info)
			iter_t it = info.trees.begin()->children.begin();
			NodeInfo ni = ((NodeInfo)(it->value.value()));
			is.push( Instruction( op_line_num, DevaObject( "", ni.file ), DevaObject( "", (size_t)0, false) ) );
		}

		// walk the tree, generating the IL for each node
		iter_t i = info.trees.begin();
		generate_IL_for_node( i, is, i, 0 );

		// last node always a halt
		is.push( Instruction( op_halt ) );
	}
	catch( DevaSemanticException & e )
	{
		emit_error( e.node, e.what() );
		return false;
	}

	return true;
}

void walk_children( iter_t const & i, InstructionStream & is )
{
//	cout << "in scope " << i->value.value().scope << ", at line: " << i->value.value().line << endl;
//	string s( i->value.begin(), i->value.end() );
//	cout << strip_symbol( s ) << endl;

	// walk children
	for( int c = 0; c < i->children.size(); ++c )
	{
		generate_IL_for_node( i->children.begin() + c, is, i, c );
	}
}

void reverse_walk_children( iter_t const & i, InstructionStream & is )
{
	// walk children in reverse order 
	// (for handling arguments to functions)
	for( int c = i->children.size() - 1; c >= 0; --c )
	{
		generate_IL_for_node( i->children.begin() + c, is, i, c );
	}
}

// helper to walk sub-tree looking for a return statement
bool walk_looking_for_return( iter_t const & iter )
{
	if( iter->value.id() == return_statement_id )
		return true;
	for( int j = 0; j < iter->children.size(); ++j )
	{
		if( walk_looking_for_return( iter->children.begin() + j ) )
			return true;
	}
	return false;
}

// helper to walk a dot_op branch of the AST to see if it is ultimately a method
// call (has an arg_list_exp) and generate the ops for the args if it is
void walk_children_for_method_call( iter_t i, InstructionStream & is )
{
	// if this is an identifier with a child that is an arg list
	if( i->value.id() == identifier_id && i->children.size() > 0 && i->children[0].value.id() == arg_list_exp_id )
	{
		// generate the ret-val placeholder (op_push 'sym_address')
		pre_gen_IL_arg_list_exp( i, is );

		// generate the code for the arg list
		reverse_walk_children( i->children.begin(), is );
		gen_IL_arg_list_exp( i->children.begin(), is );

		return;
	}
	for( int j = 0; j < i->children.size(); ++j )
	{
		walk_children_for_method_call( i->children.begin() + j, is );
	}
}

// static for tracking parents to classes
static vector<DevaObject> parents;
void generate_IL_for_node( iter_t const & i, InstructionStream & is, iter_t const & parent, int child_num )
{
	///////////////////////////////////////
	// statics used for building classes
	///////////////////////////////////////
	// statics used for building classes
	// flag to indicate if we're inside a class def or not
	// (so we know whether to generate methods or classes. i.e. is there a
	// 'this' param or not?)
	static bool in_class_def = false;
	// static to hold the name of the class we're adding methods to
	static string class_name;
	// static to track the list of methods that were added
	static vector<string> method_names;
	///////////////////////////////////////

	// number
	if( i->value.id() == parser_id( number_id ) )
	{
		// self and possibly semi-colon
		walk_children( i, is );
		gen_IL_number( i, is );
	}
	// string
	else if( i->value.id() == parser_id( string_id ) )
	{
		// self and possibly semi-colon
		walk_children( i, is );
		gen_IL_string( i, is );
	}
	// boolean
	else if( i->value.id() == parser_id( boolean_id ) )
	{
		// self and possibly semi-colon
		walk_children( i, is );
		gen_IL_boolean( i, is );
	}
	// null
	else if( i->value.id() == parser_id( null_id ) )
	{
		// self and possibly semi-colon
		walk_children( i, is );
		gen_IL_null( i, is );
	}
	// func (keyword 'def' for defining functions)
	else if( i->value.id() == parser_id( func_id ) )
	{
		// children: id, arg_list, compound_statement | statement

		// create a function at this loc
		string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
		// write the current *file* offset, not instruction stream!
		generate_line_num( i, is );
		// if this is a method we need to append "@<classname>" to the function
		// name and add the method name to the list of names added to this class
		if( in_class_def )
		{
			name += "@" + class_name;
			method_names.push_back( name );
		}
		is.push( Instruction( op_defun, DevaObject( name, is.Offset(), true ) ) );

		// we need to generate an enter instruction to create the
		// proper scope for the fcn and its arguments (the 'leave' instruction 
		// isn't needed, as the return statement at the end of the 
		// function takes care of this)
		generate_line_num( i->children.begin()+2, is );
		is.push( Instruction( op_enter ) );

		// if this is a method we need to add the 'self' arg
		// (eq. of the 'this' pointer in c++) 
		if( in_class_def )
		{
			is.push( Instruction( op_defarg, DevaObject( string( "self" ), sym_unknown ) ) );
		}

		// second child is the arg_list, process it
		generate_IL_for_node( i->children.begin() + 1, is, i, 1 );

        // if this is a destructor ("delete" method), it cannot have arguments
        string del( "delete@" );
        del += class_name;
        if( in_class_def && name == del )
        {
            // make sure there are no args
            if( i->children[1].children.size() != 2 )
                throw DevaSemanticException( "Destructors cannot take parameters.", i->value.value() );
        }

		// if this is a constructor ("new" method), add code to call the
		// base-class constructors...
		string nw( "new@" );
		nw += class_name;
		if( in_class_def && name == nw )
		{
			// for each parent
			for( vector<DevaObject>::iterator it = parents.begin(); it != parents.end(); ++it )
			{
				// push the return address
				int ret_addr_loc = is.size();
				is.push( Instruction( op_push, DevaObject( "", (size_t)-1, true ) ) );
				// push self
				is.push( Instruction( op_push, DevaObject( "self", sym_unknown ) ) );
				// force a line num for calls
				generate_line_num( i, is, true );
				// call the base constructor
				string fcn( "new@" );
				fcn += it->name;
				is.push( Instruction( op_call, DevaObject( fcn.c_str(), sym_function_call ), DevaObject( "", (size_t)0, false ) ) );
				// back-patch the return address
				is[ret_addr_loc] = Instruction( op_push, DevaObject( "", (size_t)is.Offset(), true ) );
				// pop the return value, 'new' can't return anything
				is.push( Instruction( op_pop ) );
			}
		}

		// third child is statement|compound_statement, process it
		generate_IL_for_node( i->children.begin() + 2, is, i, 2 );

		// if no return statement was generated, we need to generate one
		bool returned = false;
		iter_t iter = i->children.begin() + 2;
		if( iter->value.id() == return_statement_id )
			returned = true;
		else
			returned = walk_looking_for_return( iter );
		if( !returned )
		{
			generate_line_num( iter, is );
			// constructors must return 'self'
			string basename( name, 0, 3 );
			if( basename == "new" )
			{
				is.push( Instruction( op_push, DevaObject( "self", sym_unknown ) ) );
				is.push( Instruction( op_return ) );
			}
			// all fcns return *something*
			else
			{
				is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
				is.push( Instruction( op_return ) );
			}
		}
	}
	else if( i->value.id() == parser_id( class_decl_id ) )
	{
		// first child is the identifier (class name)
		string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );

		// line number. write the current *file* offset, not instruction stream!
		generate_line_num( i, is );

		// next come parent classes
		int c = 1;
		parser_id type = i->children[c].value.id();
		parents.clear();
		while( type == identifier_id )
		{
			string parent = strip_symbol( string( i->children[c].value.begin(), i->children[c].value.end() ) );
			// add to list of parents
			parents.push_back( DevaObject( parent, sym_unknown ) );
			// next
			++c;
			type = i->children[c].value.id();
		}

		// create a new class object and store it in the given name
		is.push( Instruction( op_push, DevaObject( name, sym_unknown ) ) );
		is.push( Instruction( op_new_class, parents ) );
		is.push( Instruction( op_store ) );

		// subsequent children are the methods
		in_class_def = true;
		class_name = name;
		for( ; c < i->children.size(); ++c )
		{
			generate_IL_for_node( i->children.begin() + c, is, i, c );
		}
		// add all the method names created to the class object
		for( vector<string>::iterator i = method_names.begin(); i != method_names.end(); ++i )
		{
			// generate a tbl_store instruction to add the method to the class
			// "table" (class)
			is.push( Instruction( op_push, DevaObject( name, sym_unknown ) ) );
			// key (method name)
			is.push( Instruction( op_push, DevaObject( "", *i ) ) );
			// object (fcn)
			is.push( Instruction( op_push, DevaObject( *i, sym_unknown ) ) );
			is.push( Instruction( op_tbl_store ) );
		}
		// reset the static variables
		class_name = "";
		method_names.clear();
		in_class_def = false;
	}
	else if( i->value.id() == parser_id( new_decl_id ) )
	{
		int arg_idx;
		if( i->children.size() == 2 )
		{
			// first child is the identifier for the class name to create an
			// instance of
			string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
			arg_idx = 1;

			// line number
			generate_line_num( i, is );

			// push the return address for the call to 'new'
			int ret_addr_loc = is.size();
			is.push( Instruction( op_push, DevaObject( "", (size_t)-1, true ) ) );

			// number of args is size of children - 2 (for the open and close parens),
			int num_args = i->children[arg_idx].children.size() - 2;
			// push the args to 'new', if any
			if( num_args > 0 )
				reverse_walk_children( i->children.begin() + 1, is );
			
			// create the new instance
			is.push( Instruction( op_push, DevaObject( name, sym_unknown ) ) );
			// +1 for 'self'
			is.push( Instruction( op_new_instance, DevaObject( "", (size_t)(i->children[arg_idx].children.size()-1), false ) ) );	// puts the new instance on the stack
			// new instance is on the stack to act as the 'self' arg to 'new'
			// force a line num for calls
			generate_line_num( i, is, true );
			// call 'new' on the object
			string fcn( "new@" );
			fcn += name;
			is.push( Instruction( op_call, DevaObject( fcn.c_str(), sym_function_call ), DevaObject( "", (size_t)num_args, false ) ) );
			// back-patch the return address
			is[ret_addr_loc] = Instruction( op_push, DevaObject( "", (size_t)is.Offset(), true ) );
		}
		// if not 2 args, must be 3
		// FUTURE: if more than 'module.class' is allowed, then this will need
		// to change
		else
		{
			string module = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
			string name = strip_symbol( string( i->children[1].value.begin(), i->children[1].value.end() ) );
			arg_idx = 2;

			// line number
			generate_line_num( i, is );

			// push the return address for the call to 'new'
			int ret_addr_loc = is.size();
			is.push( Instruction( op_push, DevaObject( "", (size_t)-1, true ) ) );

			// push the args to 'new', if any
			// number of args is size of children - 2 (for the open and close parens),
			int num_args = i->children[arg_idx].children.size() - 2;
			if( num_args > 0 ) 
				reverse_walk_children( i->children.begin() + 1, is );
			
			// create the look up of the name in the module,
			// the resulting name will be on the stack
			is.push( Instruction( op_push , DevaObject( module.c_str(), sym_unknown ) ) );
			is.push( Instruction( op_push , DevaObject( "", name ) ) );
			is.push( Instruction( op_tbl_load ) );

			// number of args is size of children - 2 (for the open and close parens),
			// +1 for 'self'
			is.push( Instruction( op_new_instance, DevaObject( "", (size_t)(i->children[arg_idx].children.size()-1), false ) ) );	// puts the new instance on the stack
			// new instance is on the stack to act as the 'self' arg to 'new'
			// (execution engine will have to push the fcn object (e.g. 'foo@bar' offset = nnnn)
			// new, "un-constructed" instance is on the stack
			// get the 'new' fcn onto the stack
			string fcn( "new@" );
			fcn += name;
			is.push( Instruction( op_push , DevaObject( module.c_str(), sym_unknown ) ) );
			is.push( Instruction( op_push , DevaObject( "", fcn ) ) );
			is.push( Instruction( op_tbl_load ) );
			// force a line num for calls
			generate_line_num( i, is, true );
			// call 'new' on the object
			is.push( Instruction( op_call, DevaObject( "", (size_t)num_args, false ) ) );
			// back-patch the return address
			is[ret_addr_loc] = Instruction( op_push, DevaObject( "", (size_t)is.Offset(), true ) );
		}
	}

	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		// pre-gen stores the start value, for the jump-to-start
		pre_gen_IL_while_s( i, is );
		// first child has the relational op stuff, walk it
		generate_IL_for_node( i->children.begin(), is, i, 0 );
		// gen_IL adds the placeholder for the jmpf
		gen_IL_while_s( i, is );
		// second child has the statement|compound_statement, walk it
		generate_IL_for_node( i->children.begin() + 1, is, i, 1 );
		// post-gen-IL will to the back-patching and add the jump to start
		post_gen_IL_while_s( i, is );
	}
	// for_s (keyword 'for')
	else if( i->value.id() == parser_id( for_s_id ) )
	{
		// pre-gen stores the start value, for the jump-to-start
		pre_gen_IL_for_s( i, is );
		// first child (and second if this is a map walk) have variables
		// second or third child has the 'in' operator
		// third or fourth child has the statement|compound_statement, walk it
		if( i->children.size() == 3 )
			generate_IL_for_node( i->children.begin() + 2, is, i, 2 );
		else if( i->children.size() == 4 )
			generate_IL_for_node( i->children.begin() + 3, is, i, 3 );
		else
			throw DevaSemanticException( "invalid for loop", i->value.value() );

		// gen_IL adds the placeholder for the jmpf
		gen_IL_for_s( i, is );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		// first child has the conditional, walk it
		generate_IL_for_node( i->children.begin(), is, i, 0 );
		// generate the jump-placeholder
		pre_gen_IL_if_s( i, is );
		// second child has the statement|compound_statement, walk it
		generate_IL_for_node( i->children.begin() + 1, is, i, 1 );

		// if there's no 'else' clause
		// (third child, if any, has the else clause)
		if( i->children.size() < 3 )
		{
			// back-patch the jump-placeholder to jump to here
			gen_IL_if_s( i, is );
		}
		else
		{
			// generate the 'else' jump-placeholder... (i.e. the jump *over* the
			// else, for when the 'if' path was taken)
			pre_gen_IL_else_s( i, is );

			// back-patch the 'if' jump-placeholder to jump to here (for when
			// the 'if' path *wasn't* taken)
			gen_IL_if_s( i, is );

			// ...then walk the children (statement|compound_statement)
			generate_IL_for_node( i->children.begin() + 2, is, i, 2 );

			// ...and then back-patch the 'else' jump to jump to here
			gen_IL_else_s( i, is );
		}
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		// always a child of an 'if', which handles this, see above
		// so just walk the children
		generate_IL_for_node( i->children.begin(), is, i, 0 );
	}
	// import_statement_s
	else if( i->value.id() == parser_id( import_statement_id ) )
	{
		gen_IL_import( i, is );
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		string name = strip_symbol( string( i->value.begin(), i->value.end() ) );
		// if no children, simple variable
		if(i->children.size() == 0 )
		{
			generate_line_num( i, is );
			is.push( Instruction( op_push , DevaObject( name, sym_unknown ) ) );
		}
		// if the first child is an arg_list_exp, it's a fcn call
		else if( i->children[0].value.id() == arg_list_exp_id )
		{
			// first walk the children, in reverse order
			reverse_walk_children( i, is );

			// then generate the IL for this node (back-patching the return
			// address etc.)
			gen_IL_identifier( i, is, parent, false, child_num );
		}
		// if the id is followed by []'s it is either a vector or map look-up
		else if( i->children[0].value.id() == key_exp_id )
		{
			// first add the push of the name onto the stack
			generate_line_num( i->children.begin(), is );
			is.push( Instruction( op_push , DevaObject( name, sym_unknown ) ) );

			// then walk the children - key_exp will push it's children (the key) and
			// the key-lookup op
			reverse_walk_children( i, is );
		}
	}
	// in op ('in' keyword in for loops)
	else if( i->value.id() == parser_id( in_op_id ) )
	{
		walk_children( i, is );
		gen_IL_in_op( i, is );
	}
	// map construction op
	else if( i->value.id() == parser_id( map_op_id ) )
	{
		reverse_walk_children( i, is );
		gen_IL_map_op( i, is );
	}
	// vector construction op
	else if( i->value.id() == parser_id( vec_op_id ) )
	{
		reverse_walk_children( i, is );
		gen_IL_vec_op( i, is );
	}
	// semicolon op
	else if( i->value.id() == parser_id( semicolon_op_id ) )
	{
		walk_children( i, is );
		gen_IL_semicolon_op( i, is );
	}
	// assignment op
	else if( i->value.id() == parser_id( assignment_op_id ) )
	{
		// either the two sides of the assignment, or the two sides and a semi-colon

		// if the lhs is another assignment op, ICE
		if( i->children[0].value.id() == assignment_op_id )
			throw DevaICE( "Chained assignment found. Semantic checking should have disallowed this." );
			
		// if the lhs is an identifier with a key_exp (vec/map) then generate a
		// table store
		else if( i->children[0].value.id() == identifier_id && i->children[0].children.size() > 0 &&
			 i->children[0].children[0].value.id() == key_exp_id )
		{
			// push the identifier
			string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
			generate_line_num( i->children.begin(), is );
			is.push( Instruction( op_push , DevaObject( name, sym_unknown ) ) );
			// push the key exp
			reverse_walk_children( i->children[0].children.begin(), is );
			
			// walk the rhs
			generate_IL_for_node( i->children.begin() + 1, is, i, 1 );

			// add the tbl_store op
			generate_line_num( i->children.begin()+1, is );
			is.push( Instruction( op_tbl_store, DevaObject( "", (size_t)i->children[0].children[0].children.size()-2, false ) ) );
		}
		// a dot-op on the lhs also indicates a vector store instead of load, as
		// long as it is not a function call.
		else if ( i->children[0].value.id() == dot_op_id )
		{
			// lhs of dot-op stays the same
			if( i->children[0].children[0].value.id() == identifier_id )
			{
				string lhs = strip_symbol( string( i->children[0].children[0].value.begin(), i->children[0].children[0].value.end() ) );
				generate_line_num( i->children[0].children.begin(), is );
				is.push( Instruction( op_push , DevaObject( lhs, sym_unknown ) ) );
			}
			else
			{
				// don't pass 'self' (i) as parent, keep the parent the root for the
				// whole 'dot-op chain' (e.g. in 'a.b.c.d()', the parent stays as
				// the parent of a)
				generate_IL_for_node( i->children[0].children.begin(), is, parent, 0 );
			}

			// turn the rhs into a string
			string rhs = strip_symbol( string( i->children[0].children[1].value.begin(), i->children[0].children[1].value.end() ) );
			generate_line_num( i->children[0].children.begin()+1, is );
			is.push( Instruction( op_push , DevaObject( "", rhs ) ) );

			// rhs of dot-op: check for fcn call here too (for 'a.b()' etc)!!
			// if the first child is an arg_list_exp, it's a fcn call
			if( i->children[0].children[1].children.size() > 0 && i->children[0].children[1].children[0].value.id() == arg_list_exp_id )
			{
				// first walk the children, in reverse order
				reverse_walk_children( i->children[0].children.begin() + 1, is );

				// then generate the IL for this node (back-patching the return
				// address etc.)
				gen_IL_identifier( i->children[0].children.begin() + 1, is, parent, true, child_num );
			}
			// check for a table lookup (for 'a.b[0]' etc)
			// if the first child is a key_exp, it's a table lookup
			else if( i->children[0].children[1].children.size() > 0 && i->children[0].children[1].children[0].value.id() == key_exp_id )
			{
				// first generate the tbl_load instruction for the dot-op
				generate_line_num( i->children.begin(), is );
				is.push( Instruction( op_tbl_load ) );
				// then do the key op 
				reverse_walk_children( i->children[0].children[1].children.begin(), is );
			}

			// rhs of assignment op
			// walk the rhs
			generate_IL_for_node( i->children.begin() + 1, is, i, 1 );

			// add the table store op (for the assignment op)
			generate_line_num( i->children.begin(), is );
			is.push( Instruction( op_tbl_store ) );
		}
		else
		{
			walk_children( i, is );
			gen_IL_assignment_op( i, is );
		}
	}
	// add assignment op (+=)
	else if( i->value.id() == parser_id( add_assignment_op_id ) )
	{
		walk_children( i, is );
		gen_IL_add_assignment_op( i, is );
	}
	// sub assignment op (-=)
	else if( i->value.id() == parser_id( sub_assignment_op_id ) )
	{
		walk_children( i, is );
		gen_IL_sub_assignment_op( i, is );
	}
	// mul assignment op (*=)
	else if( i->value.id() == parser_id( mul_assignment_op_id ) )
	{
		walk_children( i, is );
		gen_IL_mul_assignment_op( i, is );
	}
	// div assignment op (/=)
	else if( i->value.id() == parser_id( div_assignment_op_id ) )
	{
		walk_children( i, is );
		gen_IL_div_assignment_op( i, is );
	}
	// mod assignment op (%=)
	else if( i->value.id() == parser_id( mod_assignment_op_id ) )
	{
		walk_children( i, is );
		gen_IL_mod_assignment_op( i, is );
	}
	// logical op
	else if( i->value.id() == parser_id( logical_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		walk_children( i, is );
		gen_IL_logical_op( i, is );
	}
	// relational op
	else if( i->value.id() == parser_id( relational_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		walk_children( i, is );
		gen_IL_relational_op( i, is );
	}
	// mult_op
	else if( i->value.id() == parser_id( mult_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		walk_children( i, is );
		gen_IL_mult_op( i, is );
	}
	// add_op
	else if( i->value.id() == parser_id( add_op_id ) )
	{
		// either the two sides or the two sides and a semi-colon
		walk_children( i, is );
		gen_IL_add_op( i, is );
	}
	// unary_op
	else if( i->value.id() == parser_id( unary_op_id ) )
	{
		// operand and possibly semi-colon
		walk_children( i, is );
		gen_IL_unary_op( i, is );
	}
	// dot op
	else if( i->value.id() == parser_id( dot_op_id ) )
	{
		// operands (lhs & rhs) and possibly semi-colon
		
		// if this is ultimately a fcn (method) call, we need to push the
		// arguments to the call onto the stack FIRST. so, we need to walk the
		// AST all the way down until we find the arg_list_exp, generate code
		// for it, and then start over and walk down generating the code for the
		// dot_ops (vector_loads)...
		walk_children_for_method_call( i, is );

		if( i->children[0].value.id() == identifier_id )
		{
			// if the lhs is an identifier with a key_exp (vec/map) then generate a
			// table load
			if( i->children[0].children.size() > 0 && i->children[0].children[0].value.id() == key_exp_id )
			{
				// push the identifier
				string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
				generate_line_num( i->children.begin(), is );
				is.push( Instruction( op_push , DevaObject( name, sym_unknown ) ) );
				// push the key exp
				reverse_walk_children( i->children[0].children.begin(), is );
				is.push( Instruction( op_tbl_load ) );
			}
			// lhs stays the same
			else
			{
				string lhs = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
				generate_line_num( i->children.begin(), is );
				is.push( Instruction( op_push , DevaObject( lhs, sym_unknown ) ) );
			}
		}
		else
			// don't pass 'self' (i) as parent, keep the parent the root for the
			// whole 'dot-op chain' (e.g. in 'a.b.c.d()', the parent stays as
			// the parent of a)
			generate_IL_for_node( i->children.begin(), is, parent, 0 );

		// turn the rhs into a string
		string rhs = strip_symbol( string( i->children[1].value.begin(), i->children[1].value.end() ) );
		generate_line_num( i->children.begin()+1, is );
		is.push( Instruction( op_push , DevaObject( "", rhs ) ) );

		// check for fcn call here too (for 'a.b()' etc)!!
		// if the first child is an arg_list_exp, it's a fcn call
		if( i->children[1].children.size() > 0 && i->children[1].children[0].value.id() == arg_list_exp_id )
		{
			// this generates a tbl_load instruction with a flag indicating it's
			// for a method call (so it can push the 'self' arg too)
			gen_IL_dot_op( i, is, true );

			// we already walked the children and generated the placeholder for
			// the jump...
			// so just generate the IL for this node, which will back-patch the
			// jump placeholder
			gen_IL_identifier( i->children.begin() + 1, is, parent, true, child_num );
		}
		// check for a table lookup (for 'a.b[0]' etc)
		// if the first child is a key_exp, it's a table lookup
		else if( i->children[1].children.size() > 0 && i->children[1].children[0].value.id() == key_exp_id )
		{
			// first generate the tbl_load instruction for the dot-op
			gen_IL_dot_op( i, is );
			// then do the key op 
			reverse_walk_children( i->children[1].children.begin(), is );
			gen_IL_key_exp( i->children[1].children.begin(), is );
		}
		else
			// this generates a tbl_load instruction
			gen_IL_dot_op( i, is );
	}
	// paren ops
	else if( i->value.id() == parser_id( open_paren_op_id )
		  || i->value.id() == parser_id( close_paren_op_id ) )
	{
		walk_children( i, is );
		gen_IL_paren_op( i, is );
	}
	// bracket ops
	else if( i->value.id() == parser_id( open_bracket_op_id ) 
			|| i->value.id() == parser_id( close_bracket_op_id ) )
	{
		walk_children( i, is );
		gen_IL_bracket_op( i, is );
	}
	// arg list exp
	else if( i->value.id() == parser_id( arg_list_exp_id ) )
	{
		pre_gen_IL_arg_list_exp( i, is );
		reverse_walk_children( i, is );
		gen_IL_arg_list_exp( i, is );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
		gen_IL_arg_list_decl( i, is );
	}
//	else if( i->value.id() == parser_id( arg_id ) )
//	{
//		// do nothing, handled by arg_list_decl
//	}
	// key exp
	else if( i->value.id() == parser_id( key_exp_id ) )
	{
		reverse_walk_children( i, is );
		gen_IL_key_exp( i, is );
	}
	// const decl
	else if( i->value.id() == parser_id( const_decl_id ) )
	{
		walk_children( i, is );
		gen_IL_const_decl( i, is );
	}
	// constant (keyword 'const')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		walk_children( i, is );
		gen_IL_constant( i, is );
	}
	// local decl
	else if( i->value.id() == parser_id( local_decl_id ) )
	{
		walk_children( i, is );
	}
	// local (keyword 'local')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		walk_children( i, is );
	}
	// translation unit
	else if( i->value.id() == parser_id( translation_unit_id ) )
	{
		walk_children( i, is );
		gen_IL_translation_unit( i, is );
	}
	// compound statement
	else if( i->value.id() == parser_id( compound_statement_id ) )
	{
		pre_gen_IL_compound_statement( i, is, parent );
		walk_children( i, is );
		gen_IL_compound_statement( i, is, parent );
	}
	// break statement
	else if( i->value.id() == parser_id( break_statement_id ) )
	{
		walk_children( i, is );
		gen_IL_break_statement( i, is );
	}
	// continue statement
	else if( i->value.id() == parser_id( continue_statement_id ) )
	{
		walk_children( i, is );
		gen_IL_continue_statement( i, is );
	}
	// return statement
	else if( i->value.id() == parser_id( return_statement_id ) )
	{
		walk_children( i, is );
		gen_IL_return_statement( i, is );
	}
}


// write bytecode to disk
bool WriteByteCode( char const* filename, unsigned char* code, size_t code_length )
{
	// open the file for writing
	ofstream file;
	file.open( filename, ios::binary );
	if( file.fail() )
		return false;

	// write the header
	FileHeader hdr;
	file << hdr.deva;
	file << '\0';
	file << hdr.ver;
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';

	// write the code
	file.write( (const char*)code, code_length );

	// close the file
	file.close();

	return true;
}

// write the bytecode to disk
bool GenerateByteCode( char const* filename, InstructionStream & is )
{
	// open the file for writing
	ofstream file;
	file.open( filename, ios::binary );
	if( file.fail() )
		return false;

	// write the header
	FileHeader hdr;
	file << hdr.deva;
	file << '\0';
	file << hdr.ver;
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';

	// write the instructions
	for( int i = 0; i < is.size(); ++i )
	{
		Instruction inst = is[i];
		// write the byte for the opcode
		file << (unsigned char)inst.op;
		// for each argument:
		for( int j = 0; j < inst.args.size(); ++j )
		{
			// write the byte for the type
			file << (unsigned char)inst.args[j].Type();
			// write the name
			file.write( inst.args[j].name.c_str(), inst.args[j].name.length() );
			file << '\0';
			// write the value
			switch( inst.args[j].Type() )
			{
				case sym_number:
					file.write( (char*)&(inst.args[j].num_val), sizeof( double ) );
					break;
				case sym_string:
					// variable length, null-terminated
					file.write( inst.args[j].str_val, strlen( inst.args[j].str_val ) );
					file << '\0';
					break;
				case sym_boolean:
					{
					// 32 bits
					long val = (long)inst.args[j].bool_val;
					file.write( (char*)&val, sizeof( long ) );
					break;
					}
				case sym_map:
					// TODO: implement
					//is.args[j].map_val
					break;
				case sym_vector:
					// TODO: implement
					//is.args[j].vec_val
					break;
				case sym_address:
				case sym_size:
					file.write( (char*)&(inst.args[j].sz_val), sizeof( size_t ) );
					break;
				case sym_function_call:
					// TODO: can this happen??
					break;
				case sym_null:
					{
					// 32 bits
					long val = 0;
					file.write( (char*)&val, sizeof( long ) );
					break;
					}
				case sym_unknown:
					// nothing to do, no known value/type
					break;
				default:
					// TODO: throw error
					break;
			}
		}
		// write the instruction terminating byte
		file << (unsigned char)255;
	}
	file.close();

	return true;
}

// generate and return bytecode. length of the generated bytecode is returned in
// the out parameter 'length'
unsigned char* GenerateByteCode( InstructionStream & is, size_t & length )
{
	ostringstream oss;

	// write the instructions
	for( int i = 0; i < is.size(); ++i )
	{
		Instruction inst = is[i];
		// write the byte for the opcode
		oss << (unsigned char)inst.op;
		// for each argument:
		for( int j = 0; j < inst.args.size(); ++j )
		{
			// write the byte for the type
			oss << (unsigned char)inst.args[j].Type();
			// write the name
			oss.write( inst.args[j].name.c_str(), inst.args[j].name.length() );
			oss << '\0';
			// write the value
			switch( inst.args[j].Type() )
			{
				case sym_number:
					oss.write( (char*)&(inst.args[j].num_val), sizeof( double ) );
					break;
				case sym_string:
					// variable length, null-terminated
					oss.write( inst.args[j].str_val, strlen( inst.args[j].str_val ) );
					oss << '\0';
					break;
				case sym_boolean:
					{
					// 32 bits
					long val = (long)inst.args[j].bool_val;
					oss.write( (char*)&val, sizeof( long ) );
					break;
					}
				case sym_map:
					// TODO: implement
					//is.args[j].map_val
					break;
				case sym_vector:
					// TODO: implement
					//is.args[j].vec_val
					break;
				case sym_address:
				case sym_size:
					oss.write( (char*)&(inst.args[j].sz_val), sizeof( size_t ) );
					break;
				case sym_function_call:
					// TODO: can this happen??
					break;
				case sym_null:
					{
					// 32 bits
					long val = 0;
					oss.write( (char*)&val, sizeof( long ) );
					break;
					}
				case sym_unknown:
					// nothing to do, no known value/type
					break;
				default:
					// TODO: throw error
					break;
			}
		}
		// write the instruction terminating byte
		oss << (unsigned char)255;
	}
	string s = oss.str();
	size_t len = s.length();
	unsigned char* buf = new unsigned char[len];
	s.copy( (char*)buf, len );
	length = len;
	return buf;
}

// de-compile a .dvc file and dump the IL to stdout
bool DeCompileFile( char const* filename )
{
	// open the file for reading
	ifstream file;
	file.open( filename, ios::binary );
	if( file.fail() )
		return false;

	cout << "deva IL:" << endl;

	// read the header
	char deva[5] = {0};
	char ver[6] = {0};
	file.read( deva, 5 );
	//cout << deva << endl;
	if( strcmp( deva, "deva") != 0 )
	{
		cout << "Invalid .dvc file: header missing 'deva' tag" << endl;
		return false;
	}
	file.read( ver, 6 );
	//cout << ver << endl;
	if( strcmp( ver, "1.0.0" ) != 0 )
	{
		cout << "Invalid .dvc version number " << ver << endl;
		return false;
	}
	char pad[6] = {0};
	file.read( pad, 5 );
	//cout << pad << endl;
	if( pad[0] != 0 || pad[1] != 0 || pad[2] != 0 || pad[3] != 0 || pad[4] != 0 )
	{
		cout << "Invalid .dvc file: malformed header after version number" << endl;
		return false;
	}

	// read the instructions
	char* name = new char[256];
	while( !file.eof() )
	{
		double num_val;
		char str_val[256] = {0};
		long bool_val;
		size_t sz_val;
		// read the byte for the opcode
		unsigned char op;
		file.read( (char*)&op, 1 );
		// avoid re-emitting the final opcode
		if( file.eof() )
			break;
		Instruction inst( (Opcode)op );
		// for each argument:
		unsigned char type;
		file.read( (char*)&type, 1 );
		while( type != sym_end )
		{
			// read the name of the arg
			memset( name, 0, 256 );
			file.getline( name, 256, '\0' );
			// read the value
			switch( (SymbolType)type )
			{
				case sym_number:
					{
					file.read( (char*)&num_val, sizeof( double ) );
					DevaObject ob( name, num_val );
					inst.args.push_back( ob );
					break;
					}
				case sym_string:
					{
					// variable length, null-terminated
					// TODO: fix. strings can be any length!
					file.getline( str_val, 256, '\0' );
					DevaObject ob( name, string( str_val ) );
					inst.args.push_back( ob );
					break;
					}
				case sym_boolean:
					{
					file.read( (char*)&bool_val, sizeof( long ) );
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
				case sym_address:
				case sym_size:
					{
					file.read( (char*)&sz_val, sizeof( size_t ) );
					DevaObject ob( name, sz_val, true );
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
					// 32 bits
					long n = -1;
					file.read( (char*)&n, sizeof( long ) );
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
			memset( name, 0, 256 );
			
			// read the type of the next arg
			// default to sym_end to drop out of loop if we can't read a byte
			type = (unsigned char)sym_end;
			file.read( (char*)&type, sizeof( type ) );
		}
		cout << inst.op << " : ";
		// dump args (vector of DevaObjects) too (need >> op for Objects)
		for( vector<DevaObject>::iterator j = inst.args.begin(); j != inst.args.end(); ++j )
			cout << *j << " ; ";
		cout << endl;
	}
	delete [] name;
	return true;
}


// parse, check semantics, generate IL and generate (and write) bytecode for a .dv file:
bool CompileAndWriteFile( const char* filename, bool debug_info /*= true*/ )
{
	// check to see if the .dvc (output) file exists, and is newer than the .dv (input ) file
	string output( filename );
	size_t pos = output.rfind( "." );
	if( pos != string::npos )
	{
		output.erase( pos );
	}
	output += ".dvc";

	struct stat in_statbuf;
	struct stat out_statbuf;

	// if we can't open the .dvc file, continue on
	if( stat( output.c_str(), &out_statbuf ) != -1 )
	{
		if( stat( filename, &in_statbuf ) != -1 ) 
		{
			// if the output is newer than the input, nothing to do
			if( out_statbuf.st_mtime > in_statbuf.st_mtime )
				return true;
		}
		// .dvc file exists, but no .dv file
		else
			return true;
	}

	tree_parse_info<iterator_t, factory_t> info;

	// open input file
	ifstream file;
	file.open( filename );
	if( !file.is_open() )
	{
		ostringstream oss;
		oss << "Error opening " << filename << endl;
		throw DevaRuntimeException( oss.str().c_str() );
	}
	// parse the file
	info = ParseFile( string( filename ), file );
	// close the file
	file.close();

	// failed to fully parse the input?
	if( !info.full )
		return false;

	// check the semantics of the AST
	if( !CheckSemantics( info ) )
		return false;

	// generate IL
	InstructionStream inst;
	if( !GenerateIL( info, inst, debug_info ) )
		return false;

	// TODO: optimize IL (???)

	// generate final IL bytecode
	if( !GenerateByteCode( output.c_str(), inst ) )
	{
		// only returns false on failure to open the file
		ostringstream oss;
		oss << "Error opening the byte-code file: " << output << " for output." << endl;
		throw DevaRuntimeException( oss.str().c_str() );
	}

	return true;
}

// parse, check semantics, generate IL and generate bytecode for an input file
// containing deva code, returns the byte code (which must be freed by the
// caller) or NULL on failure
// length of the bytecode buffer is returned in the out parameter 'length'
unsigned char* CompileFile( char const* filename, size_t & length, bool debug_info /*= true*/ )
{
	tree_parse_info<iterator_t, factory_t> info;

	// open input file
	ifstream file;
	file.open( filename );
	if( !file.is_open() )
	{
		ostringstream oss;
		oss << "Error opening " << filename << endl;
		throw DevaRuntimeException( oss.str().c_str() );
	}
	// parse the file
	info = ParseFile( string( filename ), file );
	// close the file
	file.close();

	// failed to fully parse the input?
	if( !info.full )
		return false;

	// check the semantics of the AST
	if( !CheckSemantics( info ) )
		return false;

	// generate IL
	InstructionStream inst;
	if( !GenerateIL( info, inst, debug_info ) )
		return false;

	// TODO: optimize IL (???)

	// generate final IL bytecode
	return GenerateByteCode( inst, length );
}

// parse, check semantics, generate IL and generate bytecode for an input string
// containing deva code, returns the byte code (which must be freed by the
// caller) or NULL on failure
unsigned char* CompileText( char const* const input, long input_len, bool debug_info /*= true*/ )
{
	tree_parse_info<iterator_t, factory_t> info;

	// parse the file
	info = ParseText( string( "<TEXT>" ), input, input_len );

	// failed to fully parse the input?
	if( !info.full )
		return NULL;

	// check the semantics of the AST
	if( !CheckSemantics( info ) )
		return NULL;

	// generate IL
	InstructionStream inst;
	if( !GenerateIL( info, inst, debug_info ) )
		return NULL;

	// TODO: optimize IL (???)

	// generate final IL bytecode
	size_t code_length;
	return GenerateByteCode( inst, code_length );
}
