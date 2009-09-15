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
#include "instructions.h"
#include <fstream>

using namespace std;

// emit an error message
void emit_error( NodeInfo ni, string msg );
void emit_error( string filename, string msg, int line );
void emit_error( SemanticException & e );

// emit a warning message
void emit_warning( NodeInfo ni, string msg );
void emit_warning( string filename, string msg, int line );
void emit_warning( SemanticException & e );

// parse a file
// syntax errors output to stdout
tree_parse_info<iterator_t, factory_t> ParseFile( string filename, ifstream & file );

// check the semantics of a parsed AST
// semantic errors output to stdout
bool CheckSemantics( tree_parse_info<iterator_t, factory_t> info );

// generate IL bytecode
bool GenerateIL( tree_parse_info<iterator_t, factory_t> info, InstructionStream & is );

#endif // __COMPILE_H__
