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

// deva.cpp
// deva language executor, main program
// created by jcs, april 30, 2011

// TODO: 
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

// TODO: ms-windows ??
#include <histedit.h>

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


/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////

// prompt for the line editor
const char* prompt( EditLine * e )
{
	return ">>> ";
}

static int braces = 0;

const char* continuation_prompt( EditLine * e )
{
	string s( "... " );
	for( int i = 0; i < braces; i++ )
		s += " ";
	return s.c_str();
}

string get_input( EditLine *el, History *hist, HistEvent ev )
{
	int chars_read;
	const char* cstr_in;
	string ret;
	bool done = false;

	el_set( el, EL_PROMPT, &prompt );

	while( !done )
	{
		// read a line
		cstr_in = el_gets( el, &chars_read );
		if( !cstr_in )
		{
			// error / EOF: bail
			cout << endl;
			exit( 0 );
		}
		if( chars_read > 0 )
		{
			// store in history
			history( hist, &ev, H_ENTER, cstr_in );
		}

		// end on empty line
		if( chars_read == 1 && cstr_in[chars_read] == '\n' )
		{
			done = true;
			break;
		}

		ret += cstr_in;

		// get more input if:
		//  - line doesn't end with a ';' or a '}' OR
		//  - curly braces are open (mis-matched)
		braces = count( ret.begin(), ret.end(), '{' ) - count( ret.begin(), ret.end(), '}' );
		if( braces < 0 )
			break;
		if( (cstr_in[chars_read-2] == ';' || cstr_in[chars_read-2] == '}' ) && braces == 0 )
		{
			done = true;
			break;
		}

		el_set( el, EL_PROMPT, &continuation_prompt );
	}
	return ret;
}

// Main entry point for this example
//
int ANTLR3_CDECL main( int argc, char *argv[] )
{
	deva::_argc = argc;
	deva::_argv = argv;

	reftrace = false;
	bool trace = false;

	// declare the command line options
	po::options_description desc( "Supported options" );
	desc.add_options()
		( "help", "help message" )
		( "version,v", "display program version" )
		( "trace", "show execution trace" )
		;
	po::variables_map vm;
	try
	{
		po::parsed_options parsed = po::command_line_parser( argc, argv ).options( desc ).allow_unregistered().run();
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
		cout << "devash " << DEVA_VERSION << endl;
		return 1;
	}
	if( vm.count( "trace" ) )
	{
		trace = true;
	}

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

	cout << "devash " << DEVA_VERSION << endl;

	ex = new Executor();

	try
	{
		// set the file we're compiling
//		SetCurrentFile( fname.c_str() );

		// set execution flags
		ex->trace = trace;

		// add built-in (native) modules
		ex->AddNativeModule( GetModuleOs() );
		ex->AddNativeModule( GetModuleBit() );
		ex->AddNativeModule( GetModuleMath() );
		ex->AddNativeModule( GetModuleRe() );

		// begin execution (initialize)
		ex->Begin();

		bool done = false;
		while( !done )
		{
			// get the input
			string input = get_input( el, hist, ev );
			// execute input as code
			try
			{
				// TODO: ret will be the symbol name for this block 
				// (something like '[TEXT1]')
				// should we do something with it?
				ex->ExecuteText( input.c_str(), true, true );
			}
			catch( RuntimeException & e )
			{
				if( typeid( e ) == typeid( ICE ) )
					throw;
				cout << "error: " << e.what() << endl;
				// dump the stack trace
				if( ex )
					ex->DumpTrace( cerr );
			}
			catch( SemanticException & e )
			{
				// display an error
				emit_error( e );
			}
		}
		// end execution (shutdown)
		ex->End();
	}
	catch( ICE & e )
	{
		// display an error
		emit_error( e );
		exit( -1 );
	}
	catch( RuntimeException & e )
	{
		// display an error
		emit_error( e );

		// dump the stack trace
		if( ex )
			ex->DumpTrace( cerr );

		exit( -1 );
	}

	// free the execution engine
	delete ex;

	return 0;
}

