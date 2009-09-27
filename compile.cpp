// compile.cpp
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * change asserts into (non-fatal) errors
// * encode location (line number) into bytecode via "line_num" opcodes
// * maps & vectors, including the dot operator and 'for' loops
// * semantic checking functions for all valid node types that can have children

#include <iomanip>

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

	// TODO: ensure symbol is not more than 255 characters!
//	if( s.length() > 255 )
//		throw new SyntaxError
	
	// don't add keywords
	if( is_keyword( s ) )
		return;
	// only add to the current scope if it can't be found in a parent scope
	if( find_identifier_in_parent_scopes( s, scope_bldr.back().second, scopes ) )
		return;

	// the current scope is the top of the scope_bldr stack
	scope_bldr.back().second->operator[](s) = new SymbolInfo( sym_unknown );
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


// parse a deva file
// returns the AST tree
tree_parse_info<iterator_t, factory_t> ParseFile( string filename, istream & file )
{
	// get the length of the file
	file.seekg( 0, ios::end );
	int length = file.tellg();
	file.seekg( 0, ios::beg );
	// allocate memory to read the file into
	char* buf = new char[length];
	// read the file
	file.read( buf, length );

	// create our grammar parser
	DevaGrammar deva_p;

	// create the position iterator for the parser
	iterator_t begin( buf, buf+length, filename );
	iterator_t end;

	// create our initial (global) scope
	SymbolTable* globals = new SymbolTable();
	pair<int, SymbolTable*> sym( 0, globals );
	scope_bldr.push_back( sym );
	////////////////////////
	scopes[sym.first] = sym.second;
	////////////////////////

	// parse 
	//
	tree_parse_info<iterator_t, factory_t> info;
	info = ast_parse<factory_t>( begin, end, deva_p, (space_p | comment_p( "#" )) );

	// free the buffer with the code in it
	delete[] buf;

	return info;
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

//    cout << "In eval_node. i->value = " << s << 
//        " i->children.size() = " << i->children.size() << endl;

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
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		// can have arg_list & semi-colon
        assert( i->children.size() < 3 );
		walk_children( i );
		check_identifier( i );
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
        assert( i->children.size() == 0 );
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
	// constant (keyword 'const')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		walk_children( i );
		check_constant( i );
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
		// TODO: pass actual filename
		emit_error( i->value.value(), "Semantic error: Encountered unknown node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
    }
}


void generate_IL_for_node( iter_t const & i, InstructionStream & is );
// generate IL stream
bool GenerateIL( tree_parse_info<iterator_t, factory_t> info, InstructionStream & is )
{
	// walk the tree, generating the IL for each node
	iter_t i = info.trees.begin();
	generate_IL_for_node( i, is );

	// last node always a halt
	is.push( Instruction( op_halt ) );

	// TODO: ever returns false??
	return true;
}

void walk_children( iter_t const & i, InstructionStream & is )
{
//	cout << "in scope " << i->value.value().scope << ", at line: " << i->value.value().line << endl;
//	string s( i->value.begin(), i->value.end() );
//	cout << strip_symbol( s ) << endl;

	// walk children
	for( int c = 0; c < i->children.size(); c++ )
	{
		generate_IL_for_node( i->children.begin() + c, is );
	}
}

void generate_IL_for_node( iter_t const & i, InstructionStream & is )
{
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
//		gen_IL_func( i, is );

		// create a function at this loc
		string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
		is.push( Instruction( op_defun, DevaObject( name, (double)is.size() ) ) );

		// second child is the arg_list, process it
		generate_IL_for_node( i->children.begin() + 1, is );

		// third child is statement|compound_statement, process it
		generate_IL_for_node( i->children.begin() + 2, is );
	}
	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		// pre-gen stores the start value, for the jump-to-start
		pre_gen_IL_while_s( i, is );
		// first child has the relational op stuff, walk it
		generate_IL_for_node( i->children.begin(), is );
		// gen_IL adds the placeholder for the jmpf
		gen_IL_while_s( i, is );
		// second child has the statement|compound_statement, walk it
		generate_IL_for_node( i->children.begin() + 1, is );
		// post-gen-IL will to the back-patching and add the jump to start
		post_gen_IL_while_s( i, is );
	}
	// for_s (keyword 'for')
	else if( i->value.id() == parser_id( for_s_id ) )
	{
		// pre-gen stores the start value, for the jump-to-start
//		pre_gen_IL_for_s( i, is );
//		// first child has the relational op stuff, walk it
//		generate_IL_for_node( i->children.begin(), is );
//		// gen_IL adds the placeholder for the jmpf
//		gen_IL_for_s( i, is );
//		// second child has the statement|compound_statement, walk it
//		generate_IL_for_node( i->children.begin() + 1, is );
//		// post-gen-IL will to the back-patching and add the jump to start
//		post_gen_IL_for_s( i, is );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		// first child has the conditional, walk it
		generate_IL_for_node( i->children.begin(), is );
		// generate the jump-placeholder
		pre_gen_IL_if_s( i, is );
		// second child has the statement|compound_statement, walk it
		generate_IL_for_node( i->children.begin() + 1, is );

		// back-patch the jump-placeholder
		gen_IL_if_s( i, is );

		// third child, if any, has the else clause
		if( i->children.size() == 3 )
		{

			// generate the jump-placeholder...
			pre_gen_IL_else_s( i, is );
			// ...then walk it...
			generate_IL_for_node( i->children.begin() + 2, is );
			// ...and then back-patch the jump
			gen_IL_else_s( i, is );
		}
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		// always a child of an 'if', which handles this, see above
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		// can have arg_list & semi-colon
		walk_children( i, is );
		gen_IL_identifier( i, is );
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
		walk_children( i, is );
		gen_IL_map_op( i, is );
	}
	// vector construction op
	else if( i->value.id() == parser_id( vec_op_id ) )
	{
		walk_children( i, is );
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
        // either the two sides or the two sides and a semi-colon
		walk_children( i, is );
		gen_IL_assignment_op( i, is );
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
		walk_children( i, is );
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
		walk_children( i, is );
		gen_IL_arg_list_exp( i, is );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
//		walk_children( i, is );
		gen_IL_arg_list_decl( i, is );
	}
	// key exp
	else if( i->value.id() == parser_id( key_exp_id ) )
	{
		walk_children( i, is );
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
	// translation unit
	else if( i->value.id() == parser_id( translation_unit_id ) )
	{
		walk_children( i, is );
		gen_IL_translation_unit( i, is );
	}
	// compound statement
	else if( i->value.id() == parser_id( compound_statement_id ) )
	{
		pre_gen_IL_compound_statement( i, is );
		walk_children( i, is );
		gen_IL_compound_statement( i, is );
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
			file << inst.args[j].name;
			file << '\0';
			// write the value
			switch( inst.args[j].Type() )
			{
				case sym_number:
					// 64 bits
					file << inst.args[j].num_val;
					break;
				case sym_string:
					// variable length, null-terminated
					// TODO: string table / data section, write pointer
					file << inst.args[j].str_val;
					file << '\0';
					break;
				case sym_boolean:
					// 32 bits
					file << (long)inst.args[j].bool_val;
					break;
				case sym_map:
					// TODO: implement
					//is.args[j].map_val
					break;
				case sym_vector:
					// TODO: implement
					//is.args[j].vec_val
					break;
				case sym_function:
					// TODO: can this happen??
					//is.args[j].func_val;
					break;
				case sym_function_call:
					// TODO: can this happen??
					break;
				case sym_null:
					// 32 bits
					file << (long)0;
					break;
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
	return true;
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
	//char pad[7] = "abcdef";
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
//		map<DevaObject, DevaObject>* map_val;
//		vector<DevaObject>* vec_val;
//		Function* func_val;
		// read the byte for the opcode
		unsigned char op;
		file.read( (char*)&op, 1 );
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
					// must use >> op to read, since it was used to write
					file >> num_val;
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
					// must use >> op to read, since it was used to write
					file >> bool_val;
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
				case sym_function:
					{
					// TODO: ???
//					DevaObject ob( name, sym_function );
//					inst.args.push_back( ob );
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
					file >> n;
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
			//type = 'x';
			// default to sym_end to drop out of loop if we can't read a byte
			type = (unsigned char)sym_end;
			file.read( (char*)&type, sizeof( type ) );
		}

		cout << inst.op << " : ";
		// dump args (vector of DevaObjects) too (need >> op for Objects)
		for( vector<DevaObject>::iterator j = inst.args.begin(); j != inst.args.end(); ++j )
			cout << *j;
		cout << endl;
	}
	delete [] name;
	return true;
}
