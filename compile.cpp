// compile.cpp
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * 

#include "compile.h"

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
