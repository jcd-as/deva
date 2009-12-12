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

// module_os.h
// built-in module 'os' for the deva language 
// created by jcs, november 6, 2009 

// TODO:
// * remove, rename


#include "module_os.h"
#include "util.h"
#include <cstdlib>
#include <boost/filesystem.hpp>

using namespace boost;

extern char** environ;

void do_os_exec( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'exec'." );

	// command line to exec is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'command' argument in module 'os' function 'exec'" );
	}
	if( !o )
		o = &obj;

	// ensure command is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'command' argument to module 'os' function 'exec' must be a string." );

	int ret = std::system( o->str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_os_getcwd( Executor* ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Module 'os' function 'getcwd' takes no arguments." );

	string ret = get_cwd();

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_chdir( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'chdir'." );

	// dir is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'dir' argument in module 'os' function 'chdir'" );
	}
	if( !o )
		o = &obj;

	// ensure dir is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'dir' argument to module 'os' function 'chdir' must be a string." );

	bool ret = true;
	if( chdir( o->str_val ) != 0 ) 
		ret = false;

	// pop the return address
	ex->stack.pop_back();

	// return true on success/false on failure
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_splitpath( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'splitpath'." );

	// path is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'path' argument in module 'os' function 'splitpath'" );
	}
	if( !o )
		o = &obj;

	// ensure path is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'path' argument to module 'os' function 'splitpath' must be a string." );

	string path( o->str_val );
	DOVector* ret = new DOVector();
	vector<string> v( split_path( path ) );
	for( vector<string>::iterator i = v.begin(); i != v.end(); ++i )
	{
		ret->push_back( DevaObject( "", *i ) );
	}

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_joinpaths( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'joinpaths'." );

	// path is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'paths' argument in module 'os' function 'joinpaths'" );
	}
	if( !o )
		o = &obj;

	// ensure arg is a vector
	if( o->Type() != sym_vector )
		throw DevaRuntimeException( "'paths' argument to module 'os' function 'joinpaths' must be a vector." );

	vector<string> v;
	for( int i = 0; i < o->vec_val->size(); ++i )
	{
		DevaObject* s;
		DevaObject so = o->vec_val->at( i );
		if( so.Type() == sym_unknown )
			s = ex->find_symbol( so );
		else
			s = &so;

		if( s->Type() != sym_string )
			throw DevaRuntimeException( "'paths' argument to module 'os' function 'joinpaths' must be a vector containing only strings." );

//		v.push_back( o->vec_val->at( i ).str_val );
		v.push_back( s->str_val );
	}
	string ret = join_paths( v );

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_getdir( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'getdir'." );

	// path is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'path' argument in module 'os' function 'getdir'" );
	}
	if( !o )
		o = &obj;

	// ensure path is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'path' argument to module 'os' function 'getdir' must be a string." );

	string path( o->str_val );
	string ret = get_dir_part( path );

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_getfile( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'getfile'." );

	// path is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'path' argument in module 'os' function 'getfile'" );
	}
	if( !o )
		o = &obj;

	// ensure path is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'path' argument to module 'os' function 'getfile' must be a string." );

	string path( o->str_val );
	string ret = get_file_part( path );

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_getext( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'getext'." );

	// path is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'path' argument in module 'os' function 'getext'" );
	}
	if( !o )
		o = &obj;

	// ensure path is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'path' argument to module 'os' function 'getext' must be a string." );

	string path( o->str_val );
	string ret = get_extension( path );

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_exists( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'exists'." );

	// path is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'path' argument in module 'os' function 'exists'" );
	}
	if( !o )
		o = &obj;

	// ensure path is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'path' argument to module 'os' function 'exists' must be a string." );

	string path( o->str_val );
	bool ret = exists( path );

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", ret ) );
}


void do_os_environ( Executor* ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Module 'os' function 'environ' takes no arguments." );

	// create a map of the environment
	DOMap* m = new DOMap();
//	for( int i = 0; i < sizeof(environ)/sizeof(char*); ++i )
	int i = 0;
	while( environ[i] )
	{
		// look for the '='
		char* pos = strchr( environ[i], '=' );
		if( !pos )
			throw DevaRuntimeException( "Invalid Operating System environment. Memory corruption or other critical error likely." );
		// divide into left and right sides
		string envvar( environ[i], (size_t)pos - (size_t)environ[i] );
		string value( pos+1 );
		m->insert( make_pair( DevaObject( "", envvar ), DevaObject( "", value ) ) );
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the environment map
	ex->stack.push_back( DevaObject( "", m ) );
}

void do_os_getenv( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'getenv'." );

	// name of the env var to look for is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'variable_name' argument in module 'os' function 'getenv'" );
	}
	if( !o )
		o = &obj;

	// ensure variable name is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'variable_name' argument to module 'os' function 'getenv' must be a string." );

	string ret;
	char* var = getenv( o->str_val );
	if( !var )
		ret = "";
	else
		ret = var;

	// pop the return address
	ex->stack.pop_back();

	// return the environment variable's value
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_argv( Executor* ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Module 'os' function 'argv' takes no arguments." );

	extern int _argc; 
	extern char** _argv;
	DOVector* ret = new DOVector();
	for( int i = 1; i < _argc; ++i )
	{
		ret->push_back( DevaObject( "", string( _argv[i] ) ) );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_os_dirwalk( Executor* ex )
{
	if( Executor::args_on_stack != 1 && Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'dirwalk'." );

	// starting dir is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'dir' argument in module 'os' function 'dirwalk'" );
	}
	if( !o )
		o = &obj;

	// ensure dir is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'dir' argument to module 'os' function 'dirwalk' must be a string." );

    // if there's a second arg, it's a boolean indicating whether to recurse or
    // not (default being 'true')
    bool recurse = true;
    if( Executor::args_on_stack == 2 )
    {
        DevaObject recur = ex->stack.back();
        ex->stack.pop_back();

        DevaObject* r = NULL;
        if( recur.Type() == sym_unknown )
        {
            r = ex->find_symbol( recur );
            if( !r )
                throw DevaRuntimeException( "Symbol not found for 'recur' argument in module 'os' function 'dirwalk'" );
        }
        if( !r )
            r = &recur;
        // must be a boolean
        if( r->Type() != sym_boolean )
            throw DevaRuntimeException( "'recur' argument to module 'os' function 'dirwalk' must be a boolean." );
        recurse = r->bool_val;
    }

	DevaObject ret;
	DOVector* dw_data = new DOVector();

    // ensure dir exists and is a directory
	filesystem::path p( o->str_val );
	if( !filesystem::exists( p ) )
		ret = DevaObject( "", sym_null );
    else if( !filesystem::is_directory( p ) )
		ret = DevaObject( "", sym_null );
	else
	{
        if( recurse )
        {
            filesystem::recursive_directory_iterator end;
            for( filesystem::recursive_directory_iterator i( p ); i != end; ++i )
            {
                if( filesystem::is_regular_file( i->status() ) )
                    dw_data->push_back( DevaObject( "", i->path().string() ) );
            }
        }
        else
        {
            filesystem::directory_iterator end;
            for( filesystem::directory_iterator i( p ); i != end; ++i )
            {
                if( filesystem::is_regular_file( i->status() ) )
                    dw_data->push_back( DevaObject( "", i->path().string() ) );
            }
        }
		ret = DevaObject( "", dw_data );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( ret );
}

void AddOsModule( Executor* ex )
{
	map<string, builtin_fcn> fcns = map<string, builtin_fcn>();
	fcns.insert( make_pair( string( "exec@os" ), do_os_exec ) );
	fcns.insert( make_pair( string( "getcwd@os" ), do_os_getcwd ) );
	fcns.insert( make_pair( string( "chdir@os" ), do_os_chdir ) );
	fcns.insert( make_pair( string( "splitpath@os" ), do_os_splitpath ) );
	fcns.insert( make_pair( string( "joinpaths@os" ), do_os_joinpaths ) );
	fcns.insert( make_pair( string( "getdir@os" ), do_os_getdir ) );
	fcns.insert( make_pair( string( "getfile@os" ), do_os_getfile ) );
	fcns.insert( make_pair( string( "getext@os" ), do_os_getext ) );
	fcns.insert( make_pair( string( "exists@os" ), do_os_exists ) );
	fcns.insert( make_pair( string( "environ@os" ), do_os_environ ) );
	fcns.insert( make_pair( string( "getenv@os" ), do_os_getenv ) );
	fcns.insert( make_pair( string( "argv@os" ), do_os_argv ) );
	fcns.insert( make_pair( string( "dirwalk@os" ), do_os_dirwalk ) );
	ex->ImportBuiltinModuleFunctions( string( "os" ), fcns );
}
