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

// deva.cpp
// deva language executor, main program
// created by jcs, december 09 , 2010 

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

#include <iostream>
#include <vector>
#include <boost/format.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std;
namespace po = boost::program_options;

/////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////
// global object to track current filename
const char* current_file;

int _argc;
char** _argv;

/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////

// Main entry point for this example
//
int ANTLR3_CDECL main( int argc, char *argv[] )
{
	_argc = argc;
	_argv = argv;

	// declare the command line options
	int verbosity = 0;
	bool debug = false;
	bool show_warnings = false;
	bool show_all_scopes = false;
	bool no_dvc = false;
	bool show_ast = false;
	bool disasm = false;
	string output;
	string input;
	vector<string> inputs;
	po::options_description desc( "Supported options" );
	desc.add_options()
		( "help", "help message" )
		( "version,ver", "display program version" )
		( "verbosity,r", po::value<int>( &verbosity )->default_value( 0 ), "set verbosity level (0-3)" )
		( "debug-dump", po::value<bool>( &debug )->default_value( false ), "turn debug output on/off" )
		( "disasm", po::value<bool>( &disasm )->default_value( false ), "disassemble" )
		( "show-ast", po::value<bool>( &show_ast )->default_value( false ), "show the AST" )
		( "show-warnings,w", po::value<bool>( &show_warnings )->default_value( false ), "show warnings" )
		( "all-scopes,s", "show all scopes in tracebacks, not just calls" )
		( "no-dvc", "do NOT write a .dvc compiled byte-code file to disk" )
		( "input", po::value<string>( &input ), "input filename" )
		( "options", po::value<vector<string> >( &inputs )->composing(), "options to pass to the deva program" )
		;
	po::positional_options_description p;
	p.add( "input", 1 ).add( "options", -1 );
	po::variables_map vm;
	try
	{
		po::parsed_options parsed = po::command_line_parser( argc, argv ).options( desc ).allow_unregistered().positional( p ).run();
		po::store( parsed, vm );

	}
	catch( po::error & e )
	{
		cout << e.what() << endl;
		return 1;
	}
	po::notify( vm );

	// handle the command line args
	if( vm.count( "help" ) )
	{
		cout << desc << endl;
		return 1;
	}
	if( vm.count( "version" ) )
	{
		// dump the version number
		cout << "deva " << DEVA_VERSION << endl;
		return 1;
	}
	if( vm.count( "all-scopes" ) )
	{
		show_all_scopes = true;
	}
	if( vm.count( "no-dvc" ) )
	{
		no_dvc = true;
	}
	// must be an input file specified
	if( !vm.count( "input" ) )
	{
		cout << "usage: deva [options] <input_file>" << endl;
		cout << "(use option --help for more information)" << endl;
		return 1;
	}

	// get the filename part and the directory part
	string in_dir = get_dir_part( input );
	string fname = get_file_part( input );
	string ext = get_extension( fname );

	// change cwd to the directory
	// if the input wasn't a full path:
	if( in_dir[0] != '/' )
	{
		string cwd = get_cwd();
		string dir = join_paths( cwd, in_dir );
		if( chdir( dir.c_str() ) != 0 ) 
		{
			cout << "error: unable to change the current working directory to " << dir.c_str() << endl;
			return -1;
		}
	}
	else
	{
		if( chdir( in_dir.c_str() ) != 0 )
		{
			cout << "error: unable to change the current working directory to " << in_dir.c_str() << endl;
			return -1;
		}
	}

//	bool use_dvc = false;
//	unsigned char* code;
//	if( ext != ".dvc" )
//	{
//		// TODO: compile?
//	}

	pANTLR3_INPUT_STREAM input_stream;
	pdevaLexer lxr;
	pANTLR3_COMMON_TOKEN_STREAM tstream;
	pdevaParser psr;
	devaParser_translation_unit_return devaAST;
	pANTLR3_COMMON_TREE_NODE_STREAM nodes;
	psemantic_walker treePsr;
	pcompile_walker cmpPsr;

	// Create the input stream using the supplied file name
	input_stream = antlr3AsciiFileStreamNew( (unsigned char*)fname.c_str() );
	current_file = fname.c_str();

	// The input will be created successfully, providing that there is enough
	// memory and the file exists etc
	if( !input_stream )
	{
	   ANTLR3_FPRINTF( stderr, "Unable to open file %s\n", (char *)fname.c_str() );
	   exit( -1 );
	}

	// Our input stream is now open and all set to go, so we can create a new instance of our
	// lexer and set the lexer input to our input stream:
	//  (file | memory | ?) --> inputstream -> lexer --> tokenstream --> parser ( --> treeparser )?
	lxr = devaLexerNew( input_stream );      // CLexerNew is generated by ANTLR
	if( !lxr )
	{
	   ANTLR3_FPRINTF( stderr, "Unable to create the lexer due to malloc() failure1\n" );
	   exit( ANTLR3_ERR_NOMEM );
	}

	// Our lexer is in place, so we can create the token stream from it
	tstream = antlr3CommonTokenStreamSourceNew( ANTLR3_SIZE_HINT, TOKENSOURCE( lxr ) );
	if( !tstream )
	{
	   ANTLR3_FPRINTF( stderr, "Out of memory trying to allocate token stream\n" );
	   exit( ANTLR3_ERR_NOMEM );
	}

	// Finally, now that we have our lexer constructed, we can create the parser
	psr = devaParserNew( tstream );  // CParserNew is generated by ANTLR3
	if( !psr )
	{
	   ANTLR3_FPRINTF( stderr, "Out of memory trying to allocate parser\n" );
	   exit( ANTLR3_ERR_NOMEM );
	}


	// create and init the deva semantics, compiler and execution engine components
	// ("current_file" must be set before we can do this)
	semantics = new Semantics( show_warnings );

	// parse and build the AST
	devaAST = psr->translation_unit( psr );

	// if the parser ran correctly, we will have a tree to parse
	if( psr->pParser->rec->state->errorCount == 0 )
	{
		nodes = antlr3CommonTreeNodeStreamNewTree( devaAST.tree, ANTLR3_SIZE_HINT ); // sIZE HINT WILL SOON BE DEPRECATED!!

		// Tree parsers are given a common tree node stream (or your override)
		treePsr = semantic_walkerNew( nodes );

		try
		{
			// PASS ONE: build the symbol table and check semantics
			treePsr->translation_unit( treePsr );

			// PASS TWO: compile
			// (semantics and execution engine must be created BEFORE the compiler...)
			ex = new Executor();
			compiler = new Compiler( semantics, ex );
			cmpPsr = compile_walkerNew( nodes );
			cmpPsr->translation_unit( cmpPsr );

		}
		catch( DevaSemanticException & e )
		{
			// display an error
			emit_error( e );

			// quick exit, don't bother to clean up, OS will reclaim memory
			exit( -1 );
		}
		catch( DevaICE & e )
		{
			// display an error
			emit_error( e );

			// quick exit, don't bother to clean up, OS will reclaim memory
			exit( -1 );
		}
//		catch( exception & e )
//		{
//			// clean up
//			psr->free( psr );
//			psr = NULL;
//
//			tstream->free( tstream );
//			tstream = NULL;
//
//			lxr->free( lxr );
//			lxr = NULL;
//
//			input_stream->close( input_stream );
//			input_stream = NULL;
//
//			nodes->free( nodes );        
//			nodes = NULL;
//
//			treePsr->free( treePsr );      
//			treePsr = NULL;
//
//			// free the semantic objects (symbol tables et al)
//			delete semantics;
//
//			exit( -1 );
//		}

		// print the text repr of the tree?
		if( show_ast )
		{
			cout << "AST:" << endl;
			pANTLR3_STRING s = nodes->root->toStringTree( nodes->root );
			ANTLR3_FPRINTF( stdout, "%s\n", (char*)s->chars );
		}

		// debug dumps
		if( debug && verbosity == 3 )
		{
			// dump the symbol tables...
			cout << "Symbol table:" << endl;
			for( vector<Scope*>::iterator i = semantics->scopes.begin(); i != semantics->scopes.end(); ++i )
			{
				if( *i )
					(*i)->Print();
			}
			// dump the constant data pool
			cout << "Constant data pool:" << endl;
			for( int i = 0.; i < ex->constants.Size(); i++ )
			{
				DevaObject o = ex->constants.At( i );
				if( o.type == obj_string )
					cout << o.s << endl;
				else if( o.type == obj_number )
					cout << o.d << endl;
				else if( o.type == obj_boolean )
					cout << (o.b ? "<boolean-true>" : "<boolean-false>") << endl;
				else if( o.type == obj_null )
					cout << "<null-value>" << endl;
			}
			// dump the function objects
			cout << "Function objects:" << endl;
			for( int i = 0; i < ex->functions.Size(); i++ )
			{
				DevaFunction f = ex->functions.At( i );
				cout << "function: " << f.name << ", from file: " << f.filename << ", line: " << f.first_line;
				cout << endl;
				cout << f.num_args << " arg(s), default value indices: ";
				for( int j = 0; j < f.default_args.Size(); j++ )
					cout << f.default_args.At( j ) << " ";
				cout << endl << f.num_locals << " local(s): ";
				for( int j = 0; j < f.local_names.Size(); j++ )
					cout << f.local_names.At( j ) << " ";
				cout << endl << "code address: " << f.addr << endl;
			}
		}

		if( disasm )
		{
			compiler->Decode();
		}

		nodes->free( nodes );        
		nodes = NULL;
		treePsr->free( treePsr );      
		treePsr = NULL;
	}

	// close down our open objects, in the reverse order we created them
	psr->free( psr );
	psr = NULL;

	tstream->free( tstream );
	tstream = NULL;

	lxr->free( lxr );
	lxr = NULL;

	input_stream->close( input_stream );
	input_stream = NULL;

	// free the semantics objects (symbol table et al)
	delete semantics;

	return 0;
}
