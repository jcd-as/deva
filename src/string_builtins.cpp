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

// string_builtins.cpp
// builtin string methods for the deva language
// created by jcs, march 6, 2011

// TODO:
// * 

#include "string_builtins.h"
#include "builtins_helpers.h"
#include "util.h"
#include <algorithm>
#include <locale>
#include <sstream>

using namespace std;


namespace deva
{


bool IsStringBuiltin( const string & name )
{
	const string* i = find( string_builtin_names, string_builtin_names + num_of_string_builtins, name );
	if( i != string_builtin_names + num_of_string_builtins ) return true;
	else return false;
}

NativeFunction GetStringBuiltin( const string & name )
{
	const string* i = find( string_builtin_names, string_builtin_names + num_of_string_builtins, name );
	if( i == string_builtin_names + num_of_string_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&string_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_string_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	else
	{
		// return the function
		NativeFunction nf;
		nf.p = string_builtin_fcns[idx];
		nf.is_method = true;
		return nf;
	}
}

Object* GetStringBuiltinObjectRef( const string & name )
{
	const string* i = find( string_builtin_names, string_builtin_names + num_of_string_builtins, name );
	if( i == string_builtin_names + num_of_string_builtins )
	{
		return NULL;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&string_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_string_builtins )
	{
		return NULL;
	}
	else
	{
		// return the function object
		return &string_builtin_fcn_objs[idx];
	}
}


/////////////////////////////////////////////////////////////////////////////
// string builtins
/////////////////////////////////////////////////////////////////////////////

void do_string_concat( Frame *frame )
{
	BuiltinHelper helper( "string", "concat", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );
	Object* po = helper.GetLocalN( 1 );
	helper.ExpectType( po , obj_string );

	// concatenate the strings
	// (strings are immutable. create a copy and add it to the calling frame)
	char* s = catstr( self->s, po->s );
	frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( s ) );
}

void do_string_length( Frame *frame )
{
	BuiltinHelper helper( "string", "length", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	int len = strlen( self->s );
	
	helper.ReturnVal( Object( (double)len ) );
}

void do_string_copy( Frame *frame )
{
	BuiltinHelper helper( "string", "copy", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// copy the string
	// (strings are immutable. create a copy and add it to the calling frame)
	char* s = copystr( self->s );
	frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( s ) );
}

void do_string_insert( Frame *frame )
{
	BuiltinHelper helper( "string", "insert", frame );

	helper.CheckNumberOfArguments( 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );
	Object* pos = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( pos );
	Object* po = helper.GetLocalN( 2 );
	helper.ExpectType( po, obj_string );

	// insert the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	s.insert( (size_t)pos->d, po->s );
	const char* ret = frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( ret ) );
}

void do_string_remove( Frame *frame )
{
	BuiltinHelper helper( "string", "remove", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );
	Object* startobj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( startobj );

	int start = (int)startobj->d;
	// end arg defaults to -1 (same as end-of-string)
	int end = -1;
	if( frame->NumArgsPassed() == 3 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	// remove the chars
	// (strings are immutable. create a copy and add it to the calling frame)
	size_t sz = strlen( self->s );
	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in string built-in method 'remove'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in string built-in method 'remove'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in string built-in method 'remove': start is greater than end." );

	string s( self->s );
	s.erase( start, end );
	const char* ret = frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( ret ) );
}
/*
void do_string_find( Frame *frame )
{
	if( Frame::args_on_stack > 3 || Frame::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'find' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "find", sym_string );

	// value is first on stack
	DevaObject val = get_arg_of_type( ex, "string.find", "value", sym_string );
	
	size_t i_start = 0;
	size_t i_len = -1;
	if( Frame::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = get_arg_of_type( ex, "string.find", "start", sym_number );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Frame::args_on_stack > 2 )
	{
		// length of substring to find at is next on stack
		DevaObject len = get_arg_of_type( ex, "string.find", "length", sym_number );
		// TODO: length need to be integral values. error if they aren't
		i_len = (int)len.num_val;
	}

	size_t sz = strlen( str.str_val );
	size_t sz_val = strlen( val.str_val );

	DevaObject ret;
	// can't find anything in an empty string
	if( sz == 0 )
	{
		ret = DevaObject( "", sym_null );
	}
	else
	{
		// default length is the entire search string
		if( i_len == -1 )
			i_len = sz_val;

		if( i_start >= sz || i_start < 0 )
			throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'find'." );
		if( i_len > sz_val || i_len < 0 )
			throw DevaRuntimeException( "Invalid 'length' argument in string built-in method 'find'." );

		// find the element that matches
		string s( str.str_val );
		size_t fpos = s.find( val.str_val, i_start, i_len );
		if( fpos != string::npos )
			ret = DevaObject( "", (double)fpos );
		else
			ret = DevaObject( "", sym_null );
	}

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_string_rfind( Frame *frame )
{
	if( Frame::args_on_stack > 3 || Frame::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'rfind' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "rfind", sym_string );

	// value is first on stack
	DevaObject val = get_arg_of_type( ex, "string.rfind", "value", sym_string );
	
	size_t i_start = string::npos;
	size_t i_len = -1;
	if( Frame::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = get_arg_of_type( ex, "string.rfind", "start", sym_number );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Frame::args_on_stack > 2 )
	{
		// length of substring to find at is next on stack
		DevaObject len = get_arg_of_type( ex, "string.rfind", "length", sym_number );
		// TODO: length need to be integral values. error if they aren't
		i_len = (int)len.num_val;
	}

	size_t sz = strlen( str.str_val );
	size_t sz_val = strlen( val.str_val );

	DevaObject ret;
	// can't find anything in an empty string
	if( sz == 0 )
	{
		ret = DevaObject( "", sym_null );
	}
	else
	{
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
		string s( str.str_val );
		size_t fpos = s.rfind( val.str_val, i_start, i_len );
		if( fpos != string::npos )
			ret = DevaObject( "", (double)fpos );
		else
			ret = DevaObject( "", sym_null );
	}

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_string_reverse( Frame *frame )
{
	if( Frame::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'reverse' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "reverse", sym_string );

	int i_start = 0;
	int i_end = -1;
	if( Frame::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = get_arg_of_type( ex, "string.reverse", "start", sym_number );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Frame::args_on_stack > 0 )
	{
		// end position to insert at is next on stack
		DevaObject end = get_arg_of_type( ex, "string.reverse", "end", sym_number );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	size_t sz = strlen( str.str_val );

	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'reverse'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in string built-in method 'reverse'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in string built-in method 'reverse': start is greater than end." );

	string ret( str.str_val );
	reverse( ret.begin() + i_start, ret.begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_sort( Frame *frame )
{
	if( Frame::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'sort' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "sort", sym_string );

	int i_start = 0;
	int i_end = -1;
	if( Frame::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = get_arg_of_type( ex, "string.sort", "start", sym_number );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Frame::args_on_stack > 0 )
	{
		// end position to insert at is next on stack
		DevaObject end = get_arg_of_type( ex, "string.sort", "end", sym_number );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	size_t sz = strlen( str.str_val );

	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in string built-in method 'sort'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in string built-in method 'sort'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in string built-in method 'sort': start is greater than end." );

	string ret( str.str_val );
	sort( ret.begin() + i_start, ret.begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_slice( Frame *frame )
{
	if( Frame::args_on_stack > 3 || Frame::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'slice' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "slice", sym_string );

	size_t i_start = 0;
	size_t i_end = -1;
	size_t i_step = 1;
	if( Frame::args_on_stack > 0 )
	{
		// start position to insert at is next on stack
		DevaObject start = get_arg_of_type( ex, "string.slice", "start", sym_number );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (size_t)start.num_val;
	}
	if( Frame::args_on_stack > 1 )
	{
		// end of substring to slice
		DevaObject end = get_arg_of_type( ex, "string.slice", "end", sym_number );
		// TODO: length needs to be integral values. error if they aren't
		i_end = (size_t)end.num_val;
	}
	if( Frame::args_on_stack > 2 )
	{
		// 'step' value to slice with
		DevaObject step = get_arg_of_type( ex, "string.slice", "step", sym_number );
		// TODO: step needs to be integral values. error if they aren't
		i_step = (size_t)step.num_val;
	}

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

void do_string_strip( Frame *frame )
{
	if( Frame::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'strip' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "strip", sym_string );

	// value to strip (string containing characters to strip)
	string chars;
	if( Frame::args_on_stack == 1 )
	{
		// value to strip is first on stack
		DevaObject val = get_arg_of_type( ex, "string.strip", "value", sym_string );

		chars = val.str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	string ret( "" );
	
	// left-side strip
	size_t left = in.find_first_not_of( chars );
	// if there aren't any characters that aren't in our stripped list, return
	// an empty string
	if( left != string::npos )
	{
		// right-side strip
		size_t right = in.find_last_not_of( chars );
		ret = in.substr( left, right - left + 1 );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_lstrip( Frame *frame )
{
	if( Frame::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'lstrip' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "lstrip", sym_string );

	// value to strip (string containing characters to strip)
	string chars;
	if( Frame::args_on_stack == 1 )
	{
		// value to strip is first on stack
		DevaObject val = get_arg_of_type( ex, "string.lstrip", "value", sym_string );

		chars = val.str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	string ret( "" );
	
	// left-side strip
	size_t left = in.find_first_not_of( chars );
	// if there aren't any characters that aren't in our stripped list, return
	// an empty string
	if( left != string::npos )
	{
		// right-side
		size_t right = in.length();
		if( right == string::npos )
			right = in.length();
		ret = in.substr( left, right - left + 1 );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_rstrip( Frame *frame )
{
	if( Frame::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'rstrip' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "rstrip", sym_string );

	// value to strip (string containing characters to strip)
	string chars;
	if( Frame::args_on_stack == 1 )
	{
		// value to strip is first on stack
		DevaObject val = get_arg_of_type( ex, "string.rstrip", "value", sym_string );
		chars = val.str_val;
	}
	else
		chars = string( " \t\n" );

	string in( str.str_val );
	string ret( "" );
	
	// right-side strip
	size_t right = in.find_last_not_of( chars );
	// if there aren't any characters that aren't in our stripped list, return
	// an empty string
	if( right != string::npos )
	{
		ret = in.substr( 0, right + 1 );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_split( Frame *frame )
{
	if( Frame::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'split' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "split", sym_string );

	// value to split at (string containing characters to split on)
	string chars;
	if( Frame::args_on_stack == 1 )
	{
		// value to split is first on stack
		DevaObject val = get_arg_of_type( ex, "string.split", "value", sym_string );
		chars = val.str_val;
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
		size_t len = in.length();
		// lhs of the first item is always the first character
		size_t left = 0;
		// rhs of the first item is the first splitting character
		size_t right = in.find_first_of( chars );
		if( right == string::npos )
			right = len;
		// while lhs is not at the end of the input string
		while( left <= len )
		{
			ret->push_back( DevaObject( "", string( in, left, right - left ) ) );
			left = right + 1;
			right = in.find_first_of( chars, right + 1 );
			if( right == string::npos )
				right = len;
		}
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_replace( Frame *frame )
{
	if( Frame::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to string 'replace' built-in method." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "replace", sym_string );

	// search string is next
	DevaObject srch = get_arg_of_type( ex, "string.replace", "search_value", sym_string );

	// replacement value is next on stack
	DevaObject val = get_arg_of_type( ex, "string.replace", "replacement_value", sym_string );
	
	// do the replacement
	string ret( str.str_val );
	replace( ret, srch.str_val, val.str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the new string
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_string_upper( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'upper'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "upper", sym_string );

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

void do_string_lower( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'lower'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "lower", sym_string );

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


void do_string_isalphanum( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isalphanum'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isalphanum", sym_string );

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

void do_string_isalpha( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isalpha'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isalpha", sym_string );

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

void do_string_isdigit( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isdigit'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isdigit", sym_string );

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

void do_string_islower( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'islower'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "islower", sym_string );

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

void do_string_isupper( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isupper'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isupper", sym_string );

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

void do_string_isspace( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isspace'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isspace", sym_string );

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

void do_string_ispunct( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'ispunct'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "ispunct", sym_string );

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

void do_string_iscntrl( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'iscntrl'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "ispunct", sym_string );

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

void do_string_isprint( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isprint'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isprint", sym_string );

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

void do_string_isxdigit( Frame *frame )
{
	if( Frame::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'isxdigit'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "isxdigit", sym_string );

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

void do_string_format( Frame *frame )
{
	if( Frame::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'format'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "format", sym_string );

	// get the format_args vector off the stack next
	DevaObject args = get_arg_of_type( ex, "string.format", "format_arguments", sym_vector );

	// format the string using boost's format library
	boost::format formatter;
	try
	{
		formatter = boost::format( str.str_val );
		for( DOVector::iterator i = args.vec_val->begin(); i != args.vec_val->end(); ++i )
		{
			formatter % *i;
		}
	}
	catch( boost::io::bad_format_string & e )
	{
		throw DevaRuntimeException( "The format of the string in string built-in method 'format' was invalid." );
	}
	catch( boost::io::too_many_args & e )
	{
		throw DevaRuntimeException( "The format of the string in string built-in method 'format' referred to fewer parameters than were passed in the parameter vector." );
	}
	catch( boost::io::too_few_args & e )
	{
		throw DevaRuntimeException( "The format of the string in string built-in method 'format' referred to more parameters than were passed in the parameter vector." );
	}

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", boost::str( formatter ) ) );
}

void do_string_join( Frame *frame )
{
	if( Frame::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to string built-in method 'join'." );

	// get the string object off the top of the stack
	DevaObject str = get_this( ex, "join", sym_string );

	// get the vector of objects to join off the stack
	DevaObject args = get_arg_of_type( ex, "string.join", "args", sym_vector );

	string ret;
	for( DOVector::iterator i = args.vec_val->begin(); i != args.vec_val->end(); ++i )
	{
		ostringstream s;
		if( i != args.vec_val->begin() )
			s << str.str_val;
		s << *i;
		ret += s.str();
	}

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", ret ) );
}
*/

} // end namespace deva
