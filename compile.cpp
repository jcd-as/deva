// compile.cpp
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * 'a["b"] = {};' is disallowed. enable, or disallow in syntax??
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

	// TODO: ensure symbol is not more than 255 characters ??
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
tree_parse_info<iterator_t, factory_t> ParseText( string filename, const char* const input )
{
	// create our grammar parser
	DevaGrammar deva_p;

	// create the position iterator for the parser
//	iterator_t begin( input, input + strlen( input ), "<TEXT>" );
	iterator_t begin( input, input + strlen( input ), filename );
	iterator_t end;

	// create our initial (global) scope
	SymbolTable* globals = new SymbolTable();
	pair<int, SymbolTable*> sym( 0, globals );
	scope_bldr.push_back( sym );
	scopes[sym.first] = sym.second;

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
	tree_parse_info<iterator_t, factory_t> ret = ParseText( filename, buf );

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
	else if( i->value.id() == parser_id( import_statement_id ) )
	{
		walk_children( i );
		check_import_s( i );
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		// can have arg_list & semi-colon
        assert( i->children.size() < 3 );
		walk_children( i );
		check_identifier( i );
	}
	// module_name
	else if( i->value.id() == parser_id( module_name_id ) )
	{
		// can have arg_list & semi-colon
        assert( i->children.size() < 3 );
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
		emit_error( i->value.value(), "Semantic error: Encountered unknown node type" );
		throw DevaSemanticException( "invalid node type", i->value.value() );
    }
}


void generate_IL_for_node( iter_t const & i, InstructionStream & is, iter_t const & parent );
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
			is.push( Instruction( op_line_num, DevaObject( "", ni.file ), DevaObject( "", (size_t)0) ) );
		}

        // walk the tree, generating the IL for each node
        iter_t i = info.trees.begin();
        generate_IL_for_node( i, is, i );

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
	for( int c = 0; c < i->children.size(); c++ )
	{
		generate_IL_for_node( i->children.begin() + c, is, i );
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

void generate_IL_for_node( iter_t const & i, InstructionStream & is, iter_t const & parent )
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

		// create a function at this loc
		string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
		// write the current *file* offset, not instruction stream!
		generate_line_num( i, is );
		is.push( Instruction( op_defun, DevaObject( name, is.Offset() ) ) );

		// we need to generate an enter instruction to create the
		// proper scope for the fcn and its arguments (the 'leave' instruction 
		// isn't needed, as the return statement at the end of the 
		// function takes care of this)
		if( (i->children.begin()+2)->value.id() != compound_statement_id )
		{
			generate_line_num( i->children.begin()+2, is );
			is.push( Instruction( op_enter ) );
		}

		// second child is the arg_list, process it
		generate_IL_for_node( i->children.begin() + 1, is, i );

		// third child is statement|compound_statement, process it
		generate_IL_for_node( i->children.begin() + 2, is, i );

		// if no return statement was generated, we need to generate one
		bool returned = false;
		iter_t iter = i->children.begin() + 2;
		if( iter->value.id() == return_statement_id )
			returned = true;
		else
			returned = walk_looking_for_return( iter );
		if( !returned )
		{
			// all fcns return *something*
			generate_line_num( iter, is );
			is.push( Instruction( op_push, DevaObject( "", sym_null ) ) );
			is.push( Instruction( op_return ) );
		}
	}
	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		// pre-gen stores the start value, for the jump-to-start
		pre_gen_IL_while_s( i, is );
		// first child has the relational op stuff, walk it
		generate_IL_for_node( i->children.begin(), is, i );
		// gen_IL adds the placeholder for the jmpf
		gen_IL_while_s( i, is );
		// second child has the statement|compound_statement, walk it
		generate_IL_for_node( i->children.begin() + 1, is, i );
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
			generate_IL_for_node( i->children.begin() + 2, is, i );
		else if( i->children.size() == 4 )
			generate_IL_for_node( i->children.begin() + 3, is, i );
		else
			throw DevaSemanticException( "invalid for loop", i->value.value() );

		// gen_IL adds the placeholder for the jmpf
		gen_IL_for_s( i, is );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		// first child has the conditional, walk it
		generate_IL_for_node( i->children.begin(), is, i );
		// generate the jump-placeholder
		pre_gen_IL_if_s( i, is );
		// second child has the statement|compound_statement, walk it
		generate_IL_for_node( i->children.begin() + 1, is, i );

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
			generate_IL_for_node( i->children.begin() + 2, is, i );

			// ...and then back-patch the 'else' jump to jump to here
			gen_IL_else_s( i, is );
		}
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		// always a child of an 'if', which handles this, see above
		// so just walk the children
		generate_IL_for_node( i->children.begin(), is, i );
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
			// first walk the children
			walk_children( i, is );

			// then generate the IL for this node
			gen_IL_identifier( i, is, parent, false );
		}
		// if the id is followed by []'s it is either a vector or map look-up
		else if( i->children[0].value.id() == key_exp_id )
		{
			// first add the push of the name onto the stack
			generate_line_num( i->children.begin(), is );
			is.push( Instruction( op_push , DevaObject( name, sym_unknown ) ) );

			// then walk the children - key_exp will push it's children (the key) and
			// the key-lookup op
			walk_children( i, is );
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
		// if the lhs is an identifier with a key_exp (vec/map) then generate a
		// vector store
		if( i->children[0].value.id() == identifier_id && i->children[0].children.size() > 0 &&
			 i->children[0].children[0].value.id() == key_exp_id )
		{
			// push the identifier
			string name = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
			generate_line_num( i->children.begin(), is );
			is.push( Instruction( op_push , DevaObject( name, sym_unknown ) ) );
			// push the key exp
			walk_children( i->children[0].children.begin(), is );
			
			// if the rhs is a new_vec or new_map op, we need to gen code for it
			// *last*
			// TODO: currently disallowed (the following generates bad code!)
			if( i->children[1].value.id() == vec_op_id || i->children[1].value.id() == map_op_id )
			{
				NodeInfo ni = ((NodeInfo)(i->value.value()));
				throw DevaSemanticException( "A new vector or map can only be assigned into a simple variable.", ni );
//				// add the vector store op
//				is.push( Instruction( op_vec_store ) );
//
//				// *then* walk the rhs
//				generate_IL_for_node( i->children.begin() + 1, is, i );
			}
			else
			{
				// walk the rhs
				generate_IL_for_node( i->children.begin() + 1, is, i );

				// add the vector store op
				generate_line_num( i->children.begin()+1, is );
				is.push( Instruction( op_vec_store ) );
			}
		}
		// a dot-op on the lhs also indicates a vector store instead of load, as
		// long as it is not a function call.
		else if ( i->children[0].value.id() == dot_op_id )
		{
            // TODO: validate: when the dot op IS a fcn call, etc

            // lhs of dot-op stays the same
            if( i->children[0].children[0].value.id() == identifier_id )
            {
                string lhs = strip_symbol( string( i->children[0].children[0].value.begin(), i->children[0].children[0].value.end() ) );
				generate_line_num( i->children[0].children.begin(), is );
                is.push( Instruction( op_push , DevaObject( lhs, sym_unknown ) ) );
            }
            else
                // don't pass 'self' (i) as parent, keep the parent the root for the
                // whole 'dot-op chain' (e.g. in 'a.b.c.d()', the parent stays as
                // the parent of a)
                generate_IL_for_node( i->children[0].children.begin(), is, parent );

            // turn the rhs into a string
            string rhs = strip_symbol( string( i->children[0].children[1].value.begin(), i->children[0].children[1].value.end() ) );
			generate_line_num( i->children[0].children.begin()+1, is );
            is.push( Instruction( op_push , DevaObject( "", rhs ) ) );

            // rhs of dot-op: check for fcn call here too (for 'a.b()' etc)!!
            // if the first child is an arg_list_exp, it's a fcn call
            if( i->children[0].children[1].children.size() > 0 && i->children[0].children[1].children[0].value.id() == arg_list_exp_id )
            {
                // first walk the children
                walk_children( i->children[0].children.begin() + 1, is );

                // then generate the IL for this node
                gen_IL_identifier( i->children[0].children.begin() + 1, is, parent, true );
            }

            // rhs of assignment op
            // walk the rhs
            generate_IL_for_node( i->children.begin() + 1, is, i );

            // add the vector store op
			generate_line_num( i->children.begin(), is );
            is.push( Instruction( op_vec_store ) );
		}
		else
		{
			walk_children( i, is );
			gen_IL_assignment_op( i, is );
		}
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
		
		// lhs stays the same
		if( i->children[0].value.id() == identifier_id )
		{
			string lhs = strip_symbol( string( i->children[0].value.begin(), i->children[0].value.end() ) );
			generate_line_num( i->children.begin(), is );
			is.push( Instruction( op_push , DevaObject( lhs, sym_unknown ) ) );
		}
		else
			// don't pass 'self' (i) as parent, keep the parent the root for the
			// whole 'dot-op chain' (e.g. in 'a.b.c.d()', the parent stays as
			// the parent of a)
			generate_IL_for_node( i->children.begin(), is, parent );

		// turn the rhs into a string
		string rhs = strip_symbol( string( i->children[1].value.begin(), i->children[1].value.end() ) );
		generate_line_num( i->children.begin()+1, is );
		is.push( Instruction( op_push , DevaObject( "", rhs ) ) );

		gen_IL_dot_op( i, is );

		// check for fcn call here too (for 'a.b()' etc)!!
		// if the first child is an arg_list_exp, it's a fcn call
		if( i->children[1].children.size() > 0 && i->children[1].children[0].value.id() == arg_list_exp_id )
		{
			// first walk the children
			walk_children( i->children.begin() + 1, is );

			// then generate the IL for this node
			gen_IL_identifier( i->children.begin() + 1, is, parent, true );
		}
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
		walk_children( i, is );
		gen_IL_arg_list_exp( i, is );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
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
				case sym_function:
					file.write( (char*)&(inst.args[j].func_offset), sizeof( size_t ) );
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
	return true;
}

// generate and return bytecode
unsigned char* GenerateByteCode( InstructionStream & is )
{
	// TODO: implement
	//
	// open the file for writing
//	ofstream file;
//	file.open( filename, ios::binary );
//	if( file.fail() )
//		return false;

	ostringstream oss;

	// write the header
	FileHeader hdr;
	oss << hdr.deva;
	oss << '\0';
	oss << hdr.ver;
	oss << '\0';
	oss << '\0';
	oss << '\0';
	oss << '\0';
	oss << '\0';
	oss << '\0';

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
				case sym_function:
					oss.write( (char*)&(inst.args[j].func_offset), sizeof( size_t ) );
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
		size_t func_offset;
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
				case sym_function:
					{
					file.read( (char*)&func_offset, sizeof( size_t ) );
					DevaObject ob( name, func_offset );
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
			cout << *j;
		cout << endl;
	}
	delete [] name;
	return true;
}


// parse, check semantics, generate IL and generate bytecode for a .dv file:
bool CompileFile( const char* filename, bool debug_info /*= true*/ )
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
		oss << "error opening " << filename << endl;
		throw DevaRuntimeException( oss.str().c_str() );
	}
	// parse the file
	info = ParseFile( filename, file );
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
		return false;
}

// parse, check semantics, generate IL and generate bytecode for an input string
// containing deva code, returns the byte code (which must be freed by the
// caller) or NULL on failure
unsigned char* CompileText( char const* const input, bool debug_info /*= true*/ )
{
	tree_parse_info<iterator_t, factory_t> info;

	// parse the file
	info = ParseText( string( "<TEXT>" ), input );

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
	return GenerateByteCode( inst );
}
