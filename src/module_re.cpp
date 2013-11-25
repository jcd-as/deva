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

// module_re.cpp
// native re operations module for deva language, v2
// created by jcs, april 3, 2011

// TODO:
// * 

#include "module_re.h"
#include "builtins_helpers.h"
#include "module.h"
#include "util.h"
#include <boost/regex.hpp>

using namespace boost;

namespace deva
{


const string module_re_names[] =
{
	string( "compile" ),
	string( "match" ),
	string( "search" ),
	string( "replace" ),
	string( "delete" )
};
	
NativeFunction module_re_fcns[] = 
{
	{do_re_compile, false},
	{do_re_match, false},
	{do_re_search, false},
	{do_re_replace, false},
	{do_re_delete, false}
};

const int num_of_module_re_fcns = sizeof( module_re_names ) / sizeof( module_re_names[0] );


// is this name a module re function?
bool IsModuleReFunction( const string & name )
{
	const string* i = find( module_re_names, module_re_names + num_of_module_re_fcns, name );
	if( i != module_re_names + num_of_module_re_fcns ) return true;
		else return false;
}

NativeModule* GetModuleRe()
{
	return new NativeModule( "_re", module_re_fcns, module_re_names, num_of_module_re_fcns );
}


/////////////////////////////////////////////////////////////////////////////
// module re functions
/////////////////////////////////////////////////////////////////////////////
void do_re_compile( Frame* f )
{
	BuiltinHelper helper( "_re", "compile", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_string );

	regex* r;
	try
	{
		r = new regex( o->s );
	}
	catch( boost::regex_error & e )
	{
		throw RuntimeException( boost::format( "Invalid regular expression: %1%" ) % e.what() );
	}

	helper.ReturnVal( Object( (void*)r ) );
}

void do_re_match( Frame* f )
{
	BuiltinHelper helper( "_re", "", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* r = helper.GetLocalN( 0 );
	helper.ExpectType( r, obj_native_obj );

	Object* s = helper.GetLocalN( 1 );
	helper.ExpectType( s, obj_string );


	cmatch match;
	bool found = regex_match( s->s, match, *((boost::regex*)(r->no)) );

	Vector* vec;
	if( found )
	{
		vec = CreateVector();
		vec->reserve( match.size() );
		for( dword i = 0; i < match.size(); ++i )
		{
			Map* mp = CreateMap();
			mp->IncRef();
			const char* start_s = f->GetParent()->AddString( string( "start" ) );
			mp->insert( make_pair( Object( start_s ), Object( (double)match.position( i ) ) ) );
			const char* end_s = f->GetParent()->AddString( string( "end" ) );
			mp->insert( make_pair( Object( end_s ), Object( (double)(match.position( i ) + match.length( i ) ) ) ) );
			const char* str_s = f->GetParent()->AddString( string( "str" ) );
			const char* match_s = f->GetParent()->AddString( match.str( i ) );
			mp->insert( make_pair( Object( str_s ), Object( match_s ) ) );
			vec->push_back( Object( mp ) );
		}
		helper.ReturnVal( Object( vec ) );
	}
	else
		helper.ReturnVal( Object( obj_null ) );
}

void do_re_search( Frame* f )
{
	BuiltinHelper helper( "_re", "", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* r = helper.GetLocalN( 0 );
	helper.ExpectType( r, obj_native_obj );

	Object* s = helper.GetLocalN( 1 );
	helper.ExpectType( s, obj_string );

	cmatch match;
	bool found = regex_search( s->s, match, *((boost::regex*)(r->no)) );

	Vector* vec;
	if( found )
	{
		vec = CreateVector();
		vec->reserve( match.size() );
		for( dword i = 0; i < match.size(); ++i )
		{
			Map* mp = CreateMap();
			mp->IncRef();
			const char* start_s = f->GetParent()->AddString( string( "start" ) );
			mp->insert( make_pair( Object( start_s ), Object( (double)match.position( i ) ) ) );
			const char* end_s = f->GetParent()->AddString( string( "end" ) );
			mp->insert( make_pair( Object( end_s ), Object( (double)(match.position( i ) + match.length( i ) ) ) ) );
			const char* str_s = f->GetParent()->AddString( string( "str" ) );
			const char* match_s = f->GetParent()->AddString( match.str( i ) );
			mp->insert( make_pair( Object( str_s ), Object( match_s ) ) );
			vec->push_back( Object( mp ) );
		}
		helper.ReturnVal( Object( vec ) );
	}
	else
		helper.ReturnVal( Object( obj_null ) );
}

void do_re_replace( Frame* f )
{
	BuiltinHelper helper( "_re", "", f, true );
	helper.CheckNumberOfArguments( 3 );

	Object* r = helper.GetLocalN( 0 );
	helper.ExpectType( r, obj_native_obj );

	Object* s = helper.GetLocalN( 1 );
	helper.ExpectType( s, obj_string );

	Object* fmt = helper.GetLocalN( 2 );
	helper.ExpectType( fmt, obj_string );

	string result = regex_replace( string( s->s ), *((boost::regex*)(r->no)), string( fmt->s ) );

	const char* ret = f->GetParent()->AddString( result );

	helper.ReturnVal( Object( ret ) );
}

void do_re_delete( Frame* f )
{
	BuiltinHelper helper( "_re", "", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* r = helper.GetLocalN( 0 );
	helper.ExpectType( r, obj_native_obj );

	delete (boost::regex*)r->no;

	helper.ReturnVal( Object( obj_null ) );
}


} // namespace deva

