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
	int count = 0;
	int parent = sym->parent_id;
	while( parent != -1 )
	{
		// get the parent scope object
		SymbolTable* p = s[parent];

		cout << count << " checking scope " << parent << " with parent " << p->parent_id << ", for id " << id << endl;
		++count;
		for( SymbolTable::iterator i = p->begin(); i != p->end(); ++i )
		{
			cout << i->first << endl;
		}

		// check for the symbol
		if( p->count( id ) != 0 )
		{
			cout << "FOUND" << endl;
			return true;
		}

		// get this scope's parent
		parent = p->parent_id;
	}
	return false;
}
