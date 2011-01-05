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

#include "executor.h"
#include <string>

using namespace std;


namespace deva
{

// to add new builtins you must:
// 1) add a new fcn to the builtin_names and builtin_fcns arrays below
// 2) implement the function in this file

// pre-decls for builtin executors
void do_print( Frame *f );
void do_str( Frame *f );
//void do_chr( Frame *f );
//void do_append( Frame *f );
//void do_length( Frame *f );
//void do_copy( Frame *f );
//void do_eval( Frame *f );
//void do_open( Frame *f );
//void do_close( Frame *f );
//void do_flush( Frame *f );
//void do_read( Frame *f );
//void do_readstring( Frame *f );
//void do_readline( Frame *f );
//void do_readlines( Frame *f );
//void do_write( Frame *f );
//void do_writestring( Frame *f );
//void do_writeline( Frame *f );
//void do_writelines( Frame *f );
//void do_seek( Frame *f );
//void do_tell( Frame *f );
//void do_name( Frame *f );
//void do_type( Frame *f );
//void do_stdin( Frame *f );
//void do_stdout( Frame *f );
//void do_stderr( Frame *f );
//void do_exit( Frame *f );
//void do_num( Frame *f );
//void do_range( Frame *f );
//void do_format( Frame *f );
//void do_join( Frame *f );
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
	string( "name" ),
	string( "type" ),
	string( "stdin" ),
	string( "stdout" ),
	string( "stderr" ),
	string( "exit" ),
	string( "num" ),
	string( "range" ),
	string( "format" ),
	string( "join" ),
	string( "error" ),
	string( "seterror" ),
	string( "geterror" ),
	string( "importmodule" ),
};
// ...and function pointers to the executor functions for them
static NativeFunctionPtr builtin_fcns[] = 
{
	do_print,
	do_str,
//	do_chr,
//	do_append,
//	do_length,
//	do_copy,
//	do_eval,
//	do_open,
//	do_close,
//	do_flush,
//	do_read,
//	do_readstring,
//	do_readline,
//	do_readlines,
//	do_write,
//	do_writestring,
//	do_writeline,
//	do_writelines,
//	do_seek,
//	do_tell,
//	do_name,
//	do_type,
//	do_stdin,
//	do_stdout,
//	do_stderr,
//	do_exit,
//	do_num,
//	do_range,
//	do_format,
//	do_join,
//	do_error,
//	do_seterror,
//	do_geterror,
//	do_importmodule,
};
const int num_of_builtins = sizeof( builtin_names ) / sizeof( builtin_names[0] );

// is a given name a builtin function?
bool IsBuiltin( const string & name );

} // end namespace deva

#endif // __BUILTINS_H__
