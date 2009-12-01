// Copyright (c) 2009 Joshua C. Shepard
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

// builtins.cpp
// built-in functions for the deva language virtual machine
// created by jcs, october 04, 2009 

// TODO:
// * 

#include "builtins.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// to add new builtins you must:
// 1) add a new fcn to the builtin_names and builtin_fcns arrays below
// 2) implement the function in this file

// pre-decls for builtin executors
void do_print( Executor *ex );
void do_str( Executor *ex );
void do_chr( Executor *ex );
void do_append( Executor *ex );
void do_length( Executor *ex );
void do_copy( Executor *ex );
void do_eval( Executor *ex );
void do_delete( Executor *ex );
void do_open( Executor *ex );
void do_close( Executor *ex );
void do_flush( Executor *ex );
void do_read( Executor *ex );
void do_readstring( Executor *ex );
void do_readline( Executor *ex );
void do_readlines( Executor *ex );
void do_write( Executor *ex );
void do_writestring( Executor *ex );
void do_writeline( Executor *ex );
void do_writelines( Executor *ex );
void do_seek( Executor *ex );
void do_tell( Executor *ex );
void do_name( Executor *ex );
void do_type( Executor *ex );

// tables defining the built-in function names...
static const string builtin_names[] = 
{
	string( "print" ),
    string( "str" ),
    string( "chr" ),
    string( "append" ),
    string( "length" ),
    string( "copy" ),
    string( "eval" ),
    string( "delete" ),
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
};
// ...and function pointers to the executor functions for them
//typedef void (*builtin_fcn)(Executor*, const Instruction&);
builtin_fcn builtin_fcns[] = 
{
    do_print,
    do_str,
    do_chr,
    do_append,
    do_length,
    do_copy,
	do_eval,
	do_delete,
	do_open,
	do_close,
	do_flush,
	do_read,
	do_readstring,
	do_readline,
	do_readlines,
	do_write,
	do_writestring,
	do_writeline,
	do_writelines,
	do_seek,
	do_tell,
	do_name,
	do_type,
};
const int num_of_builtins = sizeof( builtin_names ) / sizeof( builtin_names[0] );

// is this name a built-in function?
bool is_builtin( const string & name )
{
    const string* i = find( builtin_names, builtin_names + num_of_builtins, name );
    if( i != builtin_names + num_of_builtins ) return true;
	else return false;
}

// execute built-in function
void execute_builtin( Executor *ex, const Instruction & inst )
{
    // find the name of the fcn
	string name = inst.args[0].name;
    const string* i = find( builtin_names, builtin_names + num_of_builtins, name );
    if( i == builtin_names + num_of_builtins )
		throw DevaICE( "No such built-in function." );
    // compute the index of the function in the look-up table(s)
    long l = (long)i;
    l -= (long)&builtin_names;
    int idx = l / sizeof( string );
    if( idx > num_of_builtins )
		throw DevaICE( "Out-of-array-bounds looking for built-in function." );
    else
        // call the function
        builtin_fcns[idx]( ex );
}

// convert an object to a string value
string obj_to_str( const DevaObject* const o )
{
	ostringstream s;
	DevaObject obj = *o;
	s << obj;
	return s.str();
}

// the built-in executor functions:
void do_print( Executor *ex )
{
	if( Executor::args_on_stack < 1 || Executor::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'print'." );

	// get the argument off the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// if there are two arguments, concat them (the second is the "line end")
	// if there is only one, append a "\n"
	string eol_str;
	if( Executor::args_on_stack == 2 )
	{
		// second argument, if any, *must* be a string
		DevaObject eol = ex->stack.back();
		ex->stack.pop_back();
		if( eol.Type() != sym_string )
			throw DevaRuntimeException( "'eol' argument in built-in function 'print' must be a string." );
		eol_str = eol.str_val;
	}

	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'print'." );
	}
	if( !o )
		o = &obj;
	// convert to a string
	string s = obj_to_str( o );
	if( Executor::args_on_stack == 2 )
		s += eol_str;
	// print it
	cout << s;
	// default eol is a newline
	if( Executor::args_on_stack == 1 )
		cout << endl;

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_str( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'str'." );

	// get the argument off the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'str'." );
	}
	if( !o )
		o = &obj;

	string s = obj_to_str( o );

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", s ) );
}

void do_chr( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'chr'." );

	// get the argument off the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'chr'." );
	}
	if( !o )
		o = &obj;

	// ensure the argument is a number
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'num' argument to built-in function 'chr' must be a numercal value." );

	char c = (char)(o->num_val);
	char* s = new char[2];
	s[0] = c;
	s[1] = '\0';

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", s ) );
}

void do_append( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'append'." );

	// vector to append to
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// value to append
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'append'." );
	}
	if( !o )
		o = &obj;

	// vector
	if( o->Type() != sym_vector )
		throw DevaRuntimeException( "'destination' argument to built-in function 'append' must be a vector." );

	o->vec_val->push_back( val );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_length( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'length'." );

	// arg (vector, map or string) is at stack+0
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'length'." );
	}
	if( !o )
		o = &obj;

	int len;
	// string
	if( o->Type() == sym_string )
	{
		len = string( o->str_val ).size();
	}
	// vector
	else if( o->Type() == sym_vector )
	{
		len = o->vec_val->size();
	}
	// map, class, instance
	else if( o->Type() == sym_map || o->Type() == sym_class || o->Type() == sym_instance )
	{
		len = o->map_val->size();
	}

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", (double)len ) );
}

void do_copy( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'copy'." );

	// object to copy at top of stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'copy'." );
	}
	if( !o )
		o = &obj;

    DevaObject copy;
	if( o->Type() == sym_map )
	{
        // create a new map object that is a copy of the one we received,
        DOMap* m = new DOMap( *(o->map_val) );
        copy = DevaObject( "", m );
	}
	else if( o->Type() == sym_class )
	{
        DOMap* m = new DOMap( *(o->map_val) );
		copy = DevaObject::ClassFromMap( "", m );
	}
	else if( o->Type() == sym_instance )
	{
        DOMap* m = new DOMap( *(o->map_val) );
		copy = DevaObject::InstanceFromMap( "", m );
	}
    else if( o->Type() == sym_vector )
    {
        // create a new vector object that is a copy of the one we received,
        DOVector* v = new DOVector( *(o->vec_val) );
        copy = DevaObject( "", v );
    }
	else
	{
        throw DevaRuntimeException( "Object for built-in function 'copy' is not a map or vector." );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( copy );
}

void do_eval( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'eval'." );

	// string to eval must be at top of stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'eval'." );
	}
	if( !o )
		o = &obj;

	// had better be a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "eval() builtin function called with a non-string argument." );

	ex->RunText( o->str_val );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_delete( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'delete'." );

	// object to delete at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'delete'." );
	}
	if( !o )
		o = &obj;

	ex->remove_symbol( *o );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_open( Executor *ex )
{
	if( Executor::args_on_stack != 1 && Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'open'." );

	// filename to open is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	// if there are two arguments the second is the file mode
	// if there is only one, default mode is "r"
	DevaObject arg;
	DevaObject* mode;
	if( Executor::args_on_stack == 2 )
	{
		// second argument, if any, *must* be a string
		arg = ex->stack.back();
		ex->stack.pop_back();
		if( arg.Type() == sym_unknown )
		{
			mode = ex->find_symbol( arg );
			if( !mode )
				throw DevaRuntimeException( "Symbol not found for 'mode' argument in built-in function 'open'." );
		}
		else
			mode = &arg;
		if( mode->Type() != sym_string )
			throw DevaRuntimeException( "'mode' argument to built-in function 'open' must be a string." );
	}
	else
	{
		arg = DevaObject( "", string( "r" ) );
		mode = &arg;
	}

	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file_name' argument in built-in function 'open'." );
	}
	if( !o )
		o = &obj;

	// ensure filename is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'file_name' argument to built-in function 'open' must be a string." );

	FILE* file = fopen( o->str_val, mode->str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the file object as a 'native object' type
	if( file )
		ex->stack.push_back( DevaObject( "", (void*)file ) );
	// or null, on failure
	else
		ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_close( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'close'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'close'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'close' is not of the correct type." );

	fclose( (FILE*)(o->sz_val) );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_flush( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'flush'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'flush'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'flush' is not of the correct type." );

	fflush( (FILE*)(o->sz_val) );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_read( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'read'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the number of bytes to read
	DevaObject bytes = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'read'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'read' is not of the correct type." );

	// get the number of bytes
	size_t num_bytes = 0;
	if( bytes.Type() != sym_number )
	{
		DevaObject* no = ex->find_symbol( bytes );
		if( !no )
			throw DevaRuntimeException( "Symbol not found for 'num_bytes' argument in built-in function 'read'." );
		num_bytes = no->num_val;
	}
	else
		num_bytes = bytes.num_val;

	// allocate space for bytes plus a null-terminator
	unsigned char* s = new unsigned char[num_bytes + 1];
	// zero out the bytes
	memset( s, 0, num_bytes + 1 );
	size_t bytes_read = fread( (void*)s, 1, num_bytes, (FILE*)(o->sz_val) );

	// convert to a vector of numbers
	DOVector* vec = new DOVector();
	vec->reserve( bytes_read );
	for( int c = 0; c < bytes_read; ++c )
	{
		vec->push_back( DevaObject( "", (double)(s[c]) ) );
	}

	delete [] s;

	// pop the return address
	ex->stack.pop_back();

	// return the vector of read bytes
	ex->stack.push_back( DevaObject( "", vec ) );
}

// if there are embedded nulls in the bytes read the string
// returned will only contain up to the first null...
// read() should be used in this case, not readstring
void do_readstring( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'readstring'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the number of bytes to read
	DevaObject bytes = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'readstring'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'readstring' is not of the correct type." );

	// get the number of bytes
	size_t num_bytes = 0;
	if( bytes.Type() != sym_number )
	{
		DevaObject* no = ex->find_symbol( bytes );
		if( !no )
			throw DevaRuntimeException( "Symbol not found for 'num_bytes' argument in built-in function 'readstring'." );
		num_bytes = no->num_val;
	}
	else
		num_bytes = bytes.num_val;

	// allocate space for bytes plus a null-terminator
	char* s = new char[num_bytes + 1];
	// zero out the bytes
	memset( s, 0, num_bytes + 1 );
	size_t bytes_read = fread( (void*)s, 1, num_bytes, (FILE*)(o->sz_val) );
	// if we didn't read the full amount, we need to re-alloc and copy so that
	// we don't leak the extra bytes when they are assigned to a string DevaObject
	if( bytes_read != num_bytes )
	{
		char* new_s = new char[bytes_read + 1];
		new_s[bytes_read] = '\0';
		memcpy( (void*)new_s, (void*)s, bytes_read );
		delete [] s;
		s = new_s;
	}

	// if there are embedded nulls in the bytes read the string
	// returned will only contain up to the first null...
	// read() should be used in this case, not readstring

	// pop the return address
	ex->stack.pop_back();

	// return the read bytes as a string
	ex->stack.push_back( DevaObject( "", s ) );
}

void do_readline( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'readline'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'readline'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'readline' is not of the correct type." );

	// allocate space for some bytes 
	const size_t BUF_SZ = 10;
	char* buffer = new char[BUF_SZ];
	buffer[0] = '\0';
	// track the original start of the buffer
	size_t count = BUF_SZ;	// number of bytes read so far
	char* buf = buffer;
	while( fgets( buf, BUF_SZ, (FILE*)(o->sz_val) ) )
	{
		// read was valid, was the last char read a newline?
		// if so, we're done
		size_t len = strlen( buf );
		if( buf[len-1] == '\n' )
			break;
		// if not, 
		//  - allocate twice as much space
		char* new_buf = new char[count + BUF_SZ];
		//  - copy what was read, minus the null
		memcpy( (void*)new_buf, (void*)buffer, count - 1 );
		//  - free the orginal memory
		delete [] buffer;
		//  - continue
		buffer = new_buf;
		buf = buffer + count-1;
		count += BUF_SZ - 1;
	}
	// was there an error??
	if( ferror( (FILE*)(o->sz_val) ) )
		throw DevaRuntimeException( "Error accessing file in built-in method 'readline'." );

	// if we didn't fill the buffer, we need to re-alloc and copy so that
	// we don't leak the extra bytes when they are assigned to a string DevaObject
	size_t bytes_read = strlen( buffer );
	if( bytes_read != count )
	{
		char* new_buf = new char[bytes_read + 1];
		new_buf[bytes_read] = '\0';
		memcpy( (void*)new_buf, (void*)buffer, bytes_read );
		delete [] buffer;
		buffer = new_buf;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the read bytes as a string
	ex->stack.push_back( DevaObject( "", buffer ) );
}

void do_readlines( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'readlines'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'readlines'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'readlines' is not of the correct type." );

	// keep reading lines until we reach the EOF
	DOVector* vec = new DOVector;
	while( true )
	{
		// allocate space for some bytes 
		const size_t BUF_SZ = 10;
		char* buffer = new char[BUF_SZ];
		buffer[0] = '\0';
		// track the original start of the buffer
		size_t count = BUF_SZ;	// number of bytes read so far
		char* buf = buffer;
		while( fgets( buf, BUF_SZ, (FILE*)(o->sz_val) ) )
		{
			// read was valid, was the last char read a newline?
			// if so, we're done
			size_t len = strlen( buf );
			if( buf[len-1] == '\n' )
				break;
			// if not, 
			//  - allocate twice as much space
			char* new_buf = new char[count + BUF_SZ];
			//  - copy what was read, minus the null
			memcpy( (void*)new_buf, (void*)buffer, count - 1 );
			//  - free the orginal memory
			delete [] buffer;
			//  - continue
			buffer = new_buf;
			buf = buffer + count-1;
			count += BUF_SZ - 1;
		}
		// if we didn't fill the buffer, we need to re-alloc and copy so that
		// we don't leak the extra bytes when they are assigned to a string DevaObject
		size_t bytes_read = strlen( buffer );
		if( bytes_read != count )
		{
			char* new_buf = new char[bytes_read + 1];
			new_buf[bytes_read] = '\0';
			memcpy( (void*)new_buf, (void*)buffer, bytes_read );
			delete [] buffer;
			buffer = new_buf;
		}

		// add this line to our output vector
		vec->push_back( DevaObject( "", buffer ) );

		// done?
		if( feof( (FILE*)(o->sz_val) ) )
			break;
		// was there an error??
		if( ferror( (FILE*)(o->sz_val) ) )
			throw DevaRuntimeException( "Error accessing file in built-in method 'readlines'." );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the vector of lines
	ex->stack.push_back( DevaObject( "", vec ) );
}

void do_write( Executor *ex )
{
	if( Executor::args_on_stack != 3 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'write'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the number of bytes to write
	DevaObject bytes = ex->stack.back();
	ex->stack.pop_back();

	// next is the object to write from
	DevaObject src = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'write'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'write' is not of the correct type." );

	// get the number of bytes
	size_t num_bytes = 0;
	if( bytes.Type() != sym_number )
	{
		DevaObject* no = ex->find_symbol( bytes );
		if( !no )
			throw DevaRuntimeException( "Symbol not found for 'num_bytes' argument in built-in function 'write'." );
		num_bytes = no->num_val;
	}
	else
		num_bytes = bytes.num_val;

	// ensure the source is a vector
	DevaObject* source;
	if( src.Type() != sym_vector )
	{
		source = ex->find_symbol( src );
		if( !source )
			throw DevaRuntimeException( "'source' argument in built-in function 'write' must be a vector." );
	}
	else
		source = &src;

	size_t len = num_bytes > source->vec_val->size() ? num_bytes : source->vec_val->size();
	unsigned char* data = new unsigned char[len];
	// create a native array of unsigned chars to write out
	for( int c = 0; c < len; ++c )
	{
		// ensure this object is a number
		DevaObject o = source->vec_val->at( c );
		if( o.Type() != sym_number )
			throw DevaRuntimeException( "'source' vector in built-in function 'write' contains objects that are not numeric." );

		// copy the item's data
		data[c] = (unsigned char)o.num_val;
	}
	size_t bytes_written = fwrite( (void*)data, 1, len, (FILE*)(o->sz_val) );

	delete [] data;

	// pop the return address
	ex->stack.pop_back();

	// return the number of bytes written
	ex->stack.push_back( DevaObject( "", (double)bytes_written ) );
}

void do_writestring( Executor *ex )
{
	if( Executor::args_on_stack != 3 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'writestring'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the number of bytes to write
	DevaObject bytes = ex->stack.back();
	ex->stack.pop_back();

	// next is the object to write from
	DevaObject src = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'writestring'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'writestring' is not of the correct type." );

	// get the number of bytes
	size_t num_bytes = 0;
	if( bytes.Type() != sym_number )
	{
		DevaObject* no = ex->find_symbol( bytes );
		if( !no )
			throw DevaRuntimeException( "Symbol not found for 'num_bytes' argument in built-in function 'writestring'." );
		num_bytes = no->num_val;
	}
	else
		num_bytes = bytes.num_val;

	// ensure the source is a string
	DevaObject* source;
	if( src.Type() != sym_string )
	{
		source = ex->find_symbol( src );
		if( !source )
			throw DevaRuntimeException( "'source' argument in built-in function 'writestring' must be a string." );
	}
	else
		source = &src;

	size_t slen = strlen( source->str_val );
	size_t len = num_bytes > slen ? num_bytes : slen;
	size_t bytes_written = fwrite( (void*)(source->str_val), 1, len, (FILE*)(o->sz_val) );

	// pop the return address
	ex->stack.pop_back();

	// return the number of bytes written
	ex->stack.push_back( DevaObject( "", (double)bytes_written ) );
}

void do_writeline( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'writeline'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the object to write from
	DevaObject src = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'writeline'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'writeline' is not of the correct type." );

	// ensure the source is a string
	DevaObject* source;
	if( src.Type() != sym_string )
	{
		source = ex->find_symbol( src );
		if( !source )
			throw DevaRuntimeException( "Symbol not found for 'source' argument in built-in function 'writeline'." );
	}
	else
		source = &src;

	int ret = 1;
	if( fputs( source->str_val, (FILE*)(o->sz_val) ) < 0 )
		ret = 0;

	// pop the return address
	ex->stack.pop_back();

	// return, true on success, false on failure
	ex->stack.push_back( DevaObject( "", (bool)ret ) );
}

void do_writelines( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'writelines'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the object to write from
	DevaObject src = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'writelines'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'writelines' is not of the correct type." );

	// get the source vector
	DevaObject* source;
	if( src.Type() != sym_vector )
	{
		source = ex->find_symbol( src );
		if( !source )
			throw DevaRuntimeException( "Symbol not found for 'source' argument in built-in function 'writelines'." );
		// ensure it is a vector
		if( source->Type() != sym_vector )
			throw DevaRuntimeException( "'source' argument to built-in function 'writelines' is not of the correct type." );
	}
	else
		source = &src;

	int ret = 0;
	for( DOVector::iterator i = source->vec_val->begin(); i != source->vec_val->end(); ++i )
	{
		// if failed to write the line, break
		if( fputs( i->str_val, (FILE*)(o->sz_val) ) < 0 )
			break;
		++ret;
	}

	// pop the return address
	ex->stack.pop_back();

	// return the number of lines written
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_seek( Executor *ex )
{
	if( Executor::args_on_stack != 2 && Executor::args_on_stack != 3 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'seek'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// next is the position to seek to
	DevaObject position = ex->stack.back();
	ex->stack.pop_back();
	
	// next is the origin to seek from
	// (need to be defined in standard 'io' import)
	// SEEK_SET = 0
	// SEEK_CUR = 1
	// SEEK_END = 2
	// if there are only two args, default origin is SEEK_SET (0)
	DevaObject arg;
	int origin;
	if( Executor::args_on_stack == 3 )
	{
		// third argument, if any, *must* be a number
		arg = ex->stack.back();
		ex->stack.pop_back();
		DevaObject* parg;
		if( arg.Type() == sym_unknown )
		{
			parg = ex->find_symbol( arg );
			if( !parg )
				throw DevaRuntimeException( "Symbol not found for 'origin' argument in built-in function 'seek'." );
		}
		else
			parg = &arg;
		if( parg->Type() != sym_number )
			throw DevaRuntimeException( "'origin' argument to built-in function 'seek' must be a number." );
		// TODO: origin must be an integral number. error on non-integer
		origin = (int)parg->num_val;
	}
	else
	{
		origin = 0;
	}
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'seek'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'seek' is not of the correct type." );

	// get the position
	long int pos = 0;
	if( position.Type() != sym_number )
	{
		DevaObject* po = ex->find_symbol( position );
		if( !po )
			throw DevaRuntimeException( "Symbol not found for 'position' argument in built-in function 'seek'." );
		pos = (long int)po->num_val;
	}
	else
		pos = (long int)position.num_val;

	fseek( (FILE*)(o->sz_val), pos, origin );

	// pop the return address
	ex->stack.pop_back();

	// all functions must return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_tell( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'tell'." );

	// file object to close is at the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'tell'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a native object
	if( o->Type() != sym_native_obj )
		throw DevaRuntimeException( "'file' argument to built-in function 'tell' is not of the correct type." );

	long int pos = ftell( (FILE*)(o->sz_val) );

	// pop the return address
	ex->stack.pop_back();

	// return the position
	ex->stack.push_back( DevaObject( "", (double)pos ) );
}

void do_name( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'type'." );

	// get the argument off the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'type'." );
	}
	if( !o )
		o = &obj;

	string name;
	// if this is a class, get the name attribute
	if( o->Type() == sym_class )
	{
		DOMap::iterator it = o->map_val->find( DevaObject( "", string( "__name__" ) ) );
		if( it != o->map_val->end() )
		{
			if( it->second.Type() != sym_string )
				throw DevaICE( "__name__ attribute on a class object is not of type 'string'." );
			name = it->second.str_val;
		}
		else
			throw DevaICE( "__name__ attribute not found on a class object." );
	}
	else
		name = o->name;
	
	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", name ) );
}

void do_type( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'name'." );

	// get the argument off the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in call to built-in function 'name'." );
	}
	if( !o )
		o = &obj;

	string type;
	switch( o->Type() )
	{
	case sym_null:
		type = "null";
		break;
	case sym_boolean:
		type = "bool";
		break;
	case sym_number:
		type = "number";
		break;
	case sym_string:
		type = "string";
		break;
	case sym_vector:
		type = "vector";
		break;
	case sym_map:
		type = "map";
		break;
	case sym_address:
		type = "address";
		break;
	case sym_function_call:
		type = "call";
		break;
	case sym_unknown:
		type = "variable";
		break;
	case sym_class:
		type = "class";
		break;
	case sym_instance:
		type = "instance";
		break;
	case sym_size:
		type = "size";
		break;
	case sym_native_obj:
		type = "native object";
		break;
	}

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", type ) );
}

