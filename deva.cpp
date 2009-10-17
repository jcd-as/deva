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
//#include <fstream>
#include <utility>

#include "executor.h"

using namespace std;
namespace po = boost::program_options;

// the global scope table
//Scopes scopes;

int main( int argc, char** argv )
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
		cout << "usage: devac [options] <input_file>" << endl;
		cout << "(use option --help for more information)" << endl;
		return 1;
	}

	// get the filename to compile
	const char* input_filename = input.c_str();

	Executor ex( input, debug );
	if( !ex.RunFile() )
	{
		cout << "Error executing " << input_filename << endl;
		return -1;
	}

	// dump ref count map if we're in debug mode
	if( debug )
	{
		cout << "Memory reference count table for vectors:" << endl;
		smart_ptr<vector<DevaObject> >::dumpRefCountMap();
		cout << "Memory reference count table for maps:" << endl;
		smart_ptr<map<DevaObject, DevaObject> >::dumpRefCountMap();
	}

	return 0;
}
