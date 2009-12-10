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

// string_builtins.cpp
// built-in methods on strings, for the deva language virtual machine
// created by jcs, october 30, 2009 

// TODO:
// * 
//

#include "string_builtins.h"
#include <algorithm>
#include <locale>

// to add new builtins you must:
// 1) add a new fcn to the string_builtin_names and string_builtin_fcns arrays below
// 2) implement the function in this file

// pre-decls for builtin executors
void do_string_concat( Executor *ex );
void do_string_length( Executor *ex );
void do_string_copy( Executor *ex );
void do_string_insert( Executor *ex );
void do_string_remove( Executor *ex );
void do_string_find( Executor *ex );
void do_string_rfind( Executor *ex );
void do_string_reverse( Executor *ex );
void do_string_sort( Executor *ex );
void do_string_slice( Executor *ex );
void do_string_strip( Executor *ex );
void do_string_lstrip( Executor *ex );
void do_string_rstrip( Executor *ex );
void do_string_split( Executor *ex );
void do_string_replace( Executor *ex );
void do_string_upper( Executor *ex );
void do_string_lower( Executor *ex );
void do_string_isalphanum( Executor *ex );
void do_string_isalpha( Executor *ex );
void do_string_isdigit( Executor *ex );
void do_string_islower( Executor *ex );
void do_string_isupper( Executor *ex );
void do_string_isspace( Executor *ex );
void do_string_ispunct( Executor *ex );
void do_string_iscntrl( Executor *ex );
void do_string_isprint( Executor *ex );
void do_string_isxdigit( Executor *ex );

// tables defining the built-in function names...
static const string string_builtin_names[] = 
{
    string( "string_concat" ),
    string( "string_length" ),
    string( "string_copy" ),
    string( "string_insert" ),
    string( "string_remove" ),
    string( "string_find" ),
    string( "string_rfind" ),
    string( "string_reverse" ),
    string( "string_sort" ),
    string( "string_slice" ),
    string( "string_strip" ),
    string( "string_lstrip" ),
    string( "string_rstrip" ),
    string( "string_split" ),
    string( "string_replace" ),
    string( "string_upper" ),
    string( "string_lower" ),
	string( "string_isalphanum" ),
	string( "string_isalpha" ),
	string( "string_isdigit" ),
	string( "string_islower" ),
	string( "string_isupper" ),
	string( "string_isspace" ),
	string( "string_ispunct" ),
	string( "string_iscntrl" ),
	string( "string_isprint" ),
	string( "string_isxdigit" ),
};
// ...and function pointers to the executor functions for them
typedef void (*string_builtin_fcn)(Executor*);
string_builtin_fcn string_builtin_fcns[] = 
{
    do_string_concat,
    do_string_length,
    do_string_copy,
    do_string_insert,
    do_string_remove,
    do_string_find,
    do_string_rfind,
    do_string_reverse,
    do_string_sort,
    do_string_slice,
    do_string_strip,
    do_string_lstrip,
    do_string_rstrip,
    do_string_split,
    do_string_replace,
    do_string_upper,
    do_string_lower,
	do_string_isalphanum,
	do_string_isalpha,
	do_string_isdigit,
	do_string_islower,
	do_string_isupper,
	do_string_isspace,
	do_string_ispunct,
	do_string_iscntrl,
	do_string_isprint,
	do_string_isxdigit,
};
const int num_of_string_builtins = sizeof( string_builtin_names ) / sizeof( string_builtin_names[0] );

// is this name a built-in function?
bool is_string_builtin( const string & name )
{
    const string* i = find( string_builtin_names, string_builtin_names + num_of_string_builtins, name );
    if( i != string_builtin_names + num_of_string_builtins ) return true;
	else return false;
}

// execute built-in function
void execute_string_builtin( Executor *ex, const string & name )
{
    const string* i = find( string_builtin_names, string_builtin_names + num_of_string_builtins, name );
    if( i == string_builtin_names + num_of_string_builtins )
		throw DevaICE( "No such string built-in method." );
    // compute the index of the function in the look-up table(s)
    long l = (long)i;
    l -= (long)&string_builtin_names;
    int idx = l / sizeof( string );
    if( idx > num_of_string_builtins )
		throw DevaICE( "Out-of-array-bounds looking for string built-in method." );
    else
        // call the function
        string_builtin_fcns[idx]( ex );
}

// the built-in executor functions:
void do_string_concat( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'concat' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &val;

	// value
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "String expected in string built-in method 'concat'." );

	// get a pointer to the symbol
	DevaObject* s = NULL;
	if( str.Type() == sym_unknown )
	{
		s = ex->find_symbol( str );
		if( !s )
			throw DevaRuntimeException( "Symbol not found in call to string built-in method 'concat'" );
	}
	if( !s )
		s = &str;

	if( s->Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'concat'." );

	// concatenate the strings
	string ret( s->str_val );
	ret += o->str_val;

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_length( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'length' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	int len;
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'length'." );

	len = strlen( str.str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", (double)len ) );
}

void do_string_copy( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'copy'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'copy'." );

	// create a new string object that is a copy of the one we received
    DevaObject copy( "", string( str.str_val ) );

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( copy );
}

void do_string_insert( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'insert' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// position to insert at is next on stack
	DevaObject pos = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'insert'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'insert'." );
	}
	else
		o = &val;

	// position
	if( pos.Type() != sym_number )
		throw DevaRuntimeException( "Number expected in for position argument in string built-in method 'insert'." );

	// TODO: position has to be integer. throw error on non-integral number?
	int i = (int)pos.num_val;
	if( i > strlen( str.str_val ) )
		throw DevaRuntimeException( "Position argument greater than string length in string built-in method 'insert'." );

	// insert the value
	// get a pointer to the symbol
	DevaObject* s = NULL;
	s = ex->find_symbol( str );
	if( !s )
		throw DevaRuntimeException( "Symbol not found in call to string built-in method 'insert'" );

	if( s->Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'insert'." );

	// do the insert
	string ret( s->str_val );
	ret.insert( i, o->str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_remove( Executor *ex )
{
	if( Executor::args_on_stack != 2 && Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'remove' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// start position to insert at is next on stack
	DevaObject start = ex->stack.back();
	ex->stack.pop_back();

	// default value for end is '-1'
	int i_end = -1;
	bool default_arg = true;
	if( Executor::args_on_stack == 2 )
	{
		default_arg = false;
		// end position to remove at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();

		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in string built-in method 'remove'." );
		// TODO: end needs to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// start position
	if( start.Type() != sym_number )
		throw DevaRuntimeException( "Number expected in for start position argument in string built-in method 'remove'." );

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "Vector expected in string built-in method 'remove'." );

	// TODO: start needs to be integral values. error if they aren't
	size_t sz = strlen( str.str_val );
	int i_start = (int)start.num_val;
	if( default_arg )
		i_end = i_start;
	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'remove'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in string built-in method 'remove'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in string built-in method 'remove': start is greater than end." );

	// get a pointer to the symbol
	DevaObject* s = NULL;
	s = ex->find_symbol( str );
	if( !s )
		throw DevaRuntimeException( "Symbol not found in call to string built-in method 'remove'" );

	if( s->Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'remove'." );

	// do the removal
	string ret( s->str_val );
	ret.erase( i_start, i_end );


	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_find( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'find' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// value is first on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	size_t i_start = 0;
	size_t i_len = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in string built-in method 'find'." );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// length of substring to find at is next on stack
		DevaObject len = ex->stack.back();
		ex->stack.pop_back();
		// length of substr
		if( len.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'length' argument in string built-in method 'find'." );
		// TODO: length need to be integral values. error if they aren't
		i_len = (int)len.num_val;
	}

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'find'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'find'." );
	}
	else
		o = &val;

	size_t sz = strlen( str.str_val );
	size_t sz_val = strlen( o->str_val );

	// default length is the entire search string
	if( i_len == -1 )
		i_len = sz_val;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'find'." );
	if( i_len > sz_val || i_len < 0 )
		throw DevaRuntimeException( "Invalid 'length' argument in string built-in method 'find'." );

	// find the element that matches
	DevaObject ret;
	string s( str.str_val );
	size_t fpos = s.find( o->str_val, i_start, i_len );
	if( fpos != string::npos )
		ret = DevaObject( "", (double)fpos );
	else
		ret = DevaObject( "", sym_null );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_string_rfind( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'rfind' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// value is first on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	size_t i_start = string::npos;
	size_t i_len = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in string built-in method 'rfind'." );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// length of substring to find at is next on stack
		DevaObject len = ex->stack.back();
		ex->stack.pop_back();
		// length of substr
		if( len.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'length' argument in string built-in method 'rfind'." );
		// TODO: length need to be integral values. error if they aren't
		i_len = (int)len.num_val;
	}

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'rfind'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'rfind'." );
	}
	else
		o = &val;

	size_t sz = strlen( str.str_val );
	size_t sz_val = strlen( o->str_val );

	// convert '-1' into end-of-string
	if( i_start == -1 )
		i_start = string::npos;

	// default length is the entire search string
	if( i_len == -1 )
		i_len = sz_val;

	if( (i_start >= sz && i_start != string::npos) || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'rfind'." );
	if( (i_len > sz_val && i_len != string::npos) || i_len < 0 )
		throw DevaRuntimeException( "Invalid 'length' argument in string built-in method 'rfind'." );

	// rfind the element that matches
	DevaObject ret;
	string s( str.str_val );
	size_t fpos = s.rfind( o->str_val, i_start, i_len );
	if( fpos != string::npos )
		ret = DevaObject( "", (double)fpos );
	else
		ret = DevaObject( "", sym_null );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_string_reverse( Executor *ex )
{
	if( Executor::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'reverse' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in string built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 0 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in string built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "Vector expected in string built-in method 'reverse'." );

	size_t sz = strlen( str.str_val );

	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'reverse'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in string built-in method 'reverse'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in string built-in method 'reverse': start is greater than end." );

	// get a pointer to the symbol
	DevaObject* s = NULL;
	s = ex->find_symbol( str );
	if( !s )
		throw DevaRuntimeException( "Symbol not found in call to string built-in method 'remove'" );

	if( s->Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'remove'." );

	string ret( s->str_val );
	reverse( ret.begin() + i_start, ret.begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_sort( Executor *ex )
{
	if( Executor::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'sort' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in string built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 0 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in string built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'sort'." );

	size_t sz = strlen( str.str_val );

	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'sort'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in string built-in method 'sort'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in string built-in method 'sort': start is greater than end." );

	// get a pointer to the symbol
	DevaObject* s = NULL;
	s = ex->find_symbol( str );
	if( !s )
		throw DevaRuntimeException( "Symbol not found in call to string built-in method 'remove'" );

	if( s->Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'remove'." );

	string ret( s->str_val );
	sort( ret.begin() + i_start, ret.begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_slice( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'slice' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	size_t i_start = 0;
	size_t i_end = -1;
	size_t i_step = 1;
	if( Executor::args_on_stack > 0 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in string built-in method 'slice'." );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (size_t)start.num_val;
	}
	if( Executor::args_on_stack > 1 )
	{
		// end of substring to slice
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'length' argument in string built-in method 'slice'." );
		// TODO: length needs to be integral values. error if they aren't
		i_end = (size_t)end.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// 'step' value to slice with
		DevaObject step = ex->stack.back();
		ex->stack.pop_back();
		if( step.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'step' argument in string built-in method 'slice'." );
		// TODO: step needs to be integral values. error if they aren't
		i_step = (size_t)step.num_val;
	}

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'slice'." );

	size_t sz = strlen( str.str_val );

	// default length is the entire search string
	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'slice'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in string built-in method 'slice'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in string built-in method 'slice': start is greater than end." );

	// slice the string
	DevaObject ret;
	string s( str.str_val );
	// 'step' is '1' (the default)
	if( i_step == 1 )
	{
		string r = s.substr( i_start, i_end - i_start );
		ret = DevaObject( "", r );
	}
	// otherwise the string class doesn't help us, have to do it manually
	else
	{
		// first get the substring from start to end positions
		string r = s.substr( i_start, i_end - i_start );
		// TODO: call 'reserve' on the string to reduce allocations?
		// then walk it grabbing every 'nth' character
		string slice;
		for( int i = 0; i < r.length(); i += i_step )
		{
			slice += r[i];
		}
		ret = DevaObject( "", slice );
	}

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_string_strip( Executor *ex )
{
	if( Executor::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'strip' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'strip'." );

	// value to strip (string containing characters to strip)
	DevaObject* o;
	string chars;
	if( Executor::args_on_stack == 1 )
	{
		// value to strip is first on stack
		DevaObject val = ex->stack.back();
		ex->stack.pop_back();

		// value
		if( val.Type() == sym_unknown )
		{
			o = ex->find_symbol( val );
			if( !o )
				throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'strip'." );
		}
		else
			o = &val;
		if( o->Type() != sym_string )
			throw DevaRuntimeException( "'characters' argument to string built-in method 'strip' must be a string." );
		chars = o->str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	
	// left-side strip
	size_t left = in.find_first_not_of( chars );
	if( left == string::npos )
		left = 0;
	// right-side strip
	size_t right = in.find_last_not_of( chars );
	if( right == string::npos )
		right = in.length();
	string ret = in.substr( left, right - left + 1 );

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_lstrip( Executor *ex )
{
	if( Executor::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'lstrip' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'lstrip'." );

	// value to strip (string containing characters to strip)
	DevaObject* o;
	string chars;
	if( Executor::args_on_stack == 1 )
	{
		// value to strip is first on stack
		DevaObject val = ex->stack.back();
		ex->stack.pop_back();

		// value
		if( val.Type() == sym_unknown )
		{
			o = ex->find_symbol( val );
			if( !o )
				throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'lstrip'." );
		}
		else
			o = &val;
		if( o->Type() != sym_string )
			throw DevaRuntimeException( "'characters' argument to string built-in method 'lstrip' must be a string." );
		chars = o->str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	
	// left-side strip
	size_t left = in.find_first_not_of( chars );
	if( left == string::npos )
		left = 0;
	// right-side
	size_t right = in.length();
	if( right == string::npos )
		right = in.length();
	string ret = in.substr( left, right - left + 1 );

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_rstrip( Executor *ex )
{
	if( Executor::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'rstrip' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'rstrip'." );

	// value to strip (string containing characters to strip)
	DevaObject* o;
	string chars;
	if( Executor::args_on_stack == 1 )
	{
		// value to strip is first on stack
		DevaObject val = ex->stack.back();
		ex->stack.pop_back();

		// value
		if( val.Type() == sym_unknown )
		{
			o = ex->find_symbol( val );
			if( !o )
				throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'rstrip'." );
		}
		else
			o = &val;
		if( o->Type() != sym_string )
			throw DevaRuntimeException( "'characters' argument to string built-in method 'rstrip' must be a string." );
		chars = o->str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	
	// right-side strip
	size_t right = in.find_last_not_of( chars );
	string ret = in.substr( 0, right + 1 );

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_split( Executor *ex )
{
	if( Executor::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'split' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'split'." );

	// value to split at (string containing characters to split on)
	DevaObject* o;
	string chars;
	if( Executor::args_on_stack == 1 )
	{
		// value to split is first on stack
		DevaObject val = ex->stack.back();
		ex->stack.pop_back();

		// value
		if( val.Type() == sym_unknown )
		{
			o = ex->find_symbol( val );
			if( !o )
				throw DevaRuntimeException( "Symbol not found for the value argument in string built-in method 'split'." );
		}
		else
			o = &val;
		if( o->Type() != sym_string )
			throw DevaRuntimeException( "'characters' argument to string built-in method 'split' must be a string." );
		chars = o->str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	
	DOVector* ret = new DOVector();

	// special case empty string, which means split into a vector of individual
	// characters, NOT a vector containing only the original string
	if( chars.length() == 0 )
	{
		ret->reserve( in.length() );
		for( int c = 0; c < in.length(); ++c )
			ret->push_back( DevaObject( "", string( 1, in[c] ) ) );
	}
	else
	{
		size_t left = in.find_first_not_of( chars );
		if( left != string::npos )
		{	
			size_t right = in.find_first_of( chars );
			size_t len = in.length();
			while( left != string::npos )
			{
				string s( in, left, right - left );
				ret->push_back( DevaObject( "", s ) );

				left = in.find_first_not_of( chars, right );
				right = in.find_first_of( chars, right + 1 );
				// if 'left' is greater than 'right', then we passed an empty string 
				// (two matching split chars in a row), enter it and move forward
				if( left > right )
				{
					ret->push_back( DevaObject( "", string( "" ) ) );
					right = in.find_first_of( chars, right + 1 );
				}
			}
		}
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_replace( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'replace' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

	// search string is next
	DevaObject srch = ex->stack.back();
	ex->stack.pop_back();

	// replacement value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// string
	if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'replace'." );

	// search string
	DevaObject* s;
	if( srch.Type() == sym_unknown )
	{
		s = ex->find_symbol( srch );
		if( !s )
			throw DevaRuntimeException( "Symbol not found for the 'search_string' argument in string built-in method 'replace'." );
	}
	else
		s = &srch;
	
	// replacement value
	DevaObject* rep;
	if( val.Type() == sym_unknown )
	{
		rep = ex->find_symbol( val );
		if( !rep )
			throw DevaRuntimeException( "Symbol not found for the 'replacement_value' argument in string built-in method 'replace'." );
	}
	else
		rep = &val;

	// do the replacement
	string ret( str.str_val );
	string search( s->str_val );
	string rep_val( rep->str_val );
	size_t pos = ret.find( search );
	while( pos != string::npos )
	{
		// do the replacement
		ret.replace( pos, search.length(), rep_val );
		// find the search string
		pos = ret.find( search, pos + 1 );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_upper( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'upper'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'upper'." );


	string ret;
	ret.reserve( strlen( str.str_val ) );
	locale loc;
	int i = 0;
	while( str.str_val[i] )
	{
		ret.push_back( toupper( str.str_val[i], loc ) );
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_lower( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'lower'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'lower'." );


	string ret;
	ret.reserve( strlen( str.str_val ) );
	locale loc;
	int i = 0;
	while( str.str_val[i] )
	{
		ret.push_back( tolower( str.str_val[i], loc ) );
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}


void do_string_isalphanum( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isalphanum'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isalphanum'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isalnum( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_isalpha( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isalpha'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isalpha'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isalpha( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_isdigit( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isdigit'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isdigit'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isdigit( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_islower( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'islower'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'islower'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !islower( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_isupper( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isupper'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isupper'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isupper( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_isspace( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isspace'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isspace'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isspace( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_ispunct( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'ispunct'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'ispunct'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !ispunct( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_iscntrl( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'iscntrl'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'iscntrl'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !iscntrl( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_isprint( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isprint'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isprint'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isprint( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_isxdigit( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isxdigit'." );

	// get the string object off the top of the stack
	DevaObject str = ex->stack.back();
	ex->stack.pop_back();

    if( str.Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'isxdigit'." );


	locale loc;
	int i = 0;
	bool ret = true;
	while( str.str_val[i] )
	{
		if( !isxdigit( str.str_val[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( DevaObject( "", ret ) );
}
