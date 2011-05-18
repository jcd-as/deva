// Copyright (c) 2011 Joshua C. Shepard
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
// created by jcs, may 14, 2011 

// TODO:
// * function breakpoints (fcn name)
// * handle out-of-memory conditions (allocating text buffer, SymbolTable*s)
// * break on program error (exception) ???

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <utility>

#include <histedit.h>

//#include "compile.h"
#include "executor.h"
#include "util.h"
#include "api.h"
#include "module_os.h"
#include "module_bit.h"
#include "module_math.h"
#include "module_re.h"

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
extern int _argc;
extern char** _argv;
}


/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////

//ostream & operator << ( ostream & os, Instruction & inst );

void ReadFile( const char* const filename, vector<string> & vec )
{
	FILE* file = fopen( filename, "r" );
	if( !file )
		return;

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
		// we don't leak the extra bytes when they are assigned to a string Object
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
			throw RuntimeException( "Error accessing source file." );
	}
}

void ShowLine( vector<string> & lines, string file, int line )
{
	cout << "[" << file << ":" << line << "] " << lines[line-1];
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
	"breakpoint",
	"delete breakpoint",
	"display breakpoints",
	"run",
	"list",
	"eval",
	"trace",
	"stack",
	"current execution address",
	"help",
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
	'b',
	'd',
	'y',
	'r',
	'l',
	'e',
	't',
	'k',
	'x',
	'?',
};

char get_command( string & in )
{
	// first check if it matches a single-character shortcut
	if( in.length() == 1 )
	{
		for( size_t i = 0; i < sizeof( command_shortcuts ); ++i )
		{
			if( in[0] == command_shortcuts[i] )
				return command_shortcuts[i];
		}
	}
	// if not, look for the most-matching full command name
	int match_idx = -1;
	size_t greatest_match = 0;
	for( size_t i = 0; i < sizeof( commands ) / sizeof( char* ); ++i )
	{
		size_t j = 0;
		size_t len = strlen( commands[i] );
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

void display_help()
{
	cout << "commands and keyboard shortcuts:" << endl;
	for( size_t i = 0; i < sizeof( commands ) / sizeof( char* ); ++i )
	{
		cout << command_shortcuts[i] << ":\t";
		cout << commands[i] << endl;
	}
}

// prompt for the line editor
const char* prompt( EditLine * e )
{
	return "(devadb) ";
}

string GetExecutingFile()
{
	Frame* frame = ex->CurrentFrame();
	if( !frame )
		throw ICE( "nothing on callstack." );
	Function* fcn = frame->GetFunction();
	if( !fcn )
		throw ICE( "Non-function frame on top of callstack." );
	return fcn->filename;
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
		vector<string> inputs;
		po::options_description desc( "Supported options" );
		desc.add_options()
			( "help", "help message" )
			( "version,ver", "display program version" )
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

		if( ext != ".dv" )
		{
			cout << "error: input must be .dv source file" << endl;
			return -1;
		}

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

		// set the file we're compiling
		SetCurrentFile( fname.c_str() );

		string out_fname = fname + "c";

		ex = new Executor();

		// compile the file
		PassOneFlags p1flags;
		PassTwoFlags p2flags;
		Code* code = Compile( fname.c_str(), p1flags, p2flags );
		ex->AddCode( code );

		if( !code )
		{
			cout << "Error compiling " << fname << endl;
			return -1;
		}
		// unless we're not writing .dvc files
		if( !no_dvc )
		{
			// write the .dvc file
			ex->WriteCode( out_fname, code );
		}

		// read the source file
		map<string, vector<string> > files;
		string filepart = get_file_part( fname );
		ReadFile( fname.c_str(), files[filepart] );

		// line editor objects
		EditLine* el = NULL;
		History* hist = NULL;
		HistEvent ev;

		// set-up the line editor and history
		el = el_init( argv[0], stdin, stdout, stderr );
		el_set( el, EL_PROMPT, &prompt );
		el_set( el, EL_EDITOR, "emacs" );
		hist = history_init();
		if( hist == 0 )
		{
			cerr <<  "Error: Line editor history could not be initialized" << endl;
			return 1;
		}
		history( hist, &ev, H_SETSIZE, 800 );
		el_set( el, EL_HIST, history, hist );

		// for saving/re-loading breakpoints between restarts
		vector<pair<string, int> > breakpoints;

		cout << "devadb " << DEVA_VERSION << endl;
re_start:
		try
		{
			bool running = false;

// 'run' (restart) starts HERE
start:
			{
				// create the execution engine object
				if( !ex )
				{
					ex = new Executor();
					// add the code block
					code = ex->ReadCode( out_fname );
					ex->AddCode( code );
				}

				// initialize execution engine
				ex->AddNativeModule( GetModuleOs() );
				ex->AddNativeModule( GetModuleBit() );
				ex->AddNativeModule( GetModuleMath() );
				ex->AddNativeModule( GetModuleRe() );
				ex->Begin( code );


				// if we have saved breakpoints, re-load them
				for( size_t i = 0; i < breakpoints.size(); ++i )
				{
//					ex->AddBreakpoint( breakpoints[i].first, breakpoints[i].second );
				}

				// run
				char c = 0;
				while( true )
				{
					if( running )
					{
						cout << "restarting program..." << endl;
						ex->BeginExecution();
					}
					string input;
					int chars_read;
					const char* cstr_in;
					vector<string> in;
					int l = 1;
					bool done = false;
					while( !done )
					{
						// get the input, sans newline
						cstr_in = el_gets( el, &chars_read );
						// if we got NULL input, bail
						if( !cstr_in )
						{
							cout << endl;
							exit( 0 );
						}
						input = string( cstr_in, chars_read-1 );
						if( input.length() > 0 )
						{
							in.clear();
							split( input, " ", in );
							c = get_command( in[0] );

							// store in history
							history( hist, &ev, H_ENTER, input.c_str() );
						}
						if( c == 0 )
							cout << "Unknown command" << endl;
						else if( !running && c != 'r' && c != 'q' && c != '?' )
						{
							cout << "program is not running. use the 'run' command to begin." << endl;
						}
						else
						{
							// otherwise, repeat the last command
							switch( c )
							{
							// 'run' (restart)
							case 'r':
								// start
								if( !running )
								{
									cout << "starting program..." << endl;
									ex->BeginExecution();
									running = true;
								}
								// restart
								else
									done = true;
								break;
							// 'quit'
							case 'q':
								done = true;
								break;
							// 'next'
							case 'n':
								l = ex->StepOver();
								break;
							// 'step'
							case 's':
//								l = ex->StepInto();
								break;
							// 'next instruction'
							case 'i':
//								{
//								// StepInst returns 0 if this wasn't a linenum op
//								Instruction inst;
//								int il = ex->StepInst( inst );
//								if( il != 0 )
//									l = il;
//								cout << inst << endl;
//								}
								break;
							// 'continue'
							case 'c':
								{
								l = ex->ContinueExecution();
								}
								break;
							// 'print'
							case 'p':
//								{
//								// verify there are sufficient args
//								if( in.size() < 2 )
//								{
//									cout << "print command requires variable name or expression." << endl;
//									break;
//								}
//								// if not a simple variable, eval the args and try printing the result
//								Object* v = ex->find_symbol( Object( in[1].c_str(), sym_unknown ) );
//								if( !v )
//								{
//									// strip the command off the input
//									size_t idx = input.find( ' ' );
//									string code( input, idx );
//									code = string( "print( " ) + code + string( " );" );
//									int stack_depth = ex->stack.size();
//									// execute the rest as code
//									char* s = new char[code.length() + 1];
//									s[code.length()] = '\0';
//									memcpy( s, code.c_str(), code.length() );
//									try
//									{
//										ex->RunText( s );
//									}
//									catch( RuntimeException & e )
//									{
//										if( typeid( e ) == typeid( ICE ) )
//											throw;
//										cout << "Error: " << e.what() << endl;
//										cout << "Cannot evaluate '" << string( input, idx ) << "'." << endl;
//									}
//								}
//								// otherwise, it is a simple var, just print it
//								else
//								{
//									if( v->Type() == sym_string )
//										cout << "'" << *v << "'" << endl;
//									else
//										cout << *v << endl;
//								}
//								}
								break;
							// 'breakpoint':
							case 'b':
								// verify there are sufficient args
//								if( in.size() == 1 )
//								{
//									// no args: add a breakpoint on the current file & line
//									ex->AddBreakpoint( GetExecutingFile(), l );
//									break;
//								}
//								else if( in.size() == 2 )
//								{
//									// only one arg. is it a number? try to use
//									// it as a line number to set a bp in the
//									// current file
//									ex->AddBreakpoint( GetExecutingFile(), atoi( in[1].c_str() ) );
//									break;
//								}
//								else if( in.size() < 3 )
//								{
//									cout << "add breakpoint command requires filename and line number." << endl;
//									break;
//								}
//								ex->AddBreakpoint( in[1], atoi( in[2].c_str() ) );
								break;
							// 'delete breakpoint':
							case 'd':
//								if( in.size() < 2 )
//								{
//									cout << "delete breakpoint command requires index of breakpoint to remove." << endl;
//									break;
//								}
//								ex->RemoveBreakpoint( atoi( in[1].c_str() ) );
								break;
							// 'display breakpoints':
							case 'y':
//								{
//								vector<pair<string, int> > bpoints = ex->GetBreakpoints();
//								cout << "breakpoints:" << endl;
//								for( int i = 0; i < bpoints.size(); ++i )
//								{
//									cout << i << ": " << bpoints[i].first << ", line " << bpoints[i].second << endl;
//								}
//								break;
//								}
								break;
							// 'list' code:
							case 'l':
								{
								// if we're in a new file, make sure we've read it for
								// display
								string file = GetExecutingFile();

								if( files.find( file ) == files.end() )
									ReadFile( file.c_str(), files[file] );
								// display 10 lines of code around the current line
								int start = l;
									if( start < 5 )
									start = 0;
								else
									start -= 5;
								size_t end = start + 10;
								if( end > files[file].size() )
									end = files[file].size();
								for( size_t c = start; c < end; ++c )
									ShowLine( files[file], file, c );
								break;
								}
							// 'eval'
							case 'e':
//								{
//								// strip the command off the input
//								size_t idx = input.find( ' ' );
//								string code( input, idx );
//								// execute this as code
//								char* s = new char[code.length() + 1];
//								s[code.length()] = '\0';
//								memcpy( s, code.c_str(), code.length() );
//								try
//								{
//									ex->RunText( s );
//								}
//								catch( RuntimeException & e )
//								{
//									if( typeid( e ) == typeid( ICE ) )
//										throw;
//									cout << "Error: " << e.what() << endl;
//								}
//								}
								break;
							// back trace
							case 't':
								ex->DumpTrace( cout );
								break;
							// stack
							case 'k':
								{
								// display the last 10 items on the stack
								cout << "Program data stack (top-of-stack first):" << endl;
								ex->DumpStackTop( 20 );
								}
								break;
							// executing address (ip)
							case 'x':
								cout << ex->GetIP() << endl;
								break;
							// help
							case '?':
								display_help();
								break;
							}
							if( l == -1 )
							{
								c = 'q';
								break;
							}

							// if we're in a new file, make sure we've read it for
							// display
							string file = GetExecutingFile();
							if( file.length() != 0 )
							{
								if( files.find( file ) == files.end() )
									ReadFile( file.c_str(), files[file] );
								ShowLine( files[file], file, l );
							}
						}
					}
					if( c == 'q' || c == 'r' )
						break;
				}

//				ex->EndGlobalScope();

				// save breakpoints
				breakpoints.clear();
//				breakpoints = ex->GetBreakpoints();


				ex->End();
				delete ex;
				ex = NULL;

				if( c == 'r' )
					goto start;

				cout << "program terminated. restart? (y/n)" << endl;
				char c2 = getchar();
				if( c2 == 'y' )
				{
					// remove the newline
					getchar();

					// restart
					goto start;
				}
			}
		}
		catch( RuntimeException & e )
		{
			if( typeid( e ) == typeid( ICE ) )
				throw;
			cout << "Error: " << e.what() << endl;
			// dump the stack trace
			if( ex )
				ex->DumpTrace( cout );
			cout << "program terminated. restart? (y/n)" << endl;
			char c2 = getchar();
			if( c2 == 'y' )
			{
				// remove the newline
				getchar();

				// save breakpoints
				breakpoints.clear();
//				breakpoints = ex->GetBreakpoints();

				// restart
				goto re_start;
			}
			else
				return -1;
		}

		// free the execution engine, if it hasn't been already
		delete ex;

		// Clean up line editor/history
		history_end( hist );
		el_end( el );

		// done, free the scope tables
//		for( Scopes::iterator i = scopes.begin(); i != scopes.end(); ++i )
//		{
//			delete i->second;
//		}
	}
	catch( ICE & e )
	{
		cout << "Internal compiler error: " << e.what() << endl;
		return -1;
	}
//	catch( CriticalException & e )
//	{
//		cout << "Unrecoverable error: " << e.what() << endl;
//		return -1;
//	}
	// runtime exceptions really should only stem from the Executor,
	catch( RuntimeException & e )
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


