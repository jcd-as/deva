// Copyright (c) 2010 Joshua C. Shepard
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// api.h
// public api functions for the deva language
// created by jcs, december 22, 2010 

// TODO:
// * 

#ifndef __API_H__
#define __API_H__

#include <antlr3.h>
#include "devaLexer.h"
#include "devaParser.h"
#include "semantic_walker.h"
#include "compile_walker.h"

#include "semantics.h"
#include "compile.h"
#include "executor.h"
#include "exceptions.h"
#include "util.h"
#include "error.h"
#include "code.h"


namespace deva
{

// global object to track current filename
extern const char* current_file;

struct ParseReturnValue
{
	bool successful;
	pANTLR3_INPUT_STREAM input_stream;
	pdevaLexer lexer;
	pANTLR3_COMMON_TOKEN_STREAM tokenstream;
	pdevaParser parser;
	devaParser_translation_unit_return ast;
	ParseReturnValue() : successful( false ), input_stream( NULL ), lexer( NULL ), tokenstream( NULL ), parser( NULL ) {}
};

// TODO: currently no flags. do we need this?
struct PassOneFlags
{
//	bool show_warnings;
	PassOneFlags() /*: show_warnings( false )*/ {}
};

// TODO: currently no flags. do we need this?
struct PassTwoFlags
{
};

struct PassOneReturnValue
{
	pANTLR3_COMMON_TREE_NODE_STREAM nodes;
	size_t num_constants;
	PassOneReturnValue() : nodes( NULL ), num_constants( 0 ) {}
};

inline void SetCurrentFile( const char* filename ) { current_file = filename; }
inline const char* GetCurrentFile() { return current_file; }
ParseReturnValue Parse( pANTLR3_INPUT_STREAM in );
ParseReturnValue Parse( const char* module );
ParseReturnValue Parse( const char* input, size_t length );
// free parser memory, call after completely finished with parser items, ast etc
void FreeParseReturnValue( ParseReturnValue & prv );
PassOneReturnValue PassOne( ParseReturnValue prv, PassOneFlags flags );
Code* PassTwo( const char* filename, PassOneReturnValue p1rv, PassTwoFlags flags );
PassOneReturnValue Compile( const char* filename, ParseReturnValue prv, PassOneFlags p1flags, PassTwoFlags p2flags );
// free pass one/compilation memory, call after finished compiling, using ast
void FreePassOneReturnValue( PassOneReturnValue & p1rv );


} // end namespace deva

#endif // __API_H__
