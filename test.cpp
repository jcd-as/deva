// test.cpp
// test driver for deva language grammar elements
// created by jcs, august 29, 2009

// TODO:
// * 

//#define BOOST_SPIRIT_DEBUG

#include "symbol.h"
#include "grammar.h"
#include <iostream>
#include <fstream>


// the (temporarily) global symbol table
SymbolTable symTab;

// the global scope table
Scopes scopes;

// stack of scopes, for building the global scope table
ScopeBuilder scope_bldr;

void add_symbol( iterator_t start, iterator_t end )
{
	string s( start, end );
	symTab[s] = SymbolInfo( sym_unknown );
}

// output whether something succeeded or failed
int print_parsed( bool parsed )
{
	if( parsed )
	{
		cout << "parsed successfully" << endl;
		return 0;
	}
	else
	{
		cout << "parse failed!" << endl;
		return 1;
	}
}

typedef tree_match<iterator_t, factory_t> parse_tree_match_t;
typedef parse_tree_match_t::tree_iterator iter_t;

void indent( int ind )
{
	for( int i = 0; i < ind; i++ )
		cout << '\t';
}

void eval_expression( iter_t const& i );

void dumpNode( const char* name, iter_t const& i, int & indents )
{
	string s( i->value.begin(), i->value.end() );
	indent( indents );
	cout << name << ": " << strip_symbol( s ) << endl;

	indents++;
	for( int c = 0; c < i->children.size(); c++ )
		eval_expression( i->children.begin() + c );
	indents--;
}

void eval_expression( iter_t const& i )
{
	static int indents = 0;

	indent( indents );
    cout << "In eval_expression. i->value = " <<
        strip_symbol( string(i->value.begin(), i->value.end()) ) <<
        " i->children.size() = " << i->children.size() << endl;

	// number
    if( i->value.id() == parser_id( number_id ) )
    {
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );
		dumpNode( "num", i, indents );
    }
	// string
	else if( i->value.id() == parser_id( string_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		dumpNode( "string", i, indents );
	}
	// boolean
	else if( i->value.id() == parser_id( boolean_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		dumpNode( "boolean", i, indents );
	}
	// null
	else if( i->value.id() == parser_id( null_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		dumpNode( "null", i, indents );
	}
	// constant (keyword 'const')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		// self and possibly semi-colon
        assert( i->children.size() == 0 || i->children.size() == 1 );

		dumpNode( "const", i, indents );
	}
	// func (keyword 'def' for defining functions)
	else if( i->value.id() == parser_id( func_id ) )
	{
		// children: id, arg_list, compound_statement | statement
        assert( i->children.size() == 3 );
		dumpNode( "func", i, indents );
	}
	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		// self and possibly semi-colon
//        assert( i->children.size() == 0 || i->children.size() == 1 );
		dumpNode( "while_s", i, indents );
	}
	// for_s (keyword 'for')
	else if( i->value.id() == parser_id( for_s_id ) )
	{
		// self and possibly semi-colon
//        assert( i->children.size() == 0 || i->children.size() == 1 );
		dumpNode( "for_s", i, indents );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		// self and possibly semi-colon
//        assert( i->children.size() == 0 || i->children.size() == 1 );
		dumpNode( "if_s", i, indents );
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		// self and possibly semi-colon
//        assert( i->children.size() == 0 || i->children.size() == 1 );
		dumpNode( "else_s", i, indents );
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		// can have arg_list & semi-colon
        assert( i->children.size() < 3 );

		dumpNode( "identifier", i, indents );
	}
	// in op ('in' keyword in for loops)
	else if( i->value.id() == parser_id( in_op_id ) )
	{
		dumpNode( "in_op", i, indents );
	}
	// map construction op
	else if( i->value.id() == parser_id( map_op_id ) )
	{
        assert( i->children.size() == 0 );

		dumpNode( "map_op", i, indents );
	}
	// vector construction op
	else if( i->value.id() == parser_id( vec_op_id ) )
	{
        assert( i->children.size() == 0 );

		dumpNode( "vec_op", i, indents );
	}
	// semicolon op
	else if( i->value.id() == parser_id( semicolon_op_id ) )
	{
        assert( i->children.size() == 0 );

		dumpNode( "semicolon_op", i, indents );
	}
	// assignment op
	else if( i->value.id() == parser_id( assignment_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		dumpNode( "assignment_op", i, indents );
	}
	// logical op
	else if( i->value.id() == parser_id( logical_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		dumpNode( "logical_op", i, indents );
	}
	// relational op
	else if( i->value.id() == parser_id( relational_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		dumpNode( "relational_op", i, indents );
	}
	// mult_op
	else if( i->value.id() == parser_id( mult_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		dumpNode( "mult_op", i, indents );
	}
	// add_op
	else if( i->value.id() == parser_id( add_op_id ) )
	{
        // either the two sides or the two sides and a semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		dumpNode( "add_op", i, indents );
	}
	// unary_op
	else if( i->value.id() == parser_id( unary_op_id ) )
	{
		// operand and possibly semi-colon
        assert( i->children.size() == 1 || i->children.size() == 2 );

		dumpNode( "unary_op", i, indents );
	}
	// dot op
	else if( i->value.id() == parser_id( dot_op_id ) )
	{
		// operands (lhs & rhs) and possibly semi-colon
        assert( i->children.size() == 2 || i->children.size() == 3 );

		dumpNode( "dot_op", i, indents );
	}
	// paren ops
	else if( i->value.id() == parser_id( open_paren_op_id )
		  || i->value.id() == parser_id( close_paren_op_id ) )
	{
        assert( i->children.size() == 0 );

		dumpNode( "(open|close)_paren_op", i, indents );
	}
	// bracket ops
	else if( i->value.id() == parser_id( open_bracket_op_id ) 
			|| i->value.id() == parser_id( close_bracket_op_id ) )
	{
        assert( i->children.size() == 0 );

		dumpNode( "(open|close)_bracket_op", i, indents );
	}
	// comma_op
	// nodes of this type not created
//	else if( i->value.id() == parser_id( comma_op_id ) )
//	{
//        assert( i->children.size() == 0 );
//
//		dumpNode( "comma_op", i, indents );
//	}
	// factor_exp
	// nodes of this type not created
//	else if( i->value.id() == parser_id( factor_exp_id ) )
//	{
//		// TODO: assert on number of children (?)
//		dumpNode( "factor_exp", i, indents );
//	}
	// postfix only exp
//	else if( i->value.id() == parser_id( postfix_only_exp_id ) )
//	{
//	}
	// primary_exp
//	else if( i->value.id() == parser_id( primary_exp_id ) )
//	{
//	}
//	// mult_exp
//	else if( i->value.id() == parser_id( mult_exp_id ) )
//	{
//	}
//	// add_exp
//	else if( i->value.id() == parser_id( add_exp_id ) )
//	{
//	}
//	// relational exp
//	else if( i->value.id() == parser_id( relational_exp_id ) )
//	{
//	}
//	// logical exp
//	else if( i->value.id() == parser_id( logical_exp_id ) )
//	{
//	}
	// arg list exp
	else if( i->value.id() == parser_id( arg_list_exp_id ) )
	{
		dumpNode( "arg_list_exp", i, indents );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
		dumpNode( "arg_list_decl", i, indents );
	}
	// key exp
	else if( i->value.id() == parser_id( key_exp_id ) )
	{
		dumpNode( "key_exp", i, indents );
	}
	// const decl
	else if( i->value.id() == parser_id( const_decl_id ) )
	{
		dumpNode( "const_decl", i, indents );
	}
	// exp
//	else if( i->value.id() == parser_id( exp_id ) )
//	{
//	}
//	// statement
//	else if( i->value.id() == parser_id( statement_id ) )
//	{
//	}
//	// top-level statement
//	else if( i->value.id() == parser_id( top_level_statement_id ) )
//	{
//	}
	// translation unit
	else if( i->value.id() == parser_id( translation_unit_id ) )
	{
		dumpNode( "translation_unit", i, indents );
	}
	// function decl
	// nodes of this type not created
//	else if( i->value.id() == parser_id( func_decl_id ) )
//	{
//		dumpNode( "func_decl", i, indents );
//	}
//	// while statement
//	else if( i->value.id() == parser_id( while_statement_id ) )
//	{
//		dumpNode( "while_statement", i, indents );
//	}
//	// for statement
//	else if( i->value.id() == parser_id( for_statement_id ) )
//	{
//		dumpNode( "for_statement", i, indents );
//	}
//	// if statement
//	else if( i->value.id() == parser_id( if_statement_id ) )
//	{
//		dumpNode( "if_statement", i, indents );
//	}
//	// else statement
//	else if( i->value.id() == parser_id( else_statement_id ) )
//	{
//		dumpNode( "else_statement", i, indents );
//	}
	// compound statement
	else if( i->value.id() == parser_id( compound_statement_id ) )
	{
		dumpNode( "compound_statement", i, indents );
	}
	// jump statement
	// nodes of this type not created
//	else if( i->value.id() == parser_id( jump_statement_id ) )
//	{
//		dumpNode( "jump_statement", i, indents );
//	}
	// break statement
	else if( i->value.id() == parser_id( break_statement_id ) )
	{
		dumpNode( "break_statement", i, indents );
	}
	// continue statement
	else if( i->value.id() == parser_id( continue_statement_id ) )
	{
		dumpNode( "continue_statement", i, indents );
	}
	// return statement
	else if( i->value.id() == parser_id( return_statement_id ) )
	{
		dumpNode( "return_statement", i, indents );
	}
    else
    {
		// don't indent, so we stand out
		cout << "error, unknown id: " << i->value.id().to_long() << endl;
        string s( i->value.begin(), i->value.end() );
		cout << "unknown: " << strip_symbol( s ) << endl;
//        assert(0); // error
		indents++;
		for( int c = 0; c < i->children.size(); c++ )
			eval_expression( i->children.begin() + c );
		indents--;
    }
}

void evaluate( tree_parse_info<iterator_t, factory_t> info )
{
    eval_expression( info.trees.begin() );
}



int main( int argc, char** argv )
{
	// ensure we have enough command-line arguments
	if( argc != 2 )
	{
		cout << "usage: test <input_file>" << endl;
		return -1;
	}

	// get the filename to test from the command-line
	char* input_filename = argv[1];

	// create our grammar parsers
	DevaGrammar deva_p;

	// open input file for the test inputs
	ifstream file;
	file.open( input_filename );
	if( !file.is_open() )
	{
		cout << "error opening " << input_filename << endl;
		exit( -1 );
	}

	// create our initial (global) scope
	SymbolTable* globals = new SymbolTable();
	pair<int, SymbolTable*> sym( 0, globals );
	scope_bldr.push_back( sym );

	int num_tests = 0;
	int num_failed = 0;
	string line, input;
	// try and parse each of the inputs and dump some info from their AST
	while( !file.eof() )
	{
		// get a line from the file
		getline( file, line );
		// ignore lines that start with '### IGNORE'
		string ig( "### IGNORE" );
		if( line.compare( 0, ig.length(), ig ) == 0 )
		{
			continue;
		}
		// TODO: use 'SECTION' tag to indicate whether failure is expected

		// check it for '### SECTION ###', the end-of-test-section marker
		else if( line.compare( "### SECTION ###" ) == 0 )
		{
			num_tests++;
			// parse this section of test code
			cout << "===========================================================" << endl;
            cout << input.c_str() << endl;
			// create the position iterator for the parser
			iterator_t begin( input.c_str(), input.c_str() + input.length() );
			iterator_t end;
			tree_parse_info<iterator_t, factory_t> info;
			info = ast_parse<factory_t>( begin, end, deva_p, (space_p | comment_p( "#" )) );
			num_failed += print_parsed( info.full );
			if( info.full )
				evaluate( info );
			cout << "===========================================================" << endl;

			// reset the input
			input = "";

            // dump the symbol table
//            cout << "symbols:" << endl;
//            for( SymbolTable::iterator i = symTab.begin(); i != symTab.end(); ++i )
//            {
//                cout << i->first << endl;
//            }
		}
		else
		{
			input += "\n";
			input += line;
		}
	}
	file.close();
	cout << "total tests: " << num_tests << endl;
	cout << "tests failed: " << num_failed << endl;
	cout << "exiting..." << endl;
	return 0;
}
