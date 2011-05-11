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

// module_os.cpp
// native os module for deva language, v2
// created by jcs, april 2, 2011

// TODO:
// * 

#include "module_os.h"
#include "builtins_helpers.h"
#include "util.h"
#include "module.h"
#include <cstdlib>
#include <boost/filesystem.hpp>

using namespace boost;

// mac/bsd doesn't like environ being accessed from a shared lib
#ifdef MAC_OS_X
	#include <crt_externs.h>
#else
	// ms-windows needs direct.h and a dllimport decl
	#ifdef MS_WINDOWS
		#define chdir(x) _chdir(x)
		#include <direct.h>
		extern __declspec(dllimport) char** environ;
	// linux just needs the extern decl
	#else
		extern char** environ;
	#endif
#endif

namespace deva
{


const string module_os_names[] =
{
	string( "exec" ),
	string( "getcwd" ),
	string( "chdir" ),
	string( "splitpath" ),
	string( "joinpaths" ),
	string( "getdir" ),
	string( "getfile" ),
	string( "getext" ),
	string( "exists" ),
	string( "environ" ),
	string( "getenv" ),
	string( "argv" ),
	string( "dirwalk" ),
	string( "isdir" ),
	string( "isfile" ),
	string( "sep" ),
	string( "extsep" ),
	string( "curdir" ),
	string( "pardir" ),
	string( "pathsep" )
};
	
NativeFunction module_os_fcns[] = 
{
	{do_os_exec, false},
	{do_os_getcwd, false},
	{do_os_chdir, false},
	{do_os_splitpath, false},
	{do_os_joinpaths, false},
	{do_os_getdir, false},
	{do_os_getfile, false},
	{do_os_getext, false},
	{do_os_exists, false},
	{do_os_environ, false},
	{do_os_getenv, false},
	{do_os_argv, false},
	{do_os_dirwalk, false},
	{do_os_isdir, false},
	{do_os_isfile, false},
	{do_os_sep, false},
	{do_os_extsep, false},
	{do_os_curdir, false},
	{do_os_pardir, false},
	{do_os_pathsep, false}
};

const int num_of_module_os_fcns = sizeof( module_os_names ) / sizeof( module_os_names[0] );


// is this name a module os function?
bool IsModuleOsFunction( const string & name )
{
	const string* i = find( module_os_names, module_os_names + num_of_module_os_fcns, name );
	if( i != module_os_names + num_of_module_os_fcns ) return true;
		else return false;
}

NativeModule* GetModuleOs()
{
	return new NativeModule( "os", module_os_fcns, module_os_names, num_of_module_os_fcns );
}

/////////////////////////////////////////////////////////////////////////////
// module os functions
/////////////////////////////////////////////////////////////////////////////
void do_os_exec( Frame* f )
{
	BuiltinHelper helper( "os", "exec", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	int ret = std::system( o->s );

	helper.ReturnVal( Object( (double)ret ) );
}

void do_os_getcwd( Frame* f )
{
	BuiltinHelper helper( "os", "getcwd", f, true );
	helper.CheckNumberOfArguments( 0 );

	string retstr = get_cwd();

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_chdir( Frame* f )
{
	BuiltinHelper helper( "os", "chdir", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	bool ret = true;
	if( chdir( o->s ) != 0 ) 
		ret = false;

	helper.ReturnVal( Object( ret ) );
}

void do_os_splitpath( Frame* f )
{
	BuiltinHelper helper( "os", "splitpath", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	string path( o->s );

	// return vector
	Vector* ret = CreateVector();

	vector<string> v( split_path( path ) );
	for( vector<string>::iterator i = v.begin(); i != v.end(); ++i )
	{
		const char* s = f->GetParent()->AddString( *i );
		ret->push_back( Object( s ) );
	}

	helper.ReturnVal( Object( ret ) );
}

void do_os_joinpaths( Frame* f )
{
	BuiltinHelper helper( "os", "joinpaths", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_vector );

	vector<string> v;
	for( size_t i = 0; i < o->v->size(); ++i )
	{
		Object* s;
		Object so = o->v->at( i );
//		if( so.Type() == sym_unknown )
//			s = ex->find_symbol( so );
//		else
			s = &so;

		if( s->type != obj_string )
			throw RuntimeException( "'paths' argument to module 'os' function 'joinpaths' must be a vector containing only strings." );

		v.push_back( s->s );
	}
	string retstr = join_paths( v );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_getdir( Frame* f )
{
	BuiltinHelper helper( "os", "getdir", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	string path( o->s );
	string retstr = get_dir_part( path );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_getfile( Frame* f )
{
	BuiltinHelper helper( "os", "getfile", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	string path( o->s );
	string retstr = get_file_part( path );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_getext( Frame* f )
{
	BuiltinHelper helper( "os", "getext", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	string path( o->s );
	string retstr = get_extension( path );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_exists( Frame* f )
{
	BuiltinHelper helper( "os", "exists", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	string path( o->s );
	bool ret = exists( path );

	helper.ReturnVal( Object( ret ) );
}

void do_os_environ( Frame* f )
{
	BuiltinHelper helper( "os", "environ", f, true );
	helper.CheckNumberOfArguments( 0 );

#ifdef MAC_OS_X
	// mac/bsd doesn't like environ being accessed from a shared lib
	char** environ = *_NSGetEnviron();
#endif

	// create a map of the environment
	Map* m = CreateMap();
	int i = 0;
	while( environ[i] )
	{
		// look for the '='
		char* pos = strchr( environ[i], '=' );
		if( !pos )
			throw RuntimeException( "Invalid Operating System environment. Memory corruption or other critical error likely." );
		// divide into left and right sides
		string envvar( environ[i], (size_t)pos - (size_t)environ[i] );
		const char* s = f->GetParent()->AddString( envvar );
		string value( pos+1 );
		const char* s2 = f->GetParent()->AddString( value );
		m->insert( make_pair( Object( s ), Object( s2 ) ) );
		++i;
	}

	helper.ReturnVal( Object( m ) );
}

void do_os_getenv( Frame* f )
{
	BuiltinHelper helper( "os", "getenv", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	string retstr;
	char* var = getenv( o->s );
	if( !var )
		retstr = "";
	else
		retstr = var;

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_argv( Frame* f )
{
	BuiltinHelper helper( "os", "argv", f, true );
	helper.CheckNumberOfArguments( 0 );

	extern int _argc; 
	extern char** _argv;
	Vector* ret = CreateVector();
	for( int i = 1; i < _argc; ++i )
	{
		const char* s = f->GetParent()->AddString( string( _argv[i] ) );
		ret->push_back( Object( s ) );
	}

	helper.ReturnVal( Object( ret ) );
}

void do_os_dirwalk( Frame* f )
{
	BuiltinHelper helper( "os", "dirwalk", f, true );
	helper.CheckNumberOfArguments( 1, 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	bool recurse = true;
	// optional second arg is whether to recurse or not
	if( f->NumArgsPassed() == 2 )
	{
		Object* recur = helper.GetLocalN( 1 );
		helper.ExpectType( recur, obj_boolean );
		recurse = recur->b;
	}

	Object ret;
	Vector* dw_data;

	// ensure dir exists and is a directory
	filesystem::path p( o->s );
	if( !filesystem::exists( p ) )
		ret = Object( obj_null );
	else if( !filesystem::is_directory( p ) )
		ret = Object( obj_null );
	else
	{
		dw_data = CreateVector();
		if( recurse )
		{
			filesystem::recursive_directory_iterator end;
			for( filesystem::recursive_directory_iterator i( p ); i != end; ++i )
			{
				if( filesystem::is_regular_file( i->status() ) )
				{
					const char* s = f->GetParent()->AddString( i->path().string() );
					dw_data->push_back( Object( s ) );
				}
			}
		}
		else
		{
			filesystem::directory_iterator end;
			for( filesystem::directory_iterator i( p ); i != end; ++i )
			{
				if( filesystem::is_regular_file( i->status() ) )
				{
					const char* s = f->GetParent()->AddString( i->path().string() );
					dw_data->push_back( Object( s ) );
				}
			}
		}
		ret = Object( dw_data );
	}

	helper.ReturnVal( ret );
}

void do_os_isdir( Frame* f )
{
	BuiltinHelper helper( "os", "isdir", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	filesystem::path path( o->s );
	bool ret = is_directory( path );

	helper.ReturnVal( Object( ret ) );
}

void do_os_isfile( Frame* f )
{
	BuiltinHelper helper( "os", "isfile", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	filesystem::path path( o->s );
	bool ret = is_regular_file( path );

	helper.ReturnVal( Object( ret ) );
}

void do_os_sep( Frame* f )
{
	BuiltinHelper helper( "os", "sep", f, true );
	helper.CheckNumberOfArguments( 0 );

	string retstr( 1, dirsep );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_extsep( Frame* f )
{
	BuiltinHelper helper( "os", "extsep", f, true );
	helper.CheckNumberOfArguments( 0 );

	string retstr( 1, extsep );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_curdir( Frame* f )
{
	BuiltinHelper helper( "os", "curdir", f, true );
	helper.CheckNumberOfArguments( 0 );

	string retstr( cwdstr );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_pardir( Frame* f )
{
	BuiltinHelper helper( "os", "pardir", f, true );
	helper.CheckNumberOfArguments( 0 );

	string retstr( pardirstr );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}

void do_os_pathsep( Frame* f )
{
	BuiltinHelper helper( "os", "pathsep", f, true );
	helper.CheckNumberOfArguments( 0 );

	string retstr( env_var_path_seps );

	const char* ret = f->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}


} // namespace deva

