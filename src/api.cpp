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

// api.cpp
// public api functions for the deva language
// created by jcs, december 28, 2010 

// TODO:
// * 

#include "api.h"


namespace deva
{


/////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////

// global object to track current filename
//const char* deva::current_file = NULL;
const char* current_file = NULL;

// global to enable tracing refcounts
bool reftrace = false;

int _argc;
char** _argv;


/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////
//
ParseReturnValue Parse( pANTLR3_INPUT_STREAM input_stream )
{
	if( !current_file )
		throw ICE( "Error: global current_file not set. SetCurrentFile() must be called before compilation." );

	ParseReturnValue ret;
	ret.input_stream = input_stream;

	// Our input stream is now open and all set to go, so we can create a new instance of our
	// lexer and set the lexer input to our input stream:
	//  (file | memory | ?) --> inputstream -> lexer --> tokenstream --> parser ( --> treeparser )?
	ret.lexer = devaLexerNew( input_stream );      // CLexerNew is generated by ANTLR
	if( !ret.lexer )
		throw RuntimeException( "Unable to create lexer. Out of memory?" );

	// Our lexer is in place, so we can create the token stream from it
	ret.tokenstream = antlr3CommonTokenStreamSourceNew( ANTLR3_SIZE_HINT, TOKENSOURCE( ret.lexer ) );
	if( !ret.tokenstream )
		throw RuntimeException( "Unable to create token stream for lexer. Out of memory?" );

	// Finally, now that we have our lexer constructed, we can create the parser
	ret.parser = devaParserNew( ret.tokenstream );  // CParserNew is generated by ANTLR3
	if( !ret.parser )
		throw RuntimeException( "Unable to create parser. Out of memory?" );

	// parse and build the AST
	ret.ast = ret.parser->translation_unit( ret.parser );

	ret.successful = (ret.parser->pParser->rec->state->errorCount == 0);
	return ret;
}

void FreeParseReturnValue( ParseReturnValue & prv )
{
	if( prv.parser ) prv.parser->free( prv.parser );
	prv.parser = NULL;
	if( prv.tokenstream ) prv.tokenstream->free( prv.tokenstream );
	prv.tokenstream = NULL;
	if( prv.lexer ) prv.lexer->free( prv.lexer );
	prv.lexer = NULL;
	if( prv.input_stream ) prv.input_stream->close( prv.input_stream );
	prv.input_stream = NULL;
}

ParseReturnValue Parse( const char* filename )
{
	pANTLR3_INPUT_STREAM input_stream;

	// get the input stream for the given filename
	input_stream = antlr3AsciiFileStreamNew( (unsigned char*)filename );
	current_file = filename;

	// The input will be created successfully, providing that there is enough
	// memory and the file exists etc
	if( !input_stream )
	   throw RuntimeException( boost::format( "Unable to open file '%1%'" ) % filename );

	return Parse( input_stream );
}

// parse from a string
ParseReturnValue Parse( const char* input, size_t length )
{
	pANTLR3_INPUT_STREAM input_stream;
	input_stream = antlr3NewAsciiStringInPlaceStream( (unsigned char*)input, (ANTLR3_UINT32)length, NULL );

	current_file = "[TEXT]";

	// The input will be created successfully, providing that there is enough
	// memory etc
	if( !input_stream )
	   throw RuntimeException( "Error creating input for parser." );

	return Parse( input_stream );
}

PassOneReturnValue PassOne( ParseReturnValue prv, PassOneFlags flags )
{
	if( !current_file )
		throw ICE( "Error: global current_file not set. SetCurrentFile() must be called before compilation." );
	pANTLR3_COMMON_TREE_NODE_STREAM nodes;
	psemantic_walker treePsr;

	// create and init the deva semantics, compiler and execution engine components
	// ("current_file" must be set before we can do this)
	semantics = new Semantics( flags.ignore_undefined_vars );

	// TODO: other flags?

	// allocate the nodes
	nodes = antlr3CommonTreeNodeStreamNewTree( prv.ast.tree, ANTLR3_SIZE_HINT ); // sIZE HINT WILL SOON BE DEPRECATED!!

	// Tree parsers are given a common tree node stream (or your override)
	treePsr = semantic_walkerNew( nodes );

	// PASS ONE: build the symbol table and check semantics
	treePsr->translation_unit( treePsr );

	// free memory
	treePsr->free( treePsr );      

	PassOneReturnValue ret;
	ret.nodes = nodes;
	ret.num_constants = semantics->constants.size();
	return ret;
}

Code* PassTwo( const char* module_name, PassOneReturnValue p1rv, PassTwoFlags flags )
{
	if( !current_file )
		throw ICE( "Error: global current_file not set. SetCurrentFile() must be called before compilation." );
	pcompile_walker cmpPsr;

	// PASS TWO: compile
	// (semantics and execution engine must be created BEFORE the compiler...)
	
	if( !ex )
		throw ICE( "Executor object must exist before calling PassTwo()." );

	compiler = new Compiler( module_name, semantics, flags.interactive );

	cmpPsr = compile_walkerNew( p1rv.nodes );
	cmpPsr->translation_unit( cmpPsr );

	// free items
	cmpPsr->free( cmpPsr );      

	return compiler->GetCode();
}

Code* Compile( const char* module_name, PassOneFlags p1flags, PassTwoFlags p2flags )
{
	ParseReturnValue prv = Parse( module_name );
	if( !prv.successful )
		return NULL;

	if( !current_file )
		throw ICE( "Error: global current_file not set. SetCurrentFile() must be called before compilation." );

	PassOneReturnValue p1rv = PassOne( prv, p1flags );
	string mod( module_name );
	string fp = get_file_part( mod );
	string modname = get_stem( fp );
	return PassTwo( modname.c_str(), p1rv, p2flags );
}

Code* Compile( const char* module_name, ParseReturnValue prv, PassOneFlags p1flags, PassTwoFlags p2flags )
{
	if( !current_file )
		throw ICE( "Error: global current_file not set. SetCurrentFile() must be called before compilation." );
	PassOneReturnValue p1rv = PassOne( prv, p1flags );
	return PassTwo( module_name, p1rv, p2flags );
}

void FreePassOneReturnValue( PassOneReturnValue & p1rv )
{
	if( p1rv.nodes ) p1rv.nodes->free( p1rv.nodes );
	p1rv.nodes = NULL;
}


} // end namespace deva
