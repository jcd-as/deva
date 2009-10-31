// string_builtins.cpp
// built-in methods on strings, for the deva language virtual machine
// created by jcs, october 30, 2009 

// TODO:
// * 
//

#include "string_builtins.h"
//#include <iostream>
//#include <sstream>
#include <algorithm>

// to add new builtins you must:
// 1) add a new fcn to the string_builtin_names and string_builtin_fcns arrays below
// 2) implement the function in this file
// 3) add the function as a friend to Executor (executor.h) so that it can
//    access the private members of Executor (namely the stack)

// pre-decls for builtin executors
void do_string_append( Executor *ex );
void do_string_length( Executor *ex );
void do_string_copy( Executor *ex );
void do_string_insert( Executor *ex );
void do_string_remove( Executor *ex );
void do_string_find( Executor *ex );
void do_string_rfind( Executor *ex );
void do_string_reverse( Executor *ex );
void do_string_sort( Executor *ex );
void do_string_slice( Executor *ex );

// tables defining the built-in function names...
static const string string_builtin_names[] = 
{
    string( "string_append" ),
    string( "string_length" ),
    string( "string_copy" ),
    string( "string_insert" ),
    string( "string_remove" ),
    string( "string_find" ),
    string( "string_rfind" ),
    string( "string_reverse" ),
    string( "string_sort" ),
    string( "string_slice" ),
};
// ...and function pointers to the executor functions for them
typedef void (*string_builtin_fcn)(Executor*);
string_builtin_fcn string_builtin_fcns[] = 
{
    do_string_append,
    do_string_length,
    do_string_copy,
    do_string_insert,
    do_string_remove,
    do_string_find,
    do_string_rfind,
    do_string_reverse,
    do_string_sort,
    do_string_slice,
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
void do_string_append( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'append' built-in method." );

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
		throw DevaRuntimeException( "String expected in string built-in method 'append'." );

	// get a pointer to the symbol
	DevaObject* s = NULL;
	s = ex->find_symbol( str );
	if( !s )
		throw DevaRuntimeException( "Symbol not found in call to string built-in method 'append'" );

	if( s->Type() != sym_string )
		throw DevaICE( "String expected in string built-in method 'append'." );

	// concatenate the strings
	string ret( s->str_val );
	ret += o->str_val;
	*s = DevaObject( s->name, ret );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
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
		throw DevaICE( "Vector expected in string built-in method 'insert'." );

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
	*s = DevaObject( s->name, ret );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
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
	*s = DevaObject( s->name, ret );


	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
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

	reverse( s->str_val + i_start, s->str_val + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", sym_null ) );
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

	sort( s->str_val + i_start, s->str_val + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", sym_null ) );
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
		// TODO: length need to be integral values. error if they aren't
		i_end = (size_t)end.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// 'step' value to slice with
		DevaObject step = ex->stack.back();
		ex->stack.pop_back();
		if( step.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'length' argument in string built-in method 'slice'." );
		// TODO: length need to be integral values. error if they aren't
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
