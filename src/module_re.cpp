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

// module_re.h
// built-in module '_re' for the deva language 
// created by jcs, december 8, 2009 

// TODO:
// * 


#include "module_re.h"
#include <boost/regex.hpp>

using namespace boost;


void do_re_compile( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module '_re' function 'compile'." );

	// regex to make is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'regex' argument in module '_re' function 'compile'" );
	}
	if( !o )
		o = &obj;

	// ensure regex is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'regex' argument to module '_re' function 'compile' must be a string." );

	regex* r = new regex( o->str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the compiled regex object
	ex->stack.push_back( DevaObject( "", (void*)r ) );
}

void do_re_match( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module '_re' function 'match'." );

	// regex object is on top of the stack
	DevaObject regex = ex->stack.back();
	ex->stack.pop_back();

	// input string to match against is next
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* re = NULL;
	if( regex.Type() == sym_unknown )
	{
		re = ex->find_symbol( regex );
		if( !re )
			throw DevaRuntimeException( "Symbol not found for 'regex' argument in module '_re' function 'match'." );
	}
	if( !re )
		re = &regex;

	// ensure it's a native object
	if( re->Type() != sym_native_obj )
		throw DevaRuntimeException( "'regex' argument to module '_re' function 'match' is not of the correct type." );

	// check the input string arg
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module '_re' function 'match'" );
	}
	if( !o )
		o = &obj;

	// ensure input is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'input' argument to module '_re' function 'match' must be a string." );

	cmatch match;
	bool found = regex_match( o->str_val, match, *((boost::regex*)(re->nat_obj_val)) );

	DOVector* vec = new DOVector();
	if( found )
	{
		vec->reserve( match.size() );
		for( int i = 0; i < match.size(); ++i )
		{
			DOMap* mp = new DOMap();
			mp->insert( make_pair( DevaObject( "", string( "start" ) ), DevaObject( "", (double)match.position( i ) ) ) );
			mp->insert( make_pair( DevaObject( "", string( "end" ) ), DevaObject( "", (double)(match.position( i ) + match.length( i ) ) ) ) );
			mp->insert( make_pair( DevaObject( "", string( "str" ) ), DevaObject( "", match.str( i ) ) ) );
			vec->push_back( DevaObject( "", mp ) );
		}
	}
	else
		delete vec;

	// pop the return address
	ex->stack.pop_back();

	// return the vector if found
	if( found )
		ex->stack.push_back( DevaObject( "", vec ) );
	// null if not
	else
		ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_re_search( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module '_re' function 'search'." );

	// regex object is on top of the stack
	DevaObject regex = ex->stack.back();
	ex->stack.pop_back();

	// input string to match against is next
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* re = NULL;
	if( regex.Type() == sym_unknown )
	{
		re = ex->find_symbol( regex );
		if( !re )
			throw DevaRuntimeException( "Symbol not found for 'regex' argument in module '_re' function 'search'." );
	}
	if( !re )
		re = &regex;

	// ensure it's a native object
	if( re->Type() != sym_native_obj )
		throw DevaRuntimeException( "'regex' argument to module '_re' function 'search' is not of the correct type." );

	// check the input string arg
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module '_re' function 'search'" );
	}
	if( !o )
		o = &obj;

	// ensure input is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'input' argument to module '_re' function 'search' must be a string." );

	cmatch match;
	bool found = regex_search( o->str_val, match, *((boost::regex*)(re->nat_obj_val)) );

	DOVector* vec = new DOVector();
	if( found )
	{
		vec->reserve( match.size() );
		for( int i = 0; i < match.size(); ++i )
		{
			DOMap* mp = new DOMap();
			mp->insert( make_pair( DevaObject( "", string( "start" ) ), DevaObject( "", (double)match.position( i ) ) ) );
			mp->insert( make_pair( DevaObject( "", string( "end" ) ), DevaObject( "", (double)(match.position( i ) + match.length( i ) ) ) ) );
			mp->insert( make_pair( DevaObject( "", string( "str" ) ), DevaObject( "", match.str( i ) ) ) );
			vec->push_back( DevaObject( "", mp ) );
		}
	}
	else
		delete vec;

	// pop the return address
	ex->stack.pop_back();

	// return the vector if found
	if( found )
		ex->stack.push_back( DevaObject( "", vec ) );
	// null if not
	else
		ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_re_replace( Executor* ex )
{
	if( Executor::args_on_stack != 3 )
		throw DevaRuntimeException( "Incorrect number of arguments to module '_re' function 'replace'." );

	// regex object is on top of the stack
	DevaObject regex = ex->stack.back();
	ex->stack.pop_back();

	// input string to match against is next
	DevaObject input = ex->stack.back();
	ex->stack.pop_back();
	
	// format string for replace is next
	DevaObject format = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* re = NULL;
	if( regex.Type() == sym_unknown )
	{
		re = ex->find_symbol( regex );
		if( !re )
			throw DevaRuntimeException( "Symbol not found for 'regex' argument in module '_re' function 'replace'." );
	}
	if( !re )
		re = &regex;

	// ensure it's a native object
	if( re->Type() != sym_native_obj )
		throw DevaRuntimeException( "'regex' argument to module '_re' function 'replace' is not of the correct type." );

	// check the input string arg
	DevaObject* s = NULL;
	if( input.Type() == sym_unknown )
	{
		s = ex->find_symbol( input );
		if( !s )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module '_re' function 'replace'" );
	}
	if( !s )
		s = &input;

	// ensure input is a string
	if( s->Type() != sym_string )
		throw DevaRuntimeException( "'input' argument to module '_re' function 'replace' must be a string." );

	// check the format string arg
	DevaObject* fmt = NULL;
	if( format.Type() == sym_unknown )
	{
		fmt = ex->find_symbol( format );
		if( !fmt )
			throw DevaRuntimeException( "Symbol not found for 'format' argument in module '_re' function 'replace'" );
	}
	if( !fmt )
		fmt = &format;

	// ensure format is a string
	if( fmt->Type() != sym_string )
		throw DevaRuntimeException( "'format' argument to module '_re' function 'replace' must be a string." );

	string result = regex_replace( string( s->str_val ), *((boost::regex*)(re->nat_obj_val)), string( fmt->str_val ) );

	// pop the return address
	ex->stack.pop_back();

	// return the result
	ex->stack.push_back( DevaObject( "", result ) );
}

void do_re_delete( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module '_re' function 'delete'." );

	// regex to make is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'regex' argument in module '_re' function 'delete'" );
	}
	if( !o )
		o = &obj;

	// ensure regex is a string
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'regex' argument to module '_re' function 'delete' must be a regex." );

	delete (boost::regex*)o->nat_obj_val;

	// pop the return address
	ex->stack.pop_back();

	// return null
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void AddReModule( Executor* ex )
{
	map<string, builtin_fcn> fcns = map<string, builtin_fcn>();
	fcns.insert( make_pair( string( "compile@_re" ), do_re_compile ) );
	fcns.insert( make_pair( string( "match@_re" ), do_re_match ) );
	fcns.insert( make_pair( string( "search@_re" ), do_re_search ) );
	fcns.insert( make_pair( string( "replace@_re" ), do_re_replace ) );
	fcns.insert( make_pair( string( "delete@_re" ), do_re_delete ) );
	ex->ImportBuiltinModuleFunctions( string( "_re" ), fcns );
}
