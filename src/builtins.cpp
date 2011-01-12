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

// builtins.cpp
// builtin functions for the deva language
// created by jcs, december 29, 2010 

// TODO:
// * 

#include "builtins.h"
#include "builtins_helpers.h"
#include <algorithm>
#include <sstream>
#include <cstdio>


namespace deva
{

// is this name a built-in function?
bool IsBuiltin( const string & name )
{
	const string* i = find( builtin_names, builtin_names + num_of_builtins, name );
	if( i != builtin_names + num_of_builtins ) return true;
		else return false;
}

NativeFunction GetBuiltin( const string & name )
{
	const string* i = find( builtin_names, builtin_names + num_of_builtins, name );
	if( i == builtin_names + num_of_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	else
	{
		// return the function object
		return builtin_fcns[idx];
	}
}

Object* GetBuiltinObjectRef( const string & name )
{
	const string* i = find( builtin_names, builtin_names + num_of_builtins, name );
	if( i == builtin_names + num_of_builtins )
	{
		return NULL;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_builtins )
	{
		return NULL;
	}
	else
	{
		// return the function object
		return &builtin_fcn_objs[idx];
	}
}


/////////////////////////////////////////////////////////////////////////////
// builtin fcn defs
/////////////////////////////////////////////////////////////////////////////

// helper to convert an object to a string value, for output
string obj_to_str( const Object* const o )
{
	ostringstream s;
	Object obj = *o;
	s << obj;
	return s.str();
}

void do_print( Frame* frame )
{
	BuiltinHelper helper( NULL, "print", frame );

	helper.CheckNumberOfArguments( 1, 2 );

	int args_passed = frame->NumArgsPassed();

	const char *separator;
	if( args_passed == 1 )
		separator = "\n";
	else if( args_passed == 2 )
	{
		Object* sep = helper.GetLocalN( 1 );
		helper.ExpectType( sep, obj_string );
		separator = sep->s;
	}
	Object* o = helper.GetLocalN( 0 );

	string s;
	// if this is an instance object, see if there is a 'repr' method
	// and use the string returned from it, if it exists
	if( o->type == obj_instance )
	{
		Map::iterator it = o->m->find( Object( obj_symbol_name, "repr" ) );
		if( it != o->m->end() )
		{
			if( it->second.type == obj_function )
			{
				// push the object ("self")
				ex->PushStack( *o );
				// call the function (takes no args)
				ex->ExecuteFunction( it->second.f, 0 );
				// get the result (return value)
				Object retval = ex->PopStack();
				if( retval.type != obj_string )
					throw RuntimeException( "The 'repr' method on a class did not return a string value." );
				s = retval.s;
			}
		}
	}
	else
		s = obj_to_str( o );

	cout << s << separator;

	helper.ReturnVal( Object( obj_null ) );
}

void do_str( Frame *frame )
{
	BuiltinHelper helper( NULL, "str", frame );
	helper.CheckNumberOfArguments( 1 );
	Object* o = helper.GetLocalN( 0 );

	string s;
	// if this is an instance object, see if there is a 'str' method
	// and use the string returned from it, if it exists
	if( o->type == obj_instance )
	{
		Map::iterator it = o->m->find( Object( obj_symbol_name, "str" ) );
		if( it != o->m->end() )
		{
			if( it->second.type == obj_function )
			{
				// push the object ("self")
				ex->PushStack( *o );
				// call the function (takes no args)
				ex->ExecuteFunction( it->second.f, 0 );
				// get the result (return value)
				Object retval = ex->PopStack();
				if( retval.type != obj_string )
					throw RuntimeException( "The 'str' method on a class did not return a string value." );
				s = retval.s;
			}
		}
	}
	else
		s = obj_to_str( o );

	const char* str = frame->GetParent()->AddString( s );
	helper.ReturnVal( Object( str ) );
}

void do_chr( Frame *frame )
{
	BuiltinHelper helper( NULL, "chr", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	char c = (char)(o->d);
	char* s = new char[2];
	s[0] = c;
	s[1] = '\0';
	frame->GetParent()->AddString( s );

	helper.ReturnVal( Object( s ) );
}


void do_append( Frame *frame )
{
	BuiltinHelper helper( NULL, "append", frame );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 1 );
	Object* v = helper.GetLocalN( 0 );
	helper.ExpectType( v, obj_vector );

	v->v->push_back( *o );

	helper.ReturnVal( Object( obj_null ) );
}

void do_length( Frame *frame )
{
	BuiltinHelper helper( NULL, "length", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectTypes( o, obj_string, obj_vector, obj_map, obj_class, obj_instance );

	int len;
	// string
	if( o->type == obj_string )
	{
		len = string( o->s ).size();
	}
	// vector
	else if( o->type == obj_vector )
	{
		len = o->v->size();
	}
	// map, class, instance
	else if( o->type == obj_map || o->type == obj_class || o->type == obj_instance )
	{
		len = o->m->size();
	}

	helper.ReturnVal( Object( (double)len ) );
}

void do_copy( Frame *frame )
{
	BuiltinHelper helper( NULL, "copy", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectRefType( o );

	Object copy;

	if( o->type == obj_map )
	{
		// create a new map object that is a copy of the one we received,
		Map* m = CreateMap( *(o->m) );
		copy = Object( m );
	}
	else if( o->type == obj_class )
	{
		Map* m = CreateMap( *(o->m) );
		copy = Object::CreateClass( m );
	}
	else if( o->type == obj_instance )
	{
		Map* m = CreateMap( *(o->m) );
		copy = Object::CreateInstance( m );
	}
	else if( o->type == obj_vector )
	{
		// create a new vector object that is a copy of the one we received,
		Vector* v = CreateVector( *(o->v) );
		copy = Object( v );
	}

	helper.ReturnVal( copy );
}

void do_name( Frame *frame )
{
	BuiltinHelper helper( NULL, "name", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );

	const char* name;
	// if this is a class, get the __name__ attribute
	if( o->type == obj_class )
	{
		Map::iterator it = o->m->find( Object( obj_symbol_name, "__name__" ) );
		if( it != o->m->end() )
		{
			if( it->second.type != obj_string )
				throw RuntimeException( "The '__name__' attribute on a class object did not return a string value." );
			name = it->second.s;
		}
	}
	else
	{
		// locate the name of this object
		const char* str = frame->GetParent()->FindSymbolName( o );
		name = frame->GetParent()->AddString( str );
	}

	helper.ReturnVal( Object( name ) );
}

void do_type( Frame *frame )
{
	BuiltinHelper helper( NULL, "type", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );

	const char* s = object_type_names[o->type];

	helper.ReturnVal( Object( s ) );
}

void do_exit( Frame *frame )
{
	BuiltinHelper helper( NULL, "exit", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectIntegralNumber( o );

	ex->Exit( *o );

	helper.ReturnVal( Object( obj_null ) );
}

void do_num( Frame *frame )
{
	BuiltinHelper helper( NULL, "num", frame );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectNonRefType( o );

	double d = 0.0;
	switch( o->type )
	{
	case obj_number:
		d = o->d;
		break;
	case obj_string:
	case obj_symbol_name:
		d = atof( o->s );
		break;
	case obj_boolean:
		if( o->b )
			d = 1.0;
		break;
	case obj_size:
		d = o->sz;
		break;
	case obj_native_obj:
		d = (size_t)o->no;
		break;
	case obj_native_function:
		d = (size_t)o->nf.p;
		break;
	case obj_function:
		d = (size_t)o->f->addr;
		break;
	case obj_null:
		break;
	default:
		break;
	}

	helper.ReturnVal( Object( d ) );
}

void do_range( Frame *frame )
{
	BuiltinHelper helper( NULL, "range", frame );
	helper.CheckNumberOfArguments( 2, 3 );

	int step = 1;
	Object *stepobj, *startobj, *endobj;

	int num_args = frame->NumArgsPassed();
	if( num_args == 3 )
	{
		startobj = helper.GetLocalN( 0 );
		endobj = helper.GetLocalN( 1 );
		stepobj = helper.GetLocalN( 2 );

		helper.ExpectIntegralNumber( stepobj );
		step = (int)stepobj->d;
	}
	else
	{
		startobj = helper.GetLocalN( 0 );
		endobj = helper.GetLocalN( 1 );
	}
	helper.ExpectIntegralNumber( startobj );
	helper.ExpectIntegralNumber( endobj );
	int start = startobj->d;
	int end = endobj->d;

	if( start < 0 || end < 0 || step < 0 )
		throw RuntimeException( "Arguments to 'range' must be positive integral numbers." );

	// generate the range
	// convert to a vector of numbers
	Vector* vec = CreateVector();
	vec->reserve( (int)((end - start) / step) );
	for( double c = start; c < end; c += step )
	{
		vec->push_back( Object( c ) );
	}

	helper.ReturnVal( Object( vec ) );
}

/*
void do_eval( Frame *frame )
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
*/

void do_open( Frame *frame )
{
	BuiltinHelper helper( NULL, "open", frame );
	helper.CheckNumberOfArguments( 1, 2 );

	Object *fileobj, *modeobj;
	const char* mode = "r";

	int num_args = frame->NumArgsPassed();
	if( num_args == 2 )
	{
		fileobj = helper.GetLocalN( 0 );
		helper.ExpectType( fileobj, obj_string );
		modeobj = helper.GetLocalN( 1 );
		helper.ExpectType( modeobj, obj_string );
		mode = modeobj->s;
	}
	else
	{
		fileobj = helper.GetLocalN( 0 );
		helper.ExpectType( fileobj, obj_string );
	}

	FILE* file = fopen( fileobj->s, mode );

	// return the file object as a 'native object' type
	if( file )
		helper.ReturnVal( Object( (void*)file ) );
	// or null, on failure
	else
		helper.ReturnVal( Object( obj_null ) );
}

void do_close( Frame *frame )
{
	BuiltinHelper helper( NULL, "close", frame );

	helper.CheckNumberOfArguments( 1 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );

	fclose( (FILE*)(file->no) );

	helper.ReturnVal( Object( obj_null ) );
}

void do_flush( Frame *frame )
{
	BuiltinHelper helper( NULL, "flush", frame );

	helper.CheckNumberOfArguments( 1 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );

	fflush( (FILE*)(file->no) );

	helper.ReturnVal( Object( obj_null ) );
}

void do_read( Frame *frame )
{
	BuiltinHelper helper( NULL, "read", frame );

	helper.CheckNumberOfArguments( 2 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* num_bytes_obj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( num_bytes_obj );
	
	int num_bytes = (int)num_bytes_obj->d;

	// allocate space for bytes plus a null-terminator
	unsigned char* s = new unsigned char[num_bytes + 1];
	// zero out the bytes
	memset( s, 0, num_bytes + 1 );
	size_t bytes_read = fread( (void*)s, 1, num_bytes, (FILE*)(file->no) );

	// convert to a vector of numbers
	Vector* vec = CreateVector();
	vec->reserve( bytes_read );
	for( int c = 0; c < bytes_read; ++c )
	{
		vec->push_back( Object( (double)(s[c]) ) );
	}

	delete [] s;

	helper.ReturnVal( Object( vec ) );
}

// if there are embedded nulls in the bytes read the string
// returned will only contain up to the first null...
// read() should be used in this case, not readstring
void do_readstring( Frame *frame )
{
	BuiltinHelper helper( NULL, "readstring", frame );

	helper.CheckNumberOfArguments( 2 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* num_bytes_obj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( num_bytes_obj );
	
	int num_bytes = (int)num_bytes_obj->d;

	// allocate space for bytes plus a null-terminator
	char* s = new char[num_bytes + 1];
	// zero out the bytes
	memset( s, 0, num_bytes + 1 );
	size_t bytes_read = fread( (void*)s, 1, num_bytes, (FILE*)(file->no) );
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

	helper.ReturnVal( Object( s ) );
}

void do_readline( Frame *frame )
{
	BuiltinHelper helper( NULL, "readline", frame );

	helper.CheckNumberOfArguments( 1 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	
	// allocate space for some bytes 
	const size_t BUF_SZ = 10;
	char* buffer = new char[BUF_SZ];
	buffer[0] = '\0';
	// track the original start of the buffer
	size_t count = BUF_SZ;	// number of bytes read so far
	char* buf = buffer;
	while( fgets( buf, BUF_SZ, (FILE*)(file->no) ) )
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
	if( ferror( (FILE*)(file->no) ) )
		throw RuntimeException( "Error accessing file in built-in method 'readline'." );

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

	helper.ReturnVal( Object( buffer ) );
}

void do_readlines( Frame *frame )
{
	BuiltinHelper helper( NULL, "readlines", frame );

	helper.CheckNumberOfArguments( 1 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	
	// keep reading lines until we reach the EOF
	Vector* vec = CreateVector();
	while( true )
	{
		// allocate space for some bytes 
		const size_t BUF_SZ = 10;
		char* buffer = new char[BUF_SZ];
		buffer[0] = '\0';
		// track the original start of the buffer
		size_t count = BUF_SZ;	// number of bytes read so far
		char* buf = buffer;
		while( fgets( buf, BUF_SZ, (FILE*)(file->no) ) )
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
		vec->push_back( Object( buffer ) );

		// done?
		if( feof( (FILE*)(file->no) ) )
			break;
		// was there an error??
		if( ferror( (FILE*)(file->no) ) )
			throw RuntimeException( "Error accessing file in built-in method 'readlines'." );
	}

	helper.ReturnVal( Object( vec ) );
}

void do_write( Frame *frame )
{
	BuiltinHelper helper( NULL, "write", frame );

	helper.CheckNumberOfArguments( 3 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* num_bytes_obj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( num_bytes_obj );
	Object* source = helper.GetLocalN( 2 );
	helper.ExpectType( source, obj_vector );
	
	int num_bytes = (int)num_bytes_obj->d;

	size_t len = num_bytes < source->v->size() ? num_bytes : source->v->size();
	unsigned char* data = new unsigned char[len];
	// create a native array of unsigned chars to write out
	for( int c = 0; c < len; ++c )
	{
		// ensure this object is a number
		Object o = source->v->operator[]( c );
		if( o.type != obj_number )
			throw RuntimeException( "'source' vector in built-in function 'write' contains objects that are not numeric." );

		// copy the item's data
		data[c] = (unsigned char)o.d;
	}
	size_t bytes_written = fwrite( (void*)data, 1, len, (FILE*)(file->no) );

	delete [] data;

	helper.ReturnVal( Object( (double)bytes_written ) );
}

void do_writestring( Frame *frame )
{
	BuiltinHelper helper( NULL, "writestring", frame );

	helper.CheckNumberOfArguments( 3 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* num_bytes_obj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( num_bytes_obj );
	Object* source = helper.GetLocalN( 2 );
	helper.ExpectType( source, obj_vector );
	
	int num_bytes = (int)num_bytes_obj->d;

	size_t slen = strlen( source->s );
	size_t len = num_bytes < slen ? num_bytes : slen;
	size_t bytes_written = fwrite( (void*)(source->s), 1, len, (FILE*)(file->no) );

	helper.ReturnVal( Object( (double)bytes_written ) );
}

void do_writeline( Frame *frame )
{
	BuiltinHelper helper( NULL, "writeline", frame );

	helper.CheckNumberOfArguments( 2 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* source = helper.GetLocalN( 1 );
	helper.ExpectType( source, obj_string );
	
	int ret = 1;
	if( fputs( source->s, (FILE*)(file->no) ) < 0 )
		ret = 0;

	helper.ReturnVal( Object( (bool)ret ) );
}

void do_writelines( Frame *frame )
{
	BuiltinHelper helper( NULL, "writelines", frame );

	helper.CheckNumberOfArguments( 2 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* source = helper.GetLocalN( 1 );
	helper.ExpectType( source, obj_string );
	
	int ret = 0;
	for( Vector::iterator i = source->v->begin(); i != source->v->end(); ++i )
	{
		if( i->type != obj_string )
			throw RuntimeException( "Non-string found in 'lines' argument to builtin 'writelines' function: a vector of strings is required." );
		// if failed to write the line, break
		if( fputs( i->s, (FILE*)(file->no) ) < 0 )
			break;
		ret++;
	}

	helper.ReturnVal( Object( (double)ret ) );
}

void do_seek( Frame *frame )
{
	BuiltinHelper helper( NULL, "seek", frame );

	helper.CheckNumberOfArguments( 2, 3 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	Object* posobj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( posobj );
	int origin = 0;
	if( frame->NumArgsPassed() == 3 )
	{
		Object* originobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( originobj );
		origin = (int)originobj->d;
	}
	
	// get the position
	fseek( (FILE*)(file->no), (dword)posobj->d, origin );

	helper.ReturnVal( Object( obj_null ) );
}

void do_tell( Frame *frame )
{
	BuiltinHelper helper( NULL, "tell", frame );

	helper.CheckNumberOfArguments( 1 );

	Object* file = helper.GetLocalN( 0 );
	helper.ExpectType( file, obj_native_obj );
	
	// get the position
	long int pos = ftell( (FILE*)(file->no) );

	helper.ReturnVal( Object( (double)pos ) );
}

void do_stdin( Frame *frame )
{
	BuiltinHelper helper( NULL, "stdin", frame );

	helper.CheckNumberOfArguments( 0 );

	helper.ReturnVal( Object( (void*)stdin ) );
}

void do_stdout( Frame *frame )
{
	BuiltinHelper helper( NULL, "stdout", frame );

	helper.CheckNumberOfArguments( 0 );

	helper.ReturnVal( Object( (void*)stdout ) );
}

void do_stderr( Frame *frame )
{
	BuiltinHelper helper( NULL, "stderr", frame );

	helper.CheckNumberOfArguments( 0 );

	helper.ReturnVal( Object( (void*)stderr ) );
}

void do_format( Frame *frame )
{
	BuiltinHelper helper( NULL, "format", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* formatstr = helper.GetLocalN( 0 );
	helper.ExpectType( formatstr, obj_string );
	Object* formatargs = helper.GetLocalN( 0 );
	helper.ExpectType( formatargs, obj_vector );

	// format the string using boost's format library
	string ret;
	boost::format formatter;
	try
	{
		formatter = boost::format( formatstr->s );
		for( Vector::iterator i = formatargs->v->begin(); i != formatargs->v->end(); ++i )
		{
			formatter % *i;
		}
	}
	catch( boost::io::bad_format_string & e )
	{
		throw RuntimeException( "The format string passed to built-in function 'format' was invalid." );
	}
	catch( boost::io::too_many_args & e )
	{
		throw RuntimeException( "The format string passed to built-in function 'format' referred to fewer parameters than were passed in the parameter vector." );
	}
	catch( boost::io::too_few_args & e )
	{
		throw RuntimeException( "The format string passed to built-in function 'format' referred to more parameters than were passed in the parameter vector." );
	}
	ret = str( formatter );

	// return a string allocated in the parent (calling) scope
	const char* str = frame->GetParent()->AddString( ret );
	helper.ReturnVal( Object( str ) );
}

void do_join( Frame *frame )
{
	BuiltinHelper helper( NULL, "join", frame );

	helper.CheckNumberOfArguments( 1, 2 );
	Object* args = helper.GetLocalN( 0 );
	helper.ExpectType( args, obj_vector );
	const char* sep = "";
	if( frame->NumArgsPassed() == 2 )
	{
		Object *o = helper.GetLocalN( 1 );
		helper.ExpectType( o, obj_string );
		sep = o->s;
	}

	string ret;
	for( Vector::iterator i = args->v->begin(); i != args->v->end(); ++i )
	{
		ostringstream s;
		if( i != args->v->begin() )
			s << sep;
		s << *i;
		ret += s.str();
	}

	// return a string allocated in the parent (calling) scope
	const char* str = frame->GetParent()->AddString( ret );
	helper.ReturnVal( Object( str ) );
}

/*
void do_error( Frame *frame )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'error'." );

	bool err = ex->Error();

	// pop the return address
	ex->stack.pop_back();

	// return error
	ex->stack.push_back( DevaObject( "", err ) );

}

void do_seterror( Frame *frame )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'seterror'." );

	// get error data arg from stack
	DevaObject o = get_arg( ex, "seterror", "error_object" );

	// set the global error flag
	ex->SetError( true );
	
	// set the global error object
	ex->SetErrorData( o );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_geterror( Frame *frame )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'error'." );

	DevaObject* o = ex->GetErrorData();

	// pop the return address
	ex->stack.pop_back();

	// push the error object onto the stack
	if( !o )
		ex->stack.push_back( DevaObject( "", sym_null ) );
	else
		ex->stack.push_back( DevaObject( "", *o ) );
}

void do_importmodule( Frame *frame )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'importmodule'." );

	// get error data arg from stack
	DevaObject o = get_arg_of_type( ex, "importmodule", "module_name", sym_string );

	// import the module
	bool success = ex->Import( o.str_val );

	// pop the return address
	ex->stack.pop_back();

	// return value
	ex->stack.push_back( DevaObject( "", success ) );
}
*/

} // end namespace deva
