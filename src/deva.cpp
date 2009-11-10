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

int main( int argc, char** argv )
{
	try
	{
		// declare the command line options
		int verbosity;
		bool debug;
		string output;
		string input;
		po::options_description desc( "Supported options" );
		desc.add_options()
			( "help", "help message" )
			( "version,ver", "display program version" )
			( "verbosity,r", po::value<int>( &verbosity )->default_value( 0 ), "set verbosity level (0-3)" )
			( "debug-dump", po::value<bool>( &debug )->default_value( false ), "turn debug output on/off" )
			( "input", po::value<string>( &input ), "input filename" )
			;
		po::positional_options_description p;
		p.add( "input", -1 );
		po::variables_map vm;
		try
		{
			po::store( po::command_line_parser( argc, argv ).options( desc ).positional( p ).run(), vm );
		}
		catch( po::error & e )
		{
			cout << e.what() << endl;
			exit( 1 );
		}
		po::notify( vm );
		
		// handle the command line args
		if( vm.count( "help" ) )
		{
			cout << desc << endl;
			return 1;
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
			string dir = cwd + '/' + in_dir;
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

		// get the filename to compile/run
		const char* input_filename = fname.c_str();

		if( ext == "dv" )
		{
			if( !CompileFile( input_filename ) )
			{
				cout << "Error compiling " << input_filename << endl;
				return -1;
			}
			fname += "c";
		}

		// compilation is done, we don't need the scope table any more
		for( Scopes::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			delete i->second;
		}

		// create our execution engine object
		Executor ex( debug );

		// create a global scope
		ex.StartGlobalScope();

		// add the built-in modules
		AddOsModule( ex );
		AddBitModule( ex );
		AddMathModule( ex );

		// run the .dvc file
		if( !ex.RunFile( fname.c_str() ) )
		{
			ex.EndGlobalScope();
			cout << "Error executing " << input_filename << endl;
			return -1;
		}
		ex.EndGlobalScope();

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
