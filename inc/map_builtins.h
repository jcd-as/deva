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

// map_builtins.h
// builtin map methods for the deva language
// created by jcs, january 5, 2011

// TODO:
// * 

#ifndef __MAP_BUILTINS_H__ 
#define __MAP_BUILTINS_H__

#include "executor.h"
#include <string>

using namespace std;


namespace deva
{


// to add new builtins you must:
// 1) add a new fcn to the builtin_names and builtin_fcns arrays below
// 2) implement the function in this file


// pre-decls for builtin executors
void do_map_length( Frame *frame );
//void do_map_copy( Frame *frame );
//void do_map_remove( Frame *frame );
//void do_map_find( Frame *frame );
//void do_map_keys( Frame *frame );
//void do_map_values( Frame *frame );
//void do_map_merge( Frame *frame );
void do_map_rewind( Frame *frame );
void do_map_next( Frame *frame );


// tables defining the built-in function names...
static const string map_builtin_names[] = 
{
	string( "length" ),
//	string( "copy" ),
//	string( "remove" ),
//	string( "find" ),
//	string( "keys" ),
//	string( "values" ),
//	string( "merge" ),
	string( "rewind" ),
	string( "next" ),
};
// ...and function pointers to the executor functions for them
static NativeFunctionPtr map_builtin_fcns[] = 
{
	do_map_length,
//	do_map_copy,
//	do_map_remove,
//	do_map_find,
//	do_map_keys,
//	do_map_values,
//	do_map_merge,
	do_map_rewind,
	do_map_next,
};
const int num_of_map_builtins = sizeof( map_builtin_names ) / sizeof( map_builtin_names[0] );

// is a given name a builtin function?
bool IsMapBuiltin( const string & name );

// get the native function ptr
NativeFunction GetMapBuiltin( const string & name );

} // end namespace deva

#endif // __MAP_BUILTINS_H__

