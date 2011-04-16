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

// builtins.h
// builtin functions for the deva language
// created by jcs, december 29, 2010 

// TODO:
// * 

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

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
void do_print( Frame *f );
void do_str( Frame *f );
void do_chr( Frame *f );
void do_append( Frame *f );
void do_length( Frame *f );
void do_copy( Frame *f );
void do_name( Frame *f );
void do_type( Frame *f );
void do_exit( Frame *f );
void do_num( Frame *f );
void do_range( Frame *f );
void do_eval( Frame *f );
void do_open( Frame *f );
void do_close( Frame *f );
void do_flush( Frame *f );
void do_read( Frame *f );
void do_readstring( Frame *f );
void do_readline( Frame *f );
void do_readlines( Frame *f );
void do_write( Frame *f );
void do_writestring( Frame *f );
void do_writeline( Frame *f );
void do_writelines( Frame *f );
void do_seek( Frame *f );
void do_tell( Frame *f );
void do_stdin( Frame *f );
void do_stdout( Frame *f );
void do_stderr( Frame *f );
void do_format( Frame *f );
void do_join( Frame *f );
void do_error( Frame *f );
void do_seterror( Frame *f );
void do_geterror( Frame *f );
void do_importmodule( Frame *f );
void do_is_null( Frame* f );
void do_is_boolean( Frame* f );
void do_is_number( Frame* f );
void do_is_string( Frame* f );
void do_is_vector( Frame* f );
void do_is_map( Frame* f );
void do_is_function( Frame* f );
void do_is_native_function( Frame* f );
void do_is_class( Frame* f );
void do_is_instance( Frame* f );
void do_is_native_obj( Frame* f );
void do_is_size( Frame* f );
void do_is_symbol_name( Frame* f );

extern const string builtin_names[];
// ...and function pointers to the executor functions for them
extern NativeFunction builtin_fcns[];
extern Object builtin_fcn_objs[];
extern const int num_of_builtins;

// is a given name a builtin function?
bool IsBuiltin( const string & name );
NativeFunction GetBuiltin( const string & name );
Object* GetBuiltinObjectRef( const string & name );

} // end namespace deva

#endif // __BUILTINS_H__
