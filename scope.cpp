// scope.cpp
// deva language grammar for expressions
// created by jcs, september 11, 2009 

// TODO:
// * 

#include "scope.h"
#include <iostream>

using namespace std;

// utility function to locate a symbol in parent scopes
bool find_identifier_in_parent_scopes( string id, SymbolTable* sym, Scopes s )
{
	int parent = sym->parent_id;
	while( parent != -1 )
	{
		// get the parent scope object
		SymbolTable* p = s[parent];

		// check for the symbol
		if( p->count( id ) != 0 )
			return true;

		// get this scope's parent
		parent = p->parent_id;
	}
	return false;
}

// locate a symbol
SymbolInfo find_symbol( string id, SymbolTable* sym, Scopes s )
{
	// look in the current symbol table first
	if( sym->count( id ) != 0 )
		return sym->operator[]( id );

	// then check in the parents
	int parent = sym->parent_id;
	while( parent != -1 )
	{
		// get the parent scope object
		SymbolTable* p = s[parent];

		// check for the symbol
		if( p->count( id ) != 0 )
			return p->operator[]( id );

		// get this scope's parent
		parent = p->parent_id;
	}
	return SymbolInfo( sym_end );
}

// sets a symbol's 'const' value, if it is in the symbol table given
// returns true if found and set and false if not found
bool set_symbol_constness( string id, SymbolTable* symtab, Scopes s, bool constness )
{
	// look in the current symbol table first
	if( symtab->count( id ) != 0 )
	{
		SymbolInfo si = symtab->at( id );
		// remove the symbol
		symtab->erase( id );
		// add it back with the const flag set
		si.is_const = constness;
		symtab->insert( make_pair( id, si ) );
		return true;
	}

	// then check in the parents
	int parent = symtab->parent_id;
	while( parent != -1 )
	{
		// get the parent scope object
		SymbolTable* p = s[parent];

		// check for the symbol
		if( p->count( id ) != 0 )
		{
			SymbolInfo si = p->at( id );
			// remove the symbol
			p->erase( id );
			// add it back with the const flag set
			si.is_const = constness;
			p->insert( make_pair( id, si ) );
			return true;
		}

		// get this scope's parent
		parent = p->parent_id;
	}
	return false;
}

