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

// string_builtins.h
// builtin string methods for the deva language
// created by jcs, march 6, 2011

// TODO:
// * 

#ifndef __STRING_BUILTINS_H__ 
#define __STRING_BUILTINS_H__

#include "object.h"
#include <string>


using namespace std;


namespace deva
{


// to add new builtins you must:
// 1) add a new fcn to the builtin_names and builtin_fcns arrays below
// 2) implement the function in this file

// pre-decls:
class Frame;

// pre-decls for builtin executors
void do_string_concat( Frame *ex );
void do_string_length( Frame *ex );
void do_string_copy( Frame *ex );
void do_string_insert( Frame *ex );
void do_string_remove( Frame *ex );
//void do_string_find( Frame *ex );
//void do_string_rfind( Frame *ex );
//void do_string_reverse( Frame *ex );
//void do_string_sort( Frame *ex );
//void do_string_slice( Frame *ex );
//void do_string_strip( Frame *ex );
//void do_string_lstrip( Frame *ex );
//void do_string_rstrip( Frame *ex );
//void do_string_split( Frame *ex );
//void do_string_replace( Frame *ex );
//void do_string_upper( Frame *ex );
//void do_string_lower( Frame *ex );
//void do_string_isalphanum( Frame *ex );
//void do_string_isalpha( Frame *ex );
//void do_string_isdigit( Frame *ex );
//void do_string_islower( Frame *ex );
//void do_string_isupper( Frame *ex );
//void do_string_isspace( Frame *ex );
//void do_string_ispunct( Frame *ex );
//void do_string_iscntrl( Frame *ex );
//void do_string_isprint( Frame *ex );
//void do_string_isxdigit( Frame *ex );
//void do_string_format( Frame *ex );
//void do_string_join( Frame *ex );

static const string string_builtin_names[] = 
{
	string( "concat" ),
	string( "length" ),
	string( "copy" ),
	string( "insert" ),
	string( "remove" ),
	string( "find" ),
	string( "rfind" ),
	string( "reverse" ),
	string( "sort" ),
	string( "slice" ),
	string( "strip" ),
	string( "lstrip" ),
	string( "rstrip" ),
	string( "split" ),
	string( "replace" ),
	string( "upper" ),
	string( "lower" ),
	string( "isalphanum" ),
	string( "isalpha" ),
	string( "isdigit" ),
	string( "islower" ),
	string( "isupper" ),
	string( "isspace" ),
	string( "ispunct" ),
	string( "iscntrl" ),
	string( "isprint" ),
	string( "isxdigit" ),
	string( "format" ),
	string( "join" ),
};
// ...and function pointers to the executor functions for them
static NativeFunctionPtr string_builtin_fcns[] = 
{
	do_string_concat,
	do_string_length,
	do_string_copy,
	do_string_insert,
	do_string_remove,
//	do_string_find,
//	do_string_rfind,
//	do_string_reverse,
//	do_string_sort,
//	do_string_slice,
//	do_string_strip,
//	do_string_lstrip,
//	do_string_rstrip,
//	do_string_split,
//	do_string_replace,
//	do_string_upper,
//	do_string_lower,
//	do_string_isalphanum,
//	do_string_isalpha,
//	do_string_isdigit,
//	do_string_islower,
//	do_string_isupper,
//	do_string_isspace,
//	do_string_ispunct,
//	do_string_iscntrl,
//	do_string_isprint,
//	do_string_isxdigit,
//	do_string_format,
//	do_string_join,
};
static Object string_builtin_fcn_objs[] = 
{
	Object( do_string_concat ),
	Object( do_string_length ),
	Object( do_string_copy ),
	Object( do_string_insert ),
	Object( do_string_remove ),
//	Object( do_string_find ),
//	Object( do_string_rfind ),
//	Object( do_string_reverse ),
//	Object( do_string_sort ),
//	Object( do_string_slice ),
//	Object( do_string_strip ),
//	Object( do_string_lstrip ),
//	Object( do_string_rstrip ),
//	Object( do_string_split ),
//	Object( do_string_replace ),
//	Object( do_string_upper ),
//	Object( do_string_lower ),
//	Object( do_string_isalphanum ),
//	Object( do_string_isalpha ),
//	Object( do_string_isdigit ),
//	Object( do_string_islower ),
//	Object( do_string_isupper ),
//	Object( do_string_isspace ),
//	Object( do_string_ispunct ),
//	Object( do_string_iscntrl ),
//	Object( do_string_isprint ),
//	Object( do_string_isxdigit ),
//	Object( do_string_format ),
//	Object( do_string_join ),
};
const int num_of_string_builtins = sizeof( string_builtin_names ) / sizeof( string_builtin_names[0] );

// is a given name a builtin function?
bool IsStringBuiltin( const string & name );

// get the native function ptr
NativeFunction GetStringBuiltin( const string & name );

// get an Object* for the fcn
Object* GetStringBuiltinObjectRef( const string & name );


} // end namespace deva

#endif // __STRING_BUILTINS__
