// Copyright (c) 2009 Joshua C. Shepard
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
// created by jcs, september 16, 2009 

// TODO:
// * handle out-of-memory conditions (allocating text buffer, SymbolTable*s)

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <utility>

#include "compile.h"
#include "executor.h"
#include "util.h"
#include "module_os.h"
#include "module_bit.h"
#include "module_math.h"

using namespace std;
namespace po = boost::program_options;

// the global scope table (used when compilation is needed, as in imports
// and dynamic code generation)
Scopes scopes;
int _argc;
char** _argv;

int main( int argc, char** argv )
{
	_argc = argc;
	_argv = argv;
	try
	{
		// declare the command line options
		int verbosity;
		bool debug;
		bool show_all_scopes = false;
		string output;
		string input;
		po::options_description desc( "Supported options" );
		desc.add_options()
			( "help", "help message" )
			( "version,ver", "display program version" )
			( "verbosity,r", po::value<int>( &verbosity )->default_value( 0 ), "set verbosity level (0-3)" )
			( "debug-dump", po::value<bool>( &debug )->default_value( false ), "turn debug output on/off" )
			( "all-scopes,s", "show all scopes in tracebacks, not just calls" )
			( "input", po::value<string>( &input ), "input filename" )
			( "options", "options to pass to the deva program" )
			;
		po::positional_options_description p;
		p.add( "input", 1 ).add( "options", -1 );
		po::variables_map vm;
		try
		{
			po::store( po::command_line_parser( argc, argv ).options( desc ).allow_unregistered().positional( p ).run(), vm );
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
			cout << "deva " << VERSION << endl;
			return 1;
		}
		if( vm.count( "all-scopes" ) )
		{
			show_all_scopes = true;
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

		if( ext == ".dv" )
		{
			if( !CompileFile( fname.c_str() ) )
			{
				cout << "Error compiling " << fname << endl;
				return -1;
			}
			fname += "c";
		}

		// create our execution engine object
		Executor ex( debug );

		try
		{
			// create a global scope
			ex.StartGlobalScope();

			// add the built-in modules
			AddOsModule( ex );
			AddBitModule( ex );
			AddMathModule( ex );

			// run the .dvc file
			ex.RunFile( fname.c_str() );
			ex.EndGlobalScope();
		}
		catch( DevaRuntimeException & e )
		{
			if( typeid( e ) == typeid( DevaICE ) )
				throw;
			cout << "Error: " << e.what() << endl;
			// dump the stack trace
			ex.DumpTrace( cout, show_all_scopes );
			return -1;
		}

		// done, free the scope tables
		for( Scopes::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			delete i->second;
		}

		// dump ref count map if we're in debug mode
		if( debug )
		{
			cout << "Memory reference count table for vectors:" << endl;
			smart_ptr<DOVector>::dumpRefCountMap();
			cout << "Memory reference count table for maps:" << endl;
			smart_ptr<DOMap>::dumpRefCountMap();
		}
	}
	catch( DevaICE & e )
	{
		cout << "Internal compiler error: " << e.what() << endl;
		return -1;
	}
	// runtime exceptions really should only stem from the Executor,
	// but just in case...
	catch( DevaRuntimeException & e )
	{
		cout << "Error: " << e.what() << endl;
		return -1;
	}
	catch( logic_error & e )
	{
		cout << "Error: " << e.what() << endl;
		return -1;
	}

	return 0;
}
