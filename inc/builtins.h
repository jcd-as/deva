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
//void do_eval( Frame *f );
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
//void do_error( Frame *f );
//void do_seterror( Frame *f );
//void do_geterror( Frame *f );
//void do_importmodule( Frame *f );

static const string builtin_names[] = 
{
	string( "print" ),
	string( "str" ),
	string( "chr" ),
	string( "append" ),
	string( "length" ),
	string( "copy" ),
	string( "name" ),
	string( "type" ),
	string( "exit" ),
	string( "num" ),
	string( "range" ),
	string( "eval" ),
	string( "open" ),
	string( "close" ),
	string( "flush" ),
	string( "read" ),
	string( "readstring" ),
	string( "readline" ),
	string( "readlines" ),
	string( "write" ),
	string( "writestring" ),
	string( "writeline" ),
	string( "writelines" ),
	string( "seek" ),
	string( "tell" ),
	string( "stdin" ),
	string( "stdout" ),
	string( "stderr" ),
	string( "format" ),
	string( "join" ),
	string( "error" ),
	string( "seterror" ),
	string( "geterror" ),
	string( "importmodule" ),
};
// ...and function pointers to the executor functions for them
static NativeFunction builtin_fcns[] = 
{
	{do_print, false},
	{do_str, false},
	{do_chr, false},
	{do_append, false},
	{do_length, false},
	{do_copy, false},
	{do_name, false},
	{do_type, false},
	{do_exit, false},
	{do_num, false},
	{do_range, false},
//	{do_eval, false},
	{do_open, false},
	{do_close, false},
	{do_flush, false},
	{do_read, false},
	{do_readstring, false},
	{do_readline, false},
	{do_readlines, false},
	{do_write, false},
	{do_writestring, false},
	{do_writeline, false},
	{do_writelines, false},
	{do_seek, false},
	{do_tell, false},
	{do_stdin, false},
	{do_stdout, false},
	{do_stderr, false},
	{do_format, false},
	{do_join, false},
//	{do_error, false},
//	{do_seterror, false},
//	{do_geterror, false},
//	{do_importmodule, false},
};
static Object builtin_fcn_objs[] = 
{
	Object( do_print ),
	Object( do_str ),
	Object( do_chr ),
	Object( do_append ),
	Object( do_length ),
	Object( do_copy ),
	Object( do_name ),
	Object( do_type ),
	Object( do_exit ),
	Object( do_num ),
	Object( do_range ),
//	Object( do_eval ),
	Object( do_open ),
	Object( do_close ),
	Object( do_flush ),
	Object( do_read ),
	Object( do_readstring ),
	Object( do_readline ),
	Object( do_readlines ),
	Object( do_write ),
	Object( do_writestring ),
	Object( do_writeline ),
	Object( do_writelines ),
	Object( do_seek ),
	Object( do_tell ),
	Object( do_stdin ),
	Object( do_stdout ),
	Object( do_stderr ),
	Object( do_format ),
	Object( do_join ),
//	Object( do_error ),
//	Object( do_seterror ),
//	Object( do_geterror ),
//	Object( do_importmodule ),
};
const int num_of_builtins = sizeof( builtin_names ) / sizeof( builtin_names[0] );

// is a given name a builtin function?
bool IsBuiltin( const string & name );
NativeFunction GetBuiltin( const string & name );
Object* GetBuiltinObjectRef( const string & name );

} // end namespace deva

#endif // __BUILTINS_H__
