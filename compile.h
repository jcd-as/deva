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
#include "exceptions.h"
#include <fstream>

using namespace std;

// emit an error message
void emit_error( NodeInfo ni, string msg );
void emit_error( string filename, string msg, int line );
void emit_error( DevaSemanticException & e );

// emit a warning message
void emit_warning( NodeInfo ni, string msg );
void emit_warning( string filename, string msg, int line );
void emit_warning( DevaSemanticException & e );

// parse a file
// syntax errors output to stdout
tree_parse_info<iterator_t, factory_t> ParseFile( string filename, istream & file );
tree_parse_info<iterator_t, factory_t> ParseText( string filename, const char* const input, long input_len );

// check the semantics of a parsed AST
// semantic errors output to stdout
bool CheckSemantics( tree_parse_info<iterator_t, factory_t> info );

// generate IL bytecode
bool GenerateIL( tree_parse_info<iterator_t, factory_t> info, InstructionStream & is, bool debug_info );

// write the bytecode to disk
bool GenerateByteCode( char const* filename, InstructionStream & is );
// generate and return bytecode
unsigned char* GenerateByteCode( InstructionStream & is );

// de-compile a .dvc file and dump the IL to stdout
bool DeCompileFile( char const* filename );

// parse, check semantics, generate IL and generate bytecode for a .dv file
bool CompileFile( char const* filename, bool debug_info = true );

// parse, check semantics, generate IL and generate bytecode for an input string
// containing deva code, returns the byte code (which must be freed by the
// caller) or NULL on failure
unsigned char* CompileText( char const* const input, long input_len, bool debug_info = true );

#endif // __COMPILE_H__
