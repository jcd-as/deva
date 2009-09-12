// devac.cpp
// deva language compiler, main program
// created by jcs, september 8, 2009 

// TODO:
// * handle imports (parse recursively)
// 		- break compiling a module into a separate fcn from main()
// * handle out-of-memory conditions (allocating text buffer, SymbolTable*s)
// * handle stdin as input (?)

//#define BOOST_SPIRIT_DEBUG

#include "compile.h"
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <fstream>
#include <utility>

using namespace std;
namespace po = boost::program_options;

// the type of the iterator for the parser
typedef position_iterator<char const*> iterator_t;

// the global scope table
Scopes scopes;

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
		( "output,o", po::value<string>( &output )->default_value( "a.dvc" ), "output filename" )
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

	// open input file
	ifstream file;
	file.open( input_filename );
	if( !file.is_open() )
	{
		cout << "error opening " << input_filename << endl;
		exit( -1 );
	}

	tree_parse_info<iterator_t, factory_t> info = ParseFile( file );
	if( !info.full )
	{
		cout << "error parsing " << input << endl;
		return -1;
	}

	// TODO: semantic checking
	// TODO: generate IL
	// TODO: optimize IL
	// TODO: generate bytecode

	// debug & verbose diagnostics/output:
	if( debug && verbosity >= 3 )
	{
		// dump the AST:
		evaluate( info );

		// dump the symbol table:
		cout << "Symbols:" << endl;
		// for each item in the scopes map
		for( Scopes::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			cout << "Scope " << i->first << ", parent = " << i->second->parent_id << endl;
			for( SymbolTable::iterator j = i->second->begin(); j != i->second->end(); ++j )
			{
				cout << "\t" << j->first << endl;
			}
		}
	}

	// clean-up:

	// free SymbolTable pointers in the global scopes table
	for( Scopes::iterator i = scopes.begin(); i != scopes.end(); ++i )
	{
		delete i->second;
	}

	// compile completion message for non-terse settings
	if( verbosity > 0 )
		cout << "compiled " << input << " successfully" << endl;
	return 0;
}
