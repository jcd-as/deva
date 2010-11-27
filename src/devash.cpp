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

// devash.cpp
// deva language shell
// created by jcs, november 25, 2010 

// TODO:
// * command to exit?
// * better error messages?

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <utility>
#include <algorithm>

#include <histedit.h>

#include "compile.h"
#include "executor.h"
#include "util.h"

using namespace std;
namespace po = boost::program_options;

// the global scope table (used when compilation is needed, as in imports
// and dynamic code generation)
Scopes scopes;
int _argc;
char** _argv;

ostream & operator << ( ostream & os, Instruction & inst );

// prompt for the line editor
const char* prompt( EditLine * e )
{
	return ">>> ";
}

const char* continuation_prompt( EditLine * e )
{
	return "... ";
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
		int braces = count( ret.begin(), ret.end(), '{' ) - count( ret.begin(), ret.end(), '}' );
		if( (cstr_in[chars_read-2] == ';' || cstr_in[chars_read-2] == '}' ) && braces == 0 )
		{
			done = true;
			break;
		}

		el_set( el, EL_PROMPT, &continuation_prompt );
	}
	return ret;
}

int main( int argc, char** argv )
{
	_argc = argc;
	_argv = argv;
	try
	{
		// declare the command line options
		po::options_description desc( "Supported options" );
		desc.add_options()
			( "help", "help message" )
			( "version,ver", "display program version" )
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
			cout << "devadb " << VERSION << endl;
			return 1;
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

		// the execution engine
		Executor* ex = NULL;

		cout << "devash " << VERSION << endl;
		try
		{
			// create the execution engine object
			ex = new Executor();

			// create a global scope
			ex->StartGlobalScope();

			// add the built-in modules
			ex->AddAllKnownBuiltinModules();

			// get input
			// evaluate it

//			string input;
			bool done = false;
			while( !done )
			{
				// get the input
//				cstr_in = el_gets( el, &chars_read );
//				input = string( cstr_in, chars_read );
//				if( input.length() > 0 )
//				{
//					// store in history
//					history( hist, &ev, H_ENTER, input.c_str() );
//				}
				string input = get_input( el, hist, ev );
				// execute input as code
				try
				{
//					ex->RunText( cstr_in );
					ex->RunText( input.c_str() );
				}
				catch( DevaRuntimeException & e )
				{
					if( typeid( e ) == typeid( DevaICE ) )
						throw;
					cout << "Error: " << e.what() << endl;
					// dump the stack trace
					if( ex )
						ex->DumpTrace( cout, false );
				}
			}

			ex->EndGlobalScope();


			delete ex;
			ex = NULL;
		}
		catch( DevaRuntimeException & e )
		{
			if( typeid( e ) == typeid( DevaICE ) )
				throw;
			cout << "Error: " << e.what() << endl;
			// dump the stack trace
			if( ex )
				ex->DumpTrace( cout, false );
			cout << "program terminated. restart? (y/n)" << endl;
			// TODO: recover
			return -1;
		}

		// free the execution engine, if it hasn't been already
		delete ex;

		// Clean up line editor/history
		history_end( hist );
		el_end( el );

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
	catch( DevaCriticalException & e )
	{
		cout << "Unrecoverable error: " << e.what() << endl;
		return -1;
	}
	// runtime exceptions really should only stem from the Executor,
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


