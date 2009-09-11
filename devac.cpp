// devac.cpp
// deva language compiler, main program
// created by jcs, september 8, 2009 

// TODO:
// * handle imports (parse recursively)
// * handle out-of-memory condition
// * handle stdin as input (?)

//#define BOOST_SPIRIT_DEBUG

#include "symbol.h"
#include "grammar.h"
#include "debug.h"
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <fstream>

using namespace std;
namespace po = boost::program_options;

// the type of the iterator for the parser
typedef position_iterator<char const*> iterator_t;

// the (temporarily) global symbol table
SymbolTable symTab;

void add_symbol( iterator_t start, iterator_t end )
{
	string s( start, end );
	symTab[s] = SymbolInfo( sym_unknown );
}

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

	// get the length of the file
	file.seekg( 0, ios::end );
	int length = file.tellg();
	file.seekg( 0, ios::beg );
	// allocate memory to read the file into
	char* buf = new char[length];
	// read the file
	file.read( buf, length );
	// close the file
	file.close();

	// create our grammar parser
	DevaGrammar deva_p;

	// create the position iterator for the parser
	iterator_t begin( buf, buf+length );
	iterator_t end;


	// parse 
	//
	tree_parse_info<iterator_t, factory_t> info;
	info = ast_parse<factory_t>( begin, end, deva_p, (space_p | comment_p( "#" )) );
	if( !info.full )
	{
		cout << "error parsing " << input_filename << endl;
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
		cout << "symbols:" << endl;
		for( SymbolTable::iterator i = symTab.begin(); i != symTab.end(); ++i )
		{
			cout << i->first << endl;
		}
	}

	// free the buffer with the code in it
	delete[] buf;

	// compile completion message for non-terse settings
	if( verbosity > 0 )
		cout << "compiled " << input_filename << " successfully" << endl;
	return 0;
}
