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
	helper.ExpectPositiveIntegralNumber( pos );
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
	helper.ExpectPositiveIntegralNumber( startobj );

	int start = (int)startobj->d;
	// end arg defaults to -1 (same as end-of-string)
	int end = -1;
	if( frame->NumArgsPassed() == 3 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = strlen( self->s );
	if( end == -1 )
		end = sz;

	if( start >= sz )
		throw RuntimeException( "Invalid 'start' argument in string built-in method 'remove'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in string built-in method 'remove'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in string built-in method 'remove': start is greater than end." );

	// remove the chars
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	s.erase( start, end );
	const char* ret = frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( ret ) );
}

void do_string_find( Frame *frame )
{
	BuiltinHelper helper( "string", "find", frame );

	helper.CheckNumberOfArguments( 2, 4 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// value to search for is first arg
	Object* val = helper.GetLocalN( 1 );
	helper.ExpectType( val, obj_string );

	// start arg defaults to 0
	int start = 0;
	if( frame->NumArgsPassed() > 2 )
	{
		Object* startobj = helper.GetLocalN( 2 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}

	// substring length arg defaults to -1 (same as end-of-string)
	int len = -1;
	if( frame->NumArgsPassed() > 3 )
	{
		Object* lenobj = helper.GetLocalN( 3 );
		helper.ExpectIntegralNumber( lenobj );
		len = (int)lenobj->d;
	}

	size_t sz = strlen( self->s );
	size_t sz_val = strlen( val->s );
	// if a sub-string length wasn't passed, use the entire search string
	if( len == -1 )
		len = sz_val;
	// nothing to find in an empty string, and an empty string will never be
	// found
	if( sz == 0 || sz_val == 0 || len == 0 )
	{
		helper.ReturnVal( Object( obj_null ) );
	}
	else
	{
		if( start >= sz )
			throw RuntimeException( "Invalid 'start' argument in string built-in method 'find'." );
		if( len > sz_val || len < 0 )
			throw RuntimeException( "Invalid 'length' argument in string built-in method 'find'." );

		// find the matching sub-string
		string s( self->s );
		size_t fpos = s.find( val->s, start, len );
		if( fpos == string::npos )
			helper.ReturnVal( Object( obj_null ) );
		else
		{
			helper.ReturnVal( Object( (double)fpos ) );
		}
	}
}

void do_string_rfind( Frame *frame )
{
	BuiltinHelper helper( "string", "rfind", frame );

	helper.CheckNumberOfArguments( 2, 4 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// value to search for is first arg
	Object* val = helper.GetLocalN( 1 );
	helper.ExpectType( val, obj_string );

	// start arg defaults to end-of-string
	long start = string::npos;
	if( frame->NumArgsPassed() > 2 )
	{
		Object* startobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( startobj );
		start = (long)startobj->d;
	}

	// substring length arg defaults to -1 (same as end-of-string)
	int len = -1;
	if( frame->NumArgsPassed() > 3 )
	{
		Object* lenobj = helper.GetLocalN( 3 );
		helper.ExpectIntegralNumber( lenobj );
		len = (int)lenobj->d;
	}

	size_t sz = strlen( self->s );
	size_t sz_val = strlen( val->s );
	// convert a start val of '-1' into end-of-string
	if( start == -1 )
		start = string::npos;
	// if a sub-string length wasn't passed, use the entire search string
	if( len == -1 )
		len = sz_val;
	// nothing to find in an empty string, and an empty string will never be
	// found
	if( sz == 0 || sz_val == 0 || len == 0 )
	{
		helper.ReturnVal( Object( obj_null ) );
	}
	else
	{
		if( (start >= sz && start != string::npos) || start < -1 )
			throw RuntimeException( "Invalid 'start' argument in string built-in method 'rfind'." );
		if( (len > sz_val && len != string::npos) || len < 0 )
			throw RuntimeException( "Invalid 'length' argument in string built-in method 'rfind'." );

		// find the matching sub-string
		string s( self->s );
		size_t fpos = s.rfind( val->s, start, len );
		if( fpos == string::npos )
			helper.ReturnVal( Object( obj_null ) );
		else
		{
			helper.ReturnVal( Object( (double)fpos ) );
		}
	}
}

void do_string_reverse( Frame *frame )
{
	BuiltinHelper helper( "string", "reverse", frame );

	helper.CheckNumberOfArguments( 1, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// start arg defaults to 0
	int start = 0;
	if( frame->NumArgsPassed() > 1 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}

	// end arg defaults to -1 (same as end-of-string)
	int end = -1;
	if( frame->NumArgsPassed() > 2 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = strlen( self->s );
	if( end == -1 )
		end = sz;

	if( start >= sz )
		throw RuntimeException( "Invalid 'start' argument in string built-in method 'reverse'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in string built-in method 'reverse'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in string built-in method 'reverse': start is greater than end." );

	// reverse the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	reverse( s.begin() + start, s.begin() + end );
	const char* ret = frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( ret ) );
}

void do_string_sort( Frame *frame )
{
	BuiltinHelper helper( "string", "sort", frame );

	helper.CheckNumberOfArguments( 1, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// start arg defaults to 0
	int start = 0;
	if( frame->NumArgsPassed() > 1 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}

	// end arg defaults to -1 (same as end-of-string)
	int end = -1;
	if( frame->NumArgsPassed() > 2 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = strlen( self->s );
	if( end == -1 )
		end = sz;

	if( start >= sz )
		throw RuntimeException( "Invalid 'start' argument in string built-in method 'sort'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in string built-in method 'sort'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in string built-in method 'sort': start is greater than end." );

	// sort the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	sort( s.begin() + start, s.begin() + end );
	const char* ret = frame->GetParent()->AddString( s );
	
	helper.ReturnVal( Object( ret ) );
}

void do_string_slice( Frame *frame )
{
	BuiltinHelper helper( "string", "slice", frame );

	helper.CheckNumberOfArguments( 2, 4 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// start arg defaults to 0
	int start = 0;
	if( frame->NumArgsPassed() > 1 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}

	// end arg defaults to -1 (same as end-of-string)
	int end = -1;
	if( frame->NumArgsPassed() > 2 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	// step arg defaults to 1
	int step = 1;
	if( frame->NumArgsPassed() > 3 )
	{
		Object* stepobj = helper.GetLocalN( 3 );
		helper.ExpectPositiveIntegralNumber( stepobj );
		step = (int)stepobj->d;
	}

	size_t sz = strlen( self->s );
	if( end == -1 )
		end = sz;

	if( start >= sz )
		throw RuntimeException( "Invalid 'start' argument in string built-in method 'slice'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in string built-in method 'slice'." );
	if( step > sz )
		throw RuntimeException( "Invalid 'step' argument in string built-in method 'slice'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in string built-in method 'slice': start is greater than end." );

	// slice the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	if( step == 1 )
	{
		string r = s.substr( start, end - start );
		const char* ret = frame->GetParent()->AddString( r );
		helper.ReturnVal( Object( ret ) );
	}
	// otherwise the string class doesn't help us, have to do it manually
	else
	{
		// first get the substring from start to end positions
		string r = s.substr( start, end - start );
		// TODO: call 'reserve' on the string to reduce allocations?
		// then walk it grabbing every 'nth' character
		string slice;
		for( int i = 0; i < r.length(); i += step )
		{
			slice += r[i];
		}
		const char* ret = frame->GetParent()->AddString( slice );
		helper.ReturnVal( Object( ret ) );
	}
}

void do_string_strip( Frame *frame )
{
	BuiltinHelper helper( "string", "strip", frame );

	helper.CheckNumberOfArguments( 1, 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// chars arg defaults to " \t\n" (whitespace characters)
	string chars;
	if( frame->NumArgsPassed() == 2 )
	{
		Object* charobj = helper.GetLocalN( 1 );
		helper.ExpectType( charobj, obj_string );
		chars = charobj->s;
	}
	else
		chars = " \t\n";

	// strip the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	string out;
	// left-side strip
	size_t left = s.find_first_not_of( chars );
	// if there aren't any characters that aren't in our stripped list, return
	// an empty string
	if( left != string::npos )
	{
		// right-side strip
		size_t right = s.find_last_not_of( chars );
		out = s.substr( left, right - left + 1 );
	}
	const char* ret = frame->GetParent()->AddString( out );
	helper.ReturnVal( Object( ret ) );
}

void do_string_lstrip( Frame *frame )
{
	BuiltinHelper helper( "string", "lstrip", frame );

	helper.CheckNumberOfArguments( 1, 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// chars arg defaults to " \t\n" (whitespace characters)
	string chars;
	if( frame->NumArgsPassed() == 2 )
	{
		Object* charobj = helper.GetLocalN( 1 );
		helper.ExpectType( charobj, obj_string );
		chars = charobj->s;
	}
	else
		chars = " \t\n";

	// strip the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	string out;
	// left-side strip
	size_t left = s.find_first_not_of( chars );
	// if there aren't any characters that aren't in our stripped list, return
	// an empty string
	if( left != string::npos )
	{
		// right-side
		size_t right = s.length();
		out = s.substr( left, right - left + 1 );
	}
	const char* ret = frame->GetParent()->AddString( out );
	helper.ReturnVal( Object( ret ) );
}

void do_string_rstrip( Frame *frame )
{
	BuiltinHelper helper( "string", "rstrip", frame );

	helper.CheckNumberOfArguments( 1, 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// chars arg defaults to " \t\n" (whitespace characters)
	string chars;
	if( frame->NumArgsPassed() == 2 )
	{
		Object* charobj = helper.GetLocalN( 1 );
		helper.ExpectType( charobj, obj_string );
		chars = charobj->s;
	}
	else
		chars = " \t\n";

	// strip the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	string out;
	// right-side strip
	size_t right = s.find_last_not_of( chars );
	// if there aren't any characters that aren't in our stripped list, return
	// an empty string
	if( right != string::npos )
	{
		// right-side
		out = s.substr( 0, right + 1 );
	}
	const char* ret = frame->GetParent()->AddString( out );
	helper.ReturnVal( Object( ret ) );
}

void do_string_split( Frame *frame )
{
	BuiltinHelper helper( "string", "split", frame );

	helper.CheckNumberOfArguments( 1, 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// chars arg defaults to " \t\n" (whitespace characters)
	string chars;
	if( frame->NumArgsPassed() == 2 )
	{
		Object* charobj = helper.GetLocalN( 1 );
		helper.ExpectType( charobj, obj_string );
		chars = charobj->s;
	}
	else
		chars = " \t\n";

	// return vector
	Vector* ret = CreateVector();

	// split the string
	string s( self->s );

	// special case empty string, which means split into a vector of individual
	// characters, NOT a vector containing only the original string
	if( chars.length() == 0 )
	{
		ret->reserve( s.length() );
		for( int c = 0; c < s.length(); ++c )
		{
			string out( 1, s[c] );
			const char* retstr = frame->GetParent()->AddString( out );
			ret->push_back( Object( retstr ) );
		}
	}
	else
	{
		size_t len = s.length();
		// lhs of the first item is always the first character
		size_t left = 0;
		// rhs of the first item is the first splitting character
		size_t right = s.find_first_of( chars );
		if( right == string::npos )
			right = len;
		// while lhs is not at the end of the input string
		while( left <= len )
		{
			string out( s, left, right - left );
			const char* retstr = frame->GetParent()->AddString( out );
			ret->push_back( Object( retstr ) );
			left = right + 1;
			right = s.find_first_of( chars, right + 1 );
			if( right == string::npos )
				right = len;
		}
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_replace( Frame *frame )
{
	BuiltinHelper helper( "string", "replace", frame );

	helper.CheckNumberOfArguments( 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// search string
	Object* srch = helper.GetLocalN( 1 );
	helper.ExpectType( srch, obj_string );

	// replacement string
	Object* val = helper.GetLocalN( 2 );
	helper.ExpectType( val, obj_string );

	// do the replacement
	// (strings are immutable. create a copy and add it to the calling frame)
	string s( self->s );
	replace( s, srch->s, val->s );
	const char* ret = frame->GetParent()->AddString( s );

	helper.ReturnVal( Object( ret ) );
}

void do_string_upper( Frame *frame )
{
	BuiltinHelper helper( "string", "upper", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// uppercase the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s;
	s.reserve( strlen( self->s ) );
	locale loc;
	int i = 0;
	while( self->s[i] )
	{
		s.push_back( toupper( self->s[i], loc ) );
		++i;
	}
	const char* ret = frame->GetParent()->AddString( s );

	helper.ReturnVal( Object( ret ) );
}

void do_string_lower( Frame *frame )
{
	BuiltinHelper helper( "string", "upper", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	// lowercase the string
	// (strings are immutable. create a copy and add it to the calling frame)
	string s;
	s.reserve( strlen( self->s ) );
	locale loc;
	int i = 0;
	while( self->s[i] )
	{
		s.push_back( tolower( self->s[i], loc ) );
		++i;
	}
	const char* ret = frame->GetParent()->AddString( s );

	helper.ReturnVal( Object( ret ) );
}

void do_string_isalphanum( Frame *frame )
{
	BuiltinHelper helper( "string", "isalphanum", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isalnum( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_isalpha( Frame *frame )
{
	BuiltinHelper helper( "string", "isalpha", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isalpha( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_isdigit( Frame *frame )
{
	BuiltinHelper helper( "string", "isdigit", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isdigit( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_islower( Frame *frame )
{
	BuiltinHelper helper( "string", "islower", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !islower( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_isupper( Frame *frame )
{
	BuiltinHelper helper( "string", "isupper", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isupper( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_isspace( Frame *frame )
{
	BuiltinHelper helper( "string", "isspace", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isspace( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_ispunct( Frame *frame )
{
	BuiltinHelper helper( "string", "ispunct", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !ispunct( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_iscntrl( Frame *frame )
{
	BuiltinHelper helper( "string", "iscntrl", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !iscntrl( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_isprint( Frame *frame )
{
	BuiltinHelper helper( "string", "isprint", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isprint( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_isxdigit( Frame *frame )
{
	BuiltinHelper helper( "string", "isxdigit", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );

	locale loc;
	int i = 0;
	bool ret = true;
	while( self->s[i] )
	{
		if( !isxdigit( self->s[i], loc ) )
		{
			ret = false;
			break;
		}
		++i;
	}

	helper.ReturnVal( Object( ret ) );
}

void do_string_format( Frame *frame )
{
	BuiltinHelper helper( "string", "format", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );
	Object* args = helper.GetLocalN( 1 );
	helper.ExpectType( args, obj_vector );

	// format the string using boost's format library
	boost::format formatter;
	try
	{
		formatter = boost::format( self->s );
		for( Vector::iterator i = args->v->begin(); i != args->v->end(); ++i )
		{
			formatter % *i;
		}
	}
	catch( boost::io::bad_format_string & e )
	{
		throw RuntimeException( "The format of the string in string built-in method 'format' was invalid." );
	}
	catch( boost::io::too_many_args & e )
	{
		throw RuntimeException( "The format of the string in string built-in method 'format' referred to fewer parameters than were passed in the parameter vector." );
	}
	catch( boost::io::too_few_args & e )
	{
		throw RuntimeException( "The format of the string in string built-in method 'format' referred to more parameters than were passed in the parameter vector." );
	}

	const char* ret = frame->GetParent()->AddString( boost::str( formatter ) );

	helper.ReturnVal( Object( ret ) );
}

void do_string_join( Frame *frame )
{
	BuiltinHelper helper( "string", "join", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_string );
	Object* args = helper.GetLocalN( 1 );
	helper.ExpectType( args, obj_vector );

	string retstr;
	for( Vector::iterator i = args->v->begin(); i != args->v->end(); ++i )
	{
		ostringstream s;
		if( i != args->v->begin() )
			s << self->s;
		s << *i;
		retstr += s.str();
	}

	const char* ret = frame->GetParent()->AddString( retstr );

	helper.ReturnVal( Object( ret ) );
}


} // end namespace deva
