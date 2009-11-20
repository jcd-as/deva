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

// devac.cpp
// deva language compiler, main program
// created by jcs, september 8, 2009 

// TODO:
// * handle imports (parse recursively)
// 		- break compiling a module into a separate fcn from main()
// * handle out-of-memory conditions (allocating text buffer, SymbolTable*s)
// * handle stdin as input (for one, so that i can unify the test driver program
// 	 devac compiler by spawning devac from test per code section, not entire
// 	 file)

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
//typedef position_iterator<char const*> iterator_t;

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
		( "output,o", po::value<string>( &output ), "output filename" )
		( "input", po::value<string>( &input ), "input filename" )
		( "debug,d", "generate debugging information" )
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
	// output filename is either specified or input with .dvc extension
	if( !vm.count( "output" ) )
	{
		output = input;
		size_t pos = output.rfind( "." );
		if( pos != string::npos )
		{
			output.erase( pos );
		}
		output += ".dvc";
	}
	bool debug_info = false;
    if( vm.count( "debug" ) )
        debug_info = true;

	// get the filename to compile
	const char* input_filename = input.c_str();

	tree_parse_info<iterator_t, factory_t> info;

	// if the magic name 'stdin' is specified, read from stdin instead of file
	if( input == "stdin" )
	{
		// read from stdin instead of a file
		info = ParseFile( input, cin );
	}
	// if the input is a .dvc (compiled) file, de-compile instead
	else if( input.substr( input.size()-4, 4 ) == ".dvc" )
	{
		if( !DeCompileFile( input.c_str() ) )
		{
			cout << "Unable to de-compile " << input << endl;
			return -127;
		}
		else return 0;
	}	
	else
	{
		// open input file
		ifstream file;
		file.open( input_filename );
		if( !file.is_open() )
		{
			cout << "error opening " << input_filename << endl;
			exit( -1 );
		}
		// parse the file
		info = ParseFile( input, file );
		// close the file
		file.close();
	}

	// failed to fully parse the input?
	if( !info.full )
	{
		cout << "error parsing " << input << endl;
		return -1;
	}

	// check the semantics of the AST
	if( !CheckSemantics( info ) )
	{
		cout << "error checking semantics in " << input << endl;
		return -2;
	}

	// generate IL
	InstructionStream inst;
	if( !GenerateIL( info, inst, debug_info ) )
	{
		cout << "fatal error generating IL for " << input << endl;
		return -3;
	}

	// TODO: optimize IL (???)

	// generate final IL bytecode
	if( !GenerateByteCode( output.c_str(), inst ) )
	{
		cout << "fatal error generating bytecode for " << input << endl;
		return -4;
	}

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
				cout << "\t" << j->first;
				if( j->second.is_const )
					cout << " : constant";
				if( j->second.Type() == sym_address )
					cout << " : function";
				cout << endl;
			}
		}

		// dump the instruction stream
		cout << endl << "Instructions:" << endl;
		for( int i = 0; i < inst.size(); ++i )
		{
			cout << inst[i].op << " : ";
			// dump args (vector of DevaObjects) too (need >> op for Objects)
			for( vector<DevaObject>::iterator j = inst[i].args.begin(); j != inst[i].args.end(); ++j )
				cout << *j << " ; ";
			cout << endl;
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
