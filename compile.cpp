// compile.cpp
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * change asserts into (non-fatal) errors
// * semantic checking functions for all valid node types that can have children

#include "compile.h"
#include "semantics.h"

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
	// TODO: enter correct sym type, if discernable
	scope_bldr.back().second->operator[](s) = SymbolInfo( sym_unknown );
}

// emit an error message
void emit_error( NodeInfo ni, string filename, string msg )
{
	// format = filename:linenum: msg
	cout << filename << ":" << ni.line << ":" << " " << msg;
}


// compile/parse/check/code-gen functions
//////////////////////////////////////////////////////////////////////

// parse a deva file
// returns the AST tree
tree_parse_info<iterator_t, factory_t> ParseFile( ifstream & file )
{
	// get the length of the file
	file.seekg( 0, ios::end );
	int length = file.tellg();
	file.seekg( 0, ios::beg );
	// allocate memory to read the file into
	char* buf = new char[length];
	// read the file
	file.read( buf, length );
	// close the file
	file.close();

	// create our grammar parser
	DevaGrammar deva_p;

	// create the position iterator for the parser
	iterator_t begin( buf, buf+length );
	iterator_t end;

	// create our initial (global) scope
	SymbolTable* globals = new SymbolTable();
	pair<int, SymbolTable*> sym( 0, globals );
	scope_bldr.push_back( sym );

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
	catch( SemanticException & e )
	{
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
	string s = strip_symbol( string( i->value.begin(), i->value.end() ) );

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
    }
	// string
	else if( i->value.id() == parser_id( string_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		walk_children( i );
	}
	// boolean
	else if( i->value.id() == parser_id( boolean_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		walk_children( i );
	}
	// null
	else if( i->value.id() == parser_id( null_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		walk_children( i );
	}
	// constant (keyword 'const')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		walk_children( i );
	}
	// func (keyword 'def' for defining functions)
	else if( i->value.id() == parser_id( func_id ) )
	{
		// children: id, arg_list, compound_statement | statement
        //assert( i->children.size() == 3 );
		check_func( i );
		walk_children( i );
	}
	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		check_while_s( i );
		walk_children( i );
	}
	// for_s (keyword 'for')
	else if( i->value.id() == parser_id( for_s_id ) )
	{
		walk_children( i );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		walk_children( i );
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		walk_children( i );
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		// can have arg_list & semi-colon
        assert( i->children.size() < 3 );

		walk_children( i );
	}
	// in op ('in' keyword in for loops)
	else if( i->value.id() == parser_id( in_op_id ) )
	{
		walk_children( i );
	}
	// map construction op
	else if( i->value.id() == parser_id( map_op_id ) )
	{
        assert( i->children.size() == 0 );

		walk_children( i );
	}
	// vector construction op
	else if( i->value.id() == parser_id( vec_op_id ) )
	{
        assert( i->children.size() == 0 );

		walk_children( i );
	}
	// semicolon op
	else if( i->value.id() == parser_id( semicolon_op_id ) )
	{
        assert( i->children.size() == 0 );

		walk_children( i );
	}
	// assignment op
	else if( i->value.id() == parser_id( assignment_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		walk_children( i );
	}
	// logical op
	else if( i->value.id() == parser_id( logical_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		walk_children( i );
	}
	// relational op
	else if( i->value.id() == parser_id( relational_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		walk_children( i );
	}
	// mult_op
	else if( i->value.id() == parser_id( mult_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		walk_children( i );
	}
	// add_op
	else if( i->value.id() == parser_id( add_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		walk_children( i );
	}
	// unary_op
	else if( i->value.id() == parser_id( unary_op_id ) )
	{
		// operand and possibly semi-colon
        assert( i->children.size() == 1 || i->children.size() == 2 );

		walk_children( i );
	}
	// dot op
	else if( i->value.id() == parser_id( dot_op_id ) )
	{
		// operands (lhs & rhs) and possibly semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		walk_children( i );
	}
	// paren ops
	else if( i->value.id() == parser_id( open_paren_op_id )
		  || i->value.id() == parser_id( close_paren_op_id ) )
	{
        assert( i->children.size() == 0 );

		walk_children( i );
	}
	// bracket ops
	else if( i->value.id() == parser_id( open_bracket_op_id ) 
			|| i->value.id() == parser_id( close_bracket_op_id ) )
	{
        assert( i->children.size() == 0 );

		walk_children( i );
	}
	// arg list exp
	else if( i->value.id() == parser_id( arg_list_exp_id ) )
	{
		walk_children( i );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
		walk_children( i );
	}
	// key exp
	else if( i->value.id() == parser_id( key_exp_id ) )
	{
		walk_children( i );
	}
	// const decl
	else if( i->value.id() == parser_id( const_decl_id ) )
	{
		walk_children( i );
	}
	// translation unit
	else if( i->value.id() == parser_id( translation_unit_id ) )
	{
		walk_children( i );
	}
	// compound statement
	else if( i->value.id() == parser_id( compound_statement_id ) )
	{
		walk_children( i );
	}
	// break statement
	else if( i->value.id() == parser_id( break_statement_id ) )
	{
		walk_children( i );
	}
	// continue statement
	else if( i->value.id() == parser_id( continue_statement_id ) )
	{
		walk_children( i );
	}
	// return statement
	else if( i->value.id() == parser_id( return_statement_id ) )
	{
		walk_children( i );
	}
	//////////////////////////////////////////////////////////////////
	// nodes of these types shouldn't be created:
	//////////////////////////////////////////////////////////////////
	// comma_op
	else if( i->value.id() == parser_id( comma_op_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// factor_exp
	else if( i->value.id() == parser_id( factor_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// postfix only exp
	else if( i->value.id() == parser_id( postfix_only_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// primary_exp
	else if( i->value.id() == parser_id( primary_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// mult_exp
	else if( i->value.id() == parser_id( mult_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// add_exp
	else if( i->value.id() == parser_id( add_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// relational exp
	else if( i->value.id() == parser_id( relational_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// logical exp
	else if( i->value.id() == parser_id( logical_exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// exp
	else if( i->value.id() == parser_id( exp_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// statement
	else if( i->value.id() == parser_id( statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// top-level statement
	else if( i->value.id() == parser_id( top_level_statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// function decl
	else if( i->value.id() == parser_id( func_decl_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// while statement
	else if( i->value.id() == parser_id( while_statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// for statement
	else if( i->value.id() == parser_id( for_statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// if statement
	else if( i->value.id() == parser_id( if_statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// else statement
	else if( i->value.id() == parser_id( else_statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
	// jump statement
	else if( i->value.id() == parser_id( jump_statement_id ) )
	{
		emit_error( i->value.value(), "", "Semantic error: Encountered invalid node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
	}
    else
    {
		// error, invalid node type
		// TODO: pass actual filename
		emit_error( i->value.value(), "", "Semantic error: Encountered unknown node type" );
		throw SemanticException( "invalid node type", i->value.value().line );
    }
}
