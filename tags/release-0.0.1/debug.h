// debug.h
// debug functions for deva language compiler
// created by jcs, september 9, 2009

// TODO:
// * 

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "symbol.h"
#include "grammar.h"

// utility fcns:
////////////////////////
void indent( int ind );
// output whether something succeeded or failed
int print_parsed( bool parsed );


// AST evaluation fcns:
////////////////////////
void dumpNode( const char* name, iter_t const& i, int & indents );
void eval_expression( iter_t const& i );
void evaluate( tree_parse_info<iterator_t, factory_t> info );


#endif // __DEBUG_H__
