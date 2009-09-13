// semantics.cpp
// semantic analysis functions for the deva language
// created by jcs, september 12, 2009 

// TODO:
// * 

#include "semantics.h"
#include "parser_ids.h"
#include <iostream>

// TODO: remove this prototype
void check_( iter_t const & i )
{
	// double-check syntax:
	// check the number of children
	// check the types of the children
	
	// type-checking of symbols:
}

// 'def' keyword
void check_func( iter_t const & i )
{
	// check the number of children
	// 3 children: id, arg_list, compound_statement | statement
	if( i->children.size() != 3 )
		throw SemanticException( "Function declaration node doesn't have three children", i->value.value().line );
	// check the types of the children
	if( i->children[0].value.id() != parser_id( identifier_id ) )
		throw SemanticException( "Invalid identifier for function declaration", i->value.value().line );
	if( i->children[1].value.id() != arg_list_decl_id )
		throw SemanticException( "Invalid argument list for function declaration", i->value.value().line );
	// TODO: 3rd child has to be some kind of expression. identifier, assign_op
	// etc etc
//	if( i->children[2].value.id() != compound_statement_id 
//		&& i->children[2].value.id() != identifier )
//		throw SemanticException( "Invalid argument list for function declaration", i->value.value().line );
}

void check_while_s( iter_t const & i )
{
}
