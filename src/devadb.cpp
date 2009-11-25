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

// devadb.cpp
// deva language debugger
// created by jcs, november 23, 2009 

// TODO:
// * 'next' currently does a step-INTO, not step-OVER
// * breakpoints
// * data eval
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

ostream & operator << ( ostream & os, Instruction & inst );

void ReadFile( const char* const filename, vector<string> & vec )
{
	FILE* file = fopen( filename, "r" );

	// keep reading lines until we reach the EOF
	while( true )
	{
		// allocate space for some bytes 
		const size_t BUF_SZ = 10;
		char* buffer = new char[BUF_SZ];
		buffer[0] = '\0';
		// track the original start of the buffer
		size_t count = BUF_SZ;	// number of bytes read so far
		char* buf = buffer;
		while( fgets( buf, BUF_SZ, file ) )
		{
			// read was valid, was the last char read a newline?
			// if so, we're done
			size_t len = strlen( buf );
			if( buf[len-1] == '\n' )
				break;
			// if not, 
			//  - allocate twice as much space
			char* new_buf = new char[count + BUF_SZ];
			//  - copy what was read, minus the null
			memcpy( (void*)new_buf, (void*)buffer, count - 1 );
			//  - free the orginal memory
			delete [] buffer;
			//  - continue
			buffer = new_buf;
			buf = buffer + count-1;
			count += BUF_SZ - 1;
		}
		// if we didn't fill the buffer, we need to re-alloc and copy so that
		// we don't leak the extra bytes when they are assigned to a string DevaObject
		size_t bytes_read = strlen( buffer );
		if( bytes_read != count )
		{
			char* new_buf = new char[bytes_read + 1];
			new_buf[bytes_read] = '\0';
			memcpy( (void*)new_buf, (void*)buffer, bytes_read );
			delete [] buffer;
			buffer = new_buf;
		}

		// add this line to our output vector
		vec.push_back( string( buffer ) );
		delete [] buffer;

		// done?
		if( feof( file ) )
			break;
		// was there an error??
		if( ferror( file ) )
			throw DevaRuntimeException( "Error accessing source file." );
	}
}

void ShowLine( vector<string> & lines, int line )
{
	cout << "[" << line << "] " << lines[line-1];
}

void split( string & in, vector<string> & ret )
{
	size_t left = in.find_first_not_of( " " );
	if( left != string::npos )
	{	
		size_t right = in.find_first_of( " " );
		size_t len = in.length();
		while( left != string::npos )
		{
			string s( in, left, right - left );
			ret.push_back( s );

			left = in.find_first_not_of( " ", right );
			right = in.find_first_of( " ", right + 1 );
			// if 'left' is greater than 'right', then we passed an empty string 
			// (two matching split chars in a row), enter it and move forward
			if( left > right )
			{
				ret.push_back( string( "" ) );
				right = in.find_first_of( " ", right + 1 );
			}
		}
	}
}

const char* commands[] = 
{
	"next",
	"next instruction",
	"step in",
	"step out",
	"quit",
	"continue",
	"print",
};
char command_shortcuts[] = 
{
	'n',
	'i',
	's',
	'o',
	'q',
	'c',
	'p',
};

char get_command( string & in )
{
	// first check if it matches a single-character shortcut
	if( in.length() == 1 )
	{
		for( int i = 0; i < sizeof( command_shortcuts ); ++i )
		{
			if( in[0] == command_shortcuts[i] )
				return command_shortcuts[i];
		}
	}
	// if not, look for the most-matching full command name
	int match_idx = -1;
	int greatest_match = 0;
	for( int i = 0; i < sizeof( commands ) / sizeof( char* ); ++i )
	{
		int j = 0;
		int len = strlen( commands[i] );
		len = len > in.length() ? len : in.length();
		while( j < len )
		{
			if( commands[i][j] != in[j] )
				break;
			++j;
		}
		if( j > greatest_match )
		{
			greatest_match = j;
			match_idx = i;
		}
	}
	if( match_idx == -1 )
		return 0;
	else
		return command_shortcuts[match_idx];
}

int main( int argc, char** argv )
{
	_argc = argc;
	_argv = argv;
	try
	{
		// declare the command line options
		bool no_dvc = false;
		string output;
		string input;
		po::options_description desc( "Supported options" );
		desc.add_options()
			( "help", "help message" )
			( "version,ver", "display program version" )
			( "no-dvc", "do NOT write a .dvc compiled byte-code file to disk" )
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
			cout << "devadb " << VERSION << endl;
			return 1;
		}
		if( vm.count( "no-dvc" ) )
		{
			no_dvc = true;
		}
		// must be an input file specified
		if( !vm.count( "input" ) )
		{
			cout << "usage: devadb [options] <input_file>" << endl;
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

		unsigned char* code;
		string out_fname = fname + "c";
		size_t code_length;
		code = CompileFile( fname.c_str(), code_length );
		if( !code )
		{
			cout << "Error compiling " << fname << endl;
			return -1;
		}
		// unless we're not writing .dvc files
		if( !no_dvc )
		{
			// write the .dvc file
			if( !WriteByteCode( out_fname.c_str(), code, code_length ) )
			{
				cout << "Error writing bytecode to " << out_fname << endl;
				return -1;
			}
		}

		// TODO: handle multiple source files
		// (map filenames to vectors)
		//
		// read the source file
		vector<string> lines;
		ReadFile( fname.c_str(), lines );

		// create our execution engine object
		Executor ex;

		try
		{
			// create a global scope
			ex.StartGlobalScope();

			// add the built-in modules
			AddOsModule( ex );
			AddBitModule( ex );
			AddMathModule( ex );

			// add the code block we just compiled
			ex.AddCodeBlock( code );

			// run
			cout << "devadb " << VERSION << endl;
			cout << "starting program..." << endl;
			while( true )
			{
				ex.StartExecutingCode( code );
				string input;
				vector<string> in;
				char c = 0;
				int l = 1;
				bool done = false;
				while( !done )
				{
					getline( cin, input );
					if( input.length() > 0 )
					{
						split( input, in );
						c = get_command( in[0] );
						in.clear();
					}
					if( c == 0 )
						cout << "Unknown command" << endl;
					else
					{
						// otherwise, repeat the last command
						switch( c )
						{
						// 'quit'
						case 'q':
							done = true;
							break;
						// 'next'
						case 'n':
							l = ex.StepOver();
							break;
						// 'step'
						case 's':
							l = ex.StepInto();
							break;
						// 'next instruction'
						case 'i':
							{
							// StepInst returns 0 if this wasn't a linenum op
							Instruction inst;
							int il = ex.StepInst( inst );
							if( il != 0 )
								l = il;
							cout << inst << endl;
							}
							break;
						// 'continue'
						case 'c':
							l = ex.Run();
							break;
						// 'print'
						case 'p':
							{
							DevaObject* v = ex.find_symbol( DevaObject( in[1].c_str(), sym_unknown ) );
							if( !v )
								cout << "Cannot locate variable '" << in[1].c_str() << "'." << endl;
							else
								cout << *v << endl;
							}
							break;
						}
						if( l == -1 )
							break;

						ShowLine( lines, l );
					}
				}
				cout << "program terminated. restart? (y/n)" << endl;
				char c2 = getchar();
				if( c2 != 'y' )
					break;
				// remove the newline
				getchar();
			}

			ex.EndGlobalScope();
		}
		catch( DevaRuntimeException & e )
		{
			if( typeid( e ) == typeid( DevaICE ) )
				throw;
			cout << "Error: " << e.what() << endl;
			// dump the stack trace
			ex.DumpTrace( cout, false );
			return -1;
		}

		// done, free the scope tables
		for( Scopes::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			delete i->second;
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

