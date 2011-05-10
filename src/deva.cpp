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

// TODO: 
// * quick exit, don't bother to clean up, OS will reclaim memory???
//

#include "api.h"
#include "module_os.h"
#include "module_bit.h"
#include "module_math.h"
#include "module_re.h"

#include <iostream>
#include <vector>
#include <boost/format.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std;
namespace po = boost::program_options;

using namespace deva;
using namespace deva_compile;

#ifdef MS_WINDOWS
#define chdir(x) _chdir(x)
#include <direct.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////

namespace deva
{
int _argc;
char** _argv;
}

// global to enable tracing refcounts
bool deva::reftrace;

/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////

// Main entry point for this example
//
int ANTLR3_CDECL main( int argc, char *argv[] )
{
	deva::_argc = argc;
	deva::_argv = argv;

	// declare the command line options
	bool debug_dump = false;
	bool trace = false;
	reftrace = false;
	bool show_ast = false;
	bool no_dvc = false;
	bool disasm = false;
	bool compile_only = false;
	string output;
	string input;
	vector<string> inputs;
	po::options_description desc( "Supported options" );
	desc.add_options()
		( "help", "help message" )
		( "version,v", "display program version" )
		( "no-dvc", "do NOT write a .dvc compiled byte-code file to disk" )
		( "compile-only,c", "compile only, do not execute" )
		( "disasm", "disassemble" )
#ifdef DEBUG
		( "trace", "show execution trace" )
		( "reftrace", "show refcount trace" )
		( "show-ast", "show the AST" )
		( "debug-dump", "turn internal debug output on" )
#endif
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
	if( vm.count( "no-dvc" ) )
	{
		no_dvc = true;
	}
	if( vm.count( "disasm" ) )
	{
		disasm = true;
	}
#ifdef DEBUG
	if( vm.count( "trace" ) )
	{
		trace = true;
	}
	if( vm.count( "reftrace" ) )
	{
		reftrace = true;
	}
	if( vm.count( "show-ast" ) )
	{
		show_ast = true;
	}
	if( vm.count( "debug-dump" ) )
	{
		debug_dump = true;
	}
#endif
	if( vm.count( "compile-only" ) )
	{
		compile_only = true;
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

	bool use_dvc = false;
	Code* code = NULL;

	ex = new Executor();

	ParseReturnValue prv;
	PassOneReturnValue p1rv;
	string out_fname;
	try
	{
		// set the file we're compiling
		SetCurrentFile( fname.c_str() );

		// if we weren't passed a .dvc file as input, see if we need to compile
		if( ext == ".dvc" )
		{
			out_fname = fname;
			use_dvc = true;
		}
		else
		{
			out_fname = fname + "c";

			// check for a .dvc file
			struct stat in_statbuf;
			struct stat out_statbuf;

			// if we can't open the .dvc file, continue on
			if( stat( out_fname.c_str(), &out_statbuf ) != -1 )
			{
				if( stat( fname.c_str(), &in_statbuf ) != -1 ) 
				{
					// if the output is newer than the input, nothing to do
					if( out_statbuf.st_mtime > in_statbuf.st_mtime )
						use_dvc = true;
				}
				// .dvc file exists, but no .dv file
				else
					use_dvc = true;
			}
		}
		if( !use_dvc )
		{
			// parse the file
			prv = Parse( fname.c_str() );

			if( prv.successful )
			{
				PassOneFlags p1f; // currently no pass one flags
				PassTwoFlags p2f;

				// PASS ONE: build the symbol table and check semantics
				p1rv = PassOne( prv, p1f );

				// diagnostic information output:
				// print the text repr of the tree?
				if( show_ast )
				{
					cout << "AST:" << endl;
					pANTLR3_STRING s = p1rv.nodes->root->toStringTree( p1rv.nodes->root );
					ANTLR3_FPRINTF( stdout, "%s\n", (char*)s->chars );
				}

				// PASS TWO: compile
//				code = PassTwo( "", p1rv, p2f );
				string fp = get_file_part( fname );
				string module_name = get_stem( fp );
				code = PassTwo( module_name.c_str(), p1rv, p2f );

				// debug dumps
#ifdef DEBUG
				if( debug_dump )
				{
					// dump the symbol tables...
					semantics->DumpSymbolTable();
				}
#endif

				// free compile-time objects before executing code
				// free parser, compile memory
				FreeParseReturnValue( prv );
				FreePassOneReturnValue( p1rv );
				delete compiler;
				compiler = NULL;
				delete semantics;
				semantics = NULL;

				// by default, write the .dvc file
				if( !no_dvc )
					ex->WriteCode( out_fname, code );
			}
		}
		// otherwise, load the .dvc
		else
		{
			code = ex->ReadCode( out_fname );
		}

		// done compiling
#ifdef DEBUG
		if( debug_dump )
		{
			// dump the constant data pool
			ex->DumpConstantPool( code );
			// dump the function objects
			ex->DumpFunctions();
		}
#endif

		if( disasm )
		{
			ex->Decode( code );
		}

		// execute the code
		if( !compile_only )
		{
			// set execution flags
			ex->trace = trace;
			// add built-in (native) modules
			ex->AddNativeModule( GetModuleOs() );
			ex->AddNativeModule( GetModuleBit() );
			ex->AddNativeModule( GetModuleMath() );
			ex->AddNativeModule( GetModuleRe() );
			// execute the code
			ex->Execute( code );
		}
		else
			delete code;
	}
	catch( SemanticException & e )
	{
		// display an error
		emit_error( e );

		// QUICK EXIT on error: 
		// - let the os reclaim the memory
		// - do not destruct user's deva objects

		// free parser, compiler memory
//		FreeParseReturnValue( prv );
//		FreePassOneReturnValue( p1rv );
//		// free the semantics objects (symbol table et al), compiler, executor
//		delete ex;
//		delete compiler;
//		delete semantics;

		exit( -1 );
	}
	catch( ICE & e )
	{
		// display an error
		emit_error( e );

		// QUICK EXIT on error: 
		// - let the os reclaim the memory
		// - do not destruct user's deva objects

		// free parser, compile memory
//		FreeParseReturnValue( prv );
//		FreePassOneReturnValue( p1rv );
//		// free the semantics objects (symbol table et al), compiler, executor
//		delete ex;
//		delete compiler;
//		delete semantics;

		exit( -1 );
	}
	catch( RuntimeException & e )
	{
		// display an error
		emit_error( e );

		// dump the stack trace
		if( ex )
			ex->DumpTrace( cerr );

		// QUICK EXIT on error: 
		// - let the os reclaim the memory
		// - do not destruct user's deva objects

		// free parser, compile memory
//		FreeParseReturnValue( prv );
//		FreePassOneReturnValue( p1rv );
//		// free the semantics objects (symbol table et al), compiler, executor
//		delete ex;
//		delete compiler;
//		delete semantics;

		exit( -1 );
	}

	// free the execution engine
	delete ex;

	return 0;
}
