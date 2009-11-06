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
void do_print( Executor *ex, const Instruction & inst );
void do_str( Executor *ex, const Instruction & inst );
void do_append( Executor *ex, const Instruction & inst );
void do_length( Executor *ex, const Instruction & inst );
void do_copy( Executor *ex, const Instruction & inst );
void do_eval( Executor *ex, const Instruction & inst );
void do_delete( Executor *ex, const Instruction & inst );
void do_open( Executor *ex, const Instruction & inst );
void do_close( Executor *ex, const Instruction & inst );
void do_flush( Executor *ex, const Instruction & inst );
void do_read( Executor *ex, const Instruction & inst );
void do_readline( Executor *ex, const Instruction & inst );
void do_readlines( Executor *ex, const Instruction & inst );
void do_write( Executor *ex, const Instruction & inst );
void do_writeline( Executor *ex, const Instruction & inst );
void do_writelines( Executor *ex, const Instruction & inst );
void do_seek( Executor *ex, const Instruction & inst );
void do_tell( Executor *ex, const Instruction & inst );

// tables defining the built-in function names...
static const string builtin_names[] = 
{
	string( "print" ),
    string( "str" ),
    string( "append" ),
    string( "length" ),
    string( "copy" ),
    string( "eval" ),
    string( "delete" ),
    string( "open" ),
    string( "close" ),
    string( "flush" ),
    string( "read" ),
    string( "readline" ),
    string( "readlines" ),
    string( "write" ),
    string( "writeline" ),
    string( "writelines" ),
    string( "seek" ),
    string( "tell" ),
};
// ...and function pointers to the executor functions for them
typedef void (*builtin_fcn)(Executor*, const Instruction&);
builtin_fcn builtin_fcns[] = 
{
    do_print,
    do_str,
    do_append,
    do_length,
    do_copy,
	do_eval,
	do_delete,
	do_open,
	do_close,
	do_flush,
	do_read,
	do_readline,
	do_readlines,
	do_write,
	do_writeline,
	do_writelines,
	do_seek,
	do_tell,
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
        builtin_fcns[idx]( ex, inst );
}

// convert an object to a string value
string obj_to_str( const DevaObject* const o )
{
	ostringstream s;

	// evaluate it
	switch( o->Type() )
	{
		case sym_number:
			s << o->num_val;
			break;
		case sym_string:
			s << o->str_val;
			break;
		case sym_boolean:
			if( o->bool_val )
				s << "true";
			else
				s << "false";
			break;
		case sym_null:
			s << "null";
			break;
		case sym_map:
			{
			// dump map contents
			s << "map: '" << o->name << "' = " << endl;
			smart_ptr<DOMap> mp( o->map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); ++it )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				s << " - " << key << " : " << val << endl;
			}
			break;
			}
		case sym_vector:
			{
			// dump vector contents
			s << "vector: '" << o->name << "' = " << endl;
			smart_ptr<DOVector> vec( o->vec_val );
			for( DOVector::iterator it = vec->begin(); it != vec->end(); ++it )
			{
				DevaObject val = (*it);
				s << val << endl;
			}
			break;
			}
		case sym_offset:
			s << "function: '" << o->name << "'";
			break;
		case sym_function_call:
			s << "function_call: '" << o->name << "'";
			break;
		case sym_unknown:
			s << "unknown: '" << o->name << "'";
			break;
		case sym_class:
			{
			// dump map contents
			s << "class: '" << o->name << "' = " << endl;
			smart_ptr<DOMap> mp( o->map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); ++it )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				s << " - " << key << " : " << val << endl;
			}
			break;
			}
		case sym_instance:
			{
			// dump map contents
			s << "instance: '" << o->name << "' = " << endl;
			smart_ptr<DOMap> mp( o->map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); ++it )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				s << " - " << key << " : " << val << endl;
			}
			break;
			}
		default:
			s << "ERROR: unknown type";
			break;
	}
	return s.str();
}

// the built-in executor functions:
void do_print( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found in function call" );
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

void do_str( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &obj;

	string s = obj_to_str( o );

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", s ) );
}

void do_append( Executor *ex, const Instruction & inst )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to built-in function 'append'." );

	// vector or string to append to
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
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &obj;

	// string
	if( o->Type() == sym_string )
	{
		// TODO: value could be a variable, we need to look it up too!
		// value has to be a string too
		DevaObject* v;
		if( val.Type() == sym_unknown )
		{
			v = ex->find_symbol( val );
			if( !o )
				throw DevaRuntimeException( "Symbol not found for the value argument in built-in function 'append'." );
		}
		else
			v = &val;

		if( v->Type() != sym_string )
			throw DevaRuntimeException( "Cannot append a non-string to a string in append() built-in." );
		// concat the strings
		string ret( o->str_val );
		ret += v->str_val;
		*o = DevaObject( o->name, ret );
	}
	// vector
	else if( o->Type() == sym_vector )
	{
		o->vec_val->push_back( val );
	}
	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_length( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found in function call" );
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

void do_copy( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found in function call" );
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

void do_eval( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &obj;

	// had better be a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "eval() builtin function called with a non-string argument." );

	//ex->Output( inst );
	ex->RunText( o->str_val );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_delete( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &obj;

	ex->remove_symbol( *o );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_open( Executor *ex, const Instruction & inst )
{
	if( Executor::args_on_stack != 1 )
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
				throw DevaRuntimeException( "Symbol not found for 'mode' argument in built-in function 'open'" );
		}
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
			throw DevaRuntimeException( "Symbol not found for 'file_name' argument in built-in function 'open'" );
	}
	if( !o )
		o = &obj;

	// ensure filename is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'file_name' argument to built-in function 'open' must be a string." );

	FILE* file = fopen( o->str_val, mode->str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the file object as an 'offset'
	if( file )
		ex->stack.push_back( DevaObject( "", (size_t)file ) );
	// or null, on failure
	else
		ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_close( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'close'" );
	}
	if( !o )
		o = &obj;

	// ensure it's an 'offset'
	if( o->Type() != sym_offset )
		throw DevaRuntimeException( "'file' argument to built-in function 'close' is not of the correct type." );

	fclose( (FILE*)(o->func_offset) );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_flush( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'flush'" );
	}
	if( !o )
		o = &obj;

	// ensure it's an 'offset'
	if( o->Type() != sym_offset )
		throw DevaRuntimeException( "'file' argument to built-in function 'flush' is not of the correct type." );

	fflush( (FILE*)(o->func_offset) );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_read( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'read'" );
	}
	if( !o )
		o = &obj;

	// ensure it's an 'offset'
	if( o->Type() != sym_offset )
		throw DevaRuntimeException( "'file' argument to built-in function 'read' is not of the correct type." );

	// get the number of bytes
	size_t num_bytes = 0;
	if( bytes.Type() != sym_number )
	{
		DevaObject* no = ex->find_symbol( bytes );
		if( !no )
			throw DevaRuntimeException( "Symbol not found for 'num_bytes' argument in built-in function 'read'" );
		num_bytes = no->num_val;
	}
	else
		num_bytes = bytes.num_val;

	// allocate space for bytes plus a null-terminator
	char* s = new char[num_bytes + 1];
	// zero out the bytes
	memset( s, 0, num_bytes + 1 );
	size_t bytes_read = fread( (void*)s, 1, num_bytes, (FILE*)(o->func_offset) );
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
	// TODO: what to do about embedded nulls in the bytes read? the string
	// handling routines in deva will stop at the first embedded null...

	// pop the return address
	ex->stack.pop_back();

	// return the read bytes as a string
	ex->stack.push_back( DevaObject( "", s ) );
}

void do_readline( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'readline'" );
	}
	if( !o )
		o = &obj;

	// ensure it's an 'offset'
	if( o->Type() != sym_offset )
		throw DevaRuntimeException( "'file' argument to built-in function 'readline' is not of the correct type." );

	// allocate space for some bytes 
	const size_t BUF_SZ = 10;
	char* buffer = new char[BUF_SZ];
	buffer[0] = '\0';
	// track the original start of the buffer
	size_t count = BUF_SZ;	// number of bytes read so far
	char* buf = buffer;
	while( fgets( buf, BUF_SZ, (FILE*)(o->func_offset) ) )
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
	if( ferror( (FILE*)(o->func_offset) ) )
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

void do_readlines( Executor *ex, const Instruction & inst )
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
			throw DevaRuntimeException( "Symbol not found for 'file' argument in built-in function 'readline'" );
	}
	if( !o )
		o = &obj;

	// ensure it's an 'offset'
	if( o->Type() != sym_offset )
		throw DevaRuntimeException( "'file' argument to built-in function 'readline' is not of the correct type." );

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
		while( fgets( buf, BUF_SZ, (FILE*)(o->func_offset) ) )
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
		if( feof( (FILE*)(o->func_offset) ) )
			break;
		// was there an error??
		if( ferror( (FILE*)(o->func_offset) ) )
			throw DevaRuntimeException( "Error accessing file in built-in method 'readline'." );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the vector of lines
	ex->stack.push_back( DevaObject( "", vec ) );
}

void do_write( Executor *ex, const Instruction & inst )
{
}

void do_writeline( Executor *ex, const Instruction & inst )
{
}

void do_writelines( Executor *ex, const Instruction & inst )
{
}

void do_seek( Executor *ex, const Instruction & inst )
{
}

void do_tell( Executor *ex, const Instruction & inst )
{
}
