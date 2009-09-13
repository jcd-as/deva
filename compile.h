// compile.h
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * 

#ifndef __COMPILE_H__
#define __COMPILE_H__

#include "types.h"
#include "symbol.h"
#include "grammar.h"
#include "debug.h"
#include <fstream>

using namespace std;

// emit an error message
void emit_error( NodeInfo ni, string msg );

// parse a file
// syntax errors output to stdout
tree_parse_info<iterator_t, factory_t> ParseFile( ifstream & file );

// check the semantics of a parsed AST
// semantic errors output to stdout
bool CheckSemantics( tree_parse_info<iterator_t, factory_t> info );

#endif // __COMPILE_H__
