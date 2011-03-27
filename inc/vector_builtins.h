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

// vector_builtins.h
// builtin vector methods for the deva language
// created by jcs, january 3, 2011

// TODO:
// * 

#ifndef __VECTOR_BUILTINS_H__ 
#define __VECTOR_BUILTINS_H__

//#include "executor.h"
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
void do_vector_append( Frame *frame );
void do_vector_length( Frame *frame );
void do_vector_copy( Frame *frame );
void do_vector_concat( Frame *frame );
void do_vector_min( Frame *frame );
void do_vector_max( Frame *frame );
void do_vector_pop( Frame *frame );
void do_vector_insert( Frame *frame );
void do_vector_remove( Frame *frame );
void do_vector_find( Frame *frame );
void do_vector_rfind( Frame *frame );
void do_vector_count( Frame *frame );
void do_vector_reverse( Frame *frame );
void do_vector_sort( Frame *frame );
void do_vector_map( Frame *frame );
void do_vector_filter( Frame *frame );
void do_vector_reduce( Frame *frame );
void do_vector_any( Frame *frame );
void do_vector_all( Frame *frame );
void do_vector_slice( Frame *frame );
void do_vector_join( Frame *frame );
// 'enumerable interface'
void do_vector_rewind( Frame *frame );
void do_vector_next( Frame *frame );

// arrays containing
// the names of the vector builtins...
extern const string vector_builtin_names[];
// ...the function pointers to the executor functions for them...
extern NativeFunctionPtr vector_builtin_fcns[];
// ...and function objects for them
extern Object vector_builtin_fcn_objs[];
extern const int num_of_vector_builtins;

// is a given name a builtin function?
bool IsVectorBuiltin( const string & name );

// get the native function ptr
NativeFunction GetVectorBuiltin( const string & name );

// get an Object* for the fcn
Object* GetVectorBuiltinObjectRef( const string & name );


} // end namespace deva

#endif // __VECTOR_BUILTINS_H__
