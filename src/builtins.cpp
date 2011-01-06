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


namespace deva
{

// is this name a built-in function?
bool IsBuiltin( const string & name )
{
	const string* i = find( builtin_names, builtin_names + num_of_builtins, name );
	if( i != builtin_names + num_of_builtins ) return true;
		else return false;
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
	// TODO:
	// if this is an instance object, see if there is a 'repr' method
	// and use the string returned from it, if it exists
//	if( o->type == obj_string )
//	{
//	}
//	else
		s = obj_to_str( o );

	cout << s << separator;

	helper.ReturnVal( Object( obj_null ) );
}

void do_str( Frame *frame )
{
//	// if this is an instance object, see if there is a 'repr' method
//	// and use the string returned from it, if it exists
//	if( o->Type() == sym_instance )
//	{
//		// look for the '__class__' member, which holds the class name
//		DOMap::iterator it = o->map_val->find( DevaObject( "", string( "__class__" ) ) );
//		if( it == o->map_val->end() )
//			throw DevaRuntimeException( "Invalid instance, doesn't contain '__class__' member!" );
//		string cls( "@" );
//		cls += it->second.str_val;
//		string str( "str" );
//		str += cls;
//		// append "@class" to the method name
//		it = o->map_val->find( DevaObject( "", str ) );
//		if( it != o->map_val->end() )
//		{
//			if( it->second.Type() == sym_address )
//			{
//				// push the object ("self")
//				ex->stack.push_back( *o );
//				// call the function (takes no args)
//				ex->ExecuteDevaFunction( str, 1 );
//				// get the result (return value)
//				DevaObject retval = ex->stack.back();
//				ex->stack.pop_back();
//				if( retval.Type() != sym_string )
//					throw DevaRuntimeException( "The 'str' method on a class did not return a string value" );
//				s = retval.str_val;
//			}
//			else
//				s = obj_to_str( o );
//		}
//		else
//			s = obj_to_str( o );
//	}

	BuiltinHelper helper( NULL, "str", frame );
	helper.CheckNumberOfArguments( 1 );
	Object* o = helper.GetLocalN( 0 );

	string s;
	// TODO:
	// if this is an instance object, see if there is a 'repr' method
	// and use the string returned from it, if it exists
//	if( o->type == obj_instance )
//	{
//	}
//	else
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
	// TODO: if this is a class, get the name attribute
	if( o->type == obj_class )
	{
//		DOMap::iterator it = o->map_val->find( DevaObject( "", string( "__name__" ) ) );
//		if( it != o->map_val->end() )
//		{
//			if( it->second.Type() != sym_string )
//				throw DevaICE( "__name__ attribute on a class object is not of type 'string'." );
//			name = it->second.str_val;
//		}
//		else
//			throw DevaICE( "__name__ attribute not found on a class object." );
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

	// file object to read from is at the top of the stack
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

	// file object to read from is at the top of the stack
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

	// file object to write to is at the top of the stack
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

	size_t len = num_bytes < source->vec_val->size() ? num_bytes : source->vec_val->size();
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

	// file object to write to is at the top of the stack
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
	size_t len = num_bytes < slen ? num_bytes : slen;
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

	// file object to write to is at the top of the stack
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

	// file object to write to is at the top of the stack
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

	// file object to operate on is at the top of the stack
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

	// file object to operate on is at the top of the stack
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

void do_stdin( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Built-in function 'stdin' takes no arguments." );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", (void*)stdin ) );
}

void do_stdout( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Built-in function 'stdout' takes no arguments." );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", (void*)stdout ) );
}

void do_stderr( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Built-in function 'stderr' takes no arguments." );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", (void*)stderr ) );
}

void do_format( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'format'." );

	// get the format string off the stack first 
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'format_string' argument in call to built-in function 'format'." );
	}
	if( !o )
		o = &obj;

	// ensure it's a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "String expected for 'format_string' parameter in call to built-in function 'format'." );

	// get the format_args vector off the stack next
	DevaObject args = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* v = NULL;
	if( args.Type() == sym_unknown )
	{
		v = ex->find_symbol( args );
		if( !v )
			throw DevaRuntimeException( "Symbol not found for 'format_args' parameter in call to built-in function 'format'." );
	}
	if( !v )
		v = &args;

	// ensure it's a vector
	if( v->Type() != sym_vector )
		throw DevaRuntimeException( "Vector expected for 'format_args' parameter in call to built-in function 'format'." );

	string ret;

	// format the string using boost's format library
	boost::format formatter;
	try
	{
		formatter = boost::format( o->str_val );
		for( DOVector::iterator i = v->vec_val->begin(); i != v->vec_val->end(); ++i )
		{
			formatter % *i;
		}
	}
	catch( boost::io::bad_format_string & e )
	{
		throw DevaRuntimeException( "The format string passed to built-in function 'format' was invalid." );
	}
	catch( boost::io::too_many_args & e )
	{
		throw DevaRuntimeException( "The format string passed to built-in function 'format' referred to fewer parameters than were passed in the parameter vector." );
	}
	catch( boost::io::too_few_args & e )
	{
		throw DevaRuntimeException( "The format string passed to built-in function 'format' referred to more parameters than were passed in the parameter vector." );
	}
	ret = str( formatter );

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_join( Executor *ex )
{
	if( Executor::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'join'." );

	// get the vector of objects to join off the stack
	DevaObject args = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* v = NULL;
	if( args.Type() == sym_unknown )
	{
		v = ex->find_symbol( args );
		if( !v )
			throw DevaRuntimeException( "Symbol not found for 'args' parameter in call to built-in function 'join'." );
	}
	if( !v )
		v = &args;

	// ensure it's a vector
	if( v->Type() != sym_vector )
		throw DevaRuntimeException( "Vector expected for 'args' parameter in call to built-in function 'join'." );

	string sep;
	if( Executor::args_on_stack == 2 )
	{
		// optional 'separator' arg
		DevaObject val = ex->stack.back();
		ex->stack.pop_back();

		DevaObject* o;
		if( val.Type() == sym_unknown )
		{
			o = ex->find_symbol( val );
			if( !o )
				throw DevaRuntimeException( "Symbol not found for the 'sep' argument in vector built-in method 'join'." );
		}
		else
				o = &val;
		if( o->Type() != sym_string )
				throw DevaRuntimeException( "'sep' argument to vector built-in method 'join' must be a string." );

		sep = o->str_val;
	}
	else
		sep = "";

	string ret;
	for( DOVector::iterator i = v->vec_val->begin(); i != v->vec_val->end(); ++i )
	{
		ostringstream s;
		if( i != v->vec_val->begin() )
			s << sep;
		s << *i;
		ret += s.str();
	}

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_error( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'error'." );

	bool err = ex->Error();

	// pop the return address
	ex->stack.pop_back();

	// return error
	ex->stack.push_back( DevaObject( "", err ) );

}

void do_seterror( Executor *ex )
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

void do_geterror( Executor *ex )
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

void do_importmodule( Executor *ex )
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
