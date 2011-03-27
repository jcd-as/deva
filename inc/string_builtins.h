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
void do_string_find( Frame *ex );
void do_string_rfind( Frame *ex );
void do_string_reverse( Frame *ex );
void do_string_sort( Frame *ex );
void do_string_slice( Frame *ex );
void do_string_strip( Frame *ex );
void do_string_lstrip( Frame *ex );
void do_string_rstrip( Frame *ex );
void do_string_split( Frame *ex );
void do_string_replace( Frame *ex );
void do_string_upper( Frame *ex );
void do_string_lower( Frame *ex );
void do_string_isalphanum( Frame *ex );
void do_string_isalpha( Frame *ex );
void do_string_isdigit( Frame *ex );
void do_string_islower( Frame *ex );
void do_string_isupper( Frame *ex );
void do_string_isspace( Frame *ex );
void do_string_ispunct( Frame *ex );
void do_string_iscntrl( Frame *ex );
void do_string_isprint( Frame *ex );
void do_string_isxdigit( Frame *ex );
void do_string_format( Frame *ex );
void do_string_join( Frame *ex );

// tables defining the builtin names...
extern const string string_builtin_names[];
// ...function pointers to the executor functions for them...
extern NativeFunctionPtr string_builtin_fcns[];
// ...and function objects
extern Object string_builtin_fcn_objs[];
extern const int num_of_string_builtins;

// is a given name a builtin function?
bool IsStringBuiltin( const string & name );

// get the native function ptr
NativeFunction GetStringBuiltin( const string & name );

// get an Object* for the fcn
Object* GetStringBuiltinObjectRef( const string & name );


} // end namespace deva

#endif // __STRING_BUILTINS__
