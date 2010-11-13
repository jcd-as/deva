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

// vector_builtins.cpp
// built-in methods on vector objects, for the deva language virtual machine
// created by jcs, october 21, 2009 

// TODO:
// * 
//

#include "vector_builtins.h"
#include "builtin_helpers.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

// to add new builtins you must:
// 1) add a new fcn to the vector_builtin_names and vector_builtin_fcns arrays below
// 2) implement the function in this file

// pre-decls for builtin executors
void do_vector_append( Executor *ex );
void do_vector_length( Executor *ex );
void do_vector_copy( Executor *ex );
void do_vector_concat( Executor *ex );
void do_vector_min( Executor *ex );
void do_vector_max( Executor *ex );
void do_vector_pop( Executor *ex );
void do_vector_insert( Executor *ex );
void do_vector_remove( Executor *ex );
void do_vector_find( Executor *ex );
void do_vector_rfind( Executor *ex );
void do_vector_count( Executor *ex );
void do_vector_reverse( Executor *ex );
void do_vector_sort( Executor *ex );
void do_vector_map( Executor *ex );
void do_vector_filter( Executor *ex );
void do_vector_reduce( Executor *ex );
void do_vector_slice( Executor *ex );
void do_vector_join( Executor *ex );
// 'enumerable interface'
void do_vector_rewind( Executor *ex );
void do_vector_next( Executor *ex );

// tables defining the built-in function names...
static const string vector_builtin_names[] = 
{
	string( "vector_append" ),
	string( "vector_length" ),
	string( "vector_copy" ),
	string( "vector_concat" ),
	string( "vector_min" ),
	string( "vector_max" ),
	string( "vector_pop" ),
	string( "vector_insert" ),
	string( "vector_remove" ),
	string( "vector_find" ),
	string( "vector_rfind" ),
	string( "vector_count" ),
	string( "vector_reverse" ),
	string( "vector_sort" ),
	string( "vector_map" ),
	string( "vector_filter" ),
	string( "vector_reduce" ),
	string( "vector_slice" ),
    string( "vector_join" ),
	string( "vector_rewind" ),
	string( "vector_next" ),
};
// ...and function pointers to the executor functions for them
typedef void (*vector_builtin_fcn)(Executor*);
vector_builtin_fcn vector_builtin_fcns[] = 
{
	do_vector_append,
	do_vector_length,
	do_vector_copy,
	do_vector_concat,
	do_vector_min,
	do_vector_max,
	do_vector_pop,
	do_vector_insert,
	do_vector_remove,
	do_vector_find,
	do_vector_rfind,
	do_vector_count,
	do_vector_reverse,
	do_vector_sort,
	do_vector_map,
	do_vector_filter,
	do_vector_reduce,
	do_vector_slice,
    do_vector_join,
	do_vector_rewind,
	do_vector_next,
};
const int num_of_vector_builtins = sizeof( vector_builtin_names ) / sizeof( vector_builtin_names[0] );

// is this name a built-in function?
bool is_vector_builtin( const string & name )
{
	const string* i = find( vector_builtin_names, vector_builtin_names + num_of_vector_builtins, name );
	if( i != vector_builtin_names + num_of_vector_builtins ) return true;
	else return false;
}

// execute built-in function
void execute_vector_builtin( Executor *ex, const string & name )
{
	const string* i = find( vector_builtin_names, vector_builtin_names + num_of_vector_builtins, name );
	if( i == vector_builtin_names + num_of_vector_builtins )
		throw DevaICE( "No such vector built-in method." );
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&vector_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_vector_builtins )
		throw DevaICE( "Out-of-array-bounds looking for vector built-in method." );
	else
		// call the function
		vector_builtin_fcns[idx]( ex );
}

// the built-in executor functions:
void do_vector_append( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'append' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'append'." );

	// ref-counted types can be added as variables, simple types need to be
	// added as values (copies)
	DevaObject* in;
	if( val.Type() == sym_unknown )
	{
		in = ex->find_symbol( val );
		if( !in )
			throw DevaRuntimeException( "Unknown symbol passed to vector built-in method 'append'." );
	}
	else
		in = &val;

	vec.vec_val->push_back( *in );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_length( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'length' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	int len;
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'length'." );

	len = vec.vec_val->size();

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", (double)len ) );
}

void do_vector_copy( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'copy' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'copy'." );

	DevaObject copy;
	// create a new vector object that is a copy of the one we received,
	DOVector* v = new DOVector( *(vec.vec_val) );
	copy = DevaObject( "", v );

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( copy );
}

void do_vector_concat( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'concat' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'concat'." );

	DevaObject* in;
	if( val.Type() == sym_unknown )
	{
		in = ex->find_symbol( val );
		if( !in )
			throw DevaRuntimeException( "Unknown symbol passed to vector built-in method 'concat'." );
	}
	if( in->Type() != sym_vector )
		throw DevaRuntimeException( "Vector expected as argument to vector built-in method 'concat'." );

	// nothing in <algorithm> to help us with this...
	// make sure there's enough reserve size
	if( vec.vec_val->capacity() < vec.vec_val->size() + in->vec_val->size() )
		vec.vec_val->reserve( vec.vec_val->size() + in->vec_val->size() );
	// append each element
	for( DOVector::iterator i = in->vec_val->begin(); i != in->vec_val->end(); ++i )
	{
		vec.vec_val->push_back( *i );
	}

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

struct MinComparator
{
	static SymbolType type;
	bool operator()( const DevaObject & lhs, const DevaObject & rhs )
	{
		if( lhs.Type() != type || rhs.Type() != type )
			throw DevaRuntimeException( "Vector built-in method 'min' called on a vector with non-homogenous contents." );
		return lhs < rhs;
	}
};
SymbolType MinComparator::type = sym_unknown;

void do_vector_min( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'min' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'min'." );

	if( vec.vec_val->size() == 0 )
		throw DevaRuntimeException( "Vector built-in method 'min' called on an empty vector." );

	// find the min element
	MinComparator::type = vec.vec_val->operator[]( 0 ).Type();
	DOVector::iterator it = min_element( vec.vec_val->begin(), vec.vec_val->end(), MinComparator() );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( *it );
}

void do_vector_max( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'max' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'max'." );

	if( vec.vec_val->size() == 0 )
		throw DevaRuntimeException( "Vector built-in method 'max' called on an empty vector." );

	// find the min element
	MinComparator::type = vec.vec_val->operator[]( 0 ).Type();
	DOVector::iterator it = max_element( vec.vec_val->begin(), vec.vec_val->end(), MinComparator() );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( *it );
}

void do_vector_pop( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'pop' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'pop'." );

	DevaObject o = vec.vec_val->back();
	vec.vec_val->pop_back();

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( o );
}

void do_vector_insert( Executor *ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'insert' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// position to insert at is next on stack
	DevaObject pos = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'insert'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the value argument in vector built-in method 'insert'." );
	}
	else
		o = &val;

	// position
	if( pos.Type() != sym_number )
		throw DevaRuntimeException( "Number expected in for position argument in vector built-in method 'insert'." );

	// TODO: position has to be integer. throw error on non-integral number?
	int i = (int)pos.num_val;
	if( i > vec.vec_val->size() )
		throw DevaRuntimeException( "Position argument greater than vector size in vector built-in method 'insert'." );

	// insert the value
	vec.vec_val->insert( vec.vec_val->begin() + i, *o );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_remove( Executor *ex )
{
	if( Executor::args_on_stack != 2 && Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'remove' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// start position to remove at is next on stack
	DevaObject start = ex->stack.back();
	ex->stack.pop_back();

	// default value for end is '-1'
	int i_end = -1;
	bool default_arg = true;
	if( Executor::args_on_stack == 2 )
	{
		default_arg = false;
		// end position to remove at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();

		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in vector built-in method 'remove'." );
		// TODO: end needs to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// start position
	if( start.Type() != sym_number )
		throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'remove'." );

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'remove'." );

	// TODO: start needs to be integral values. error if they aren't
	int i_start = (int)start.num_val;
	if( default_arg )
		i_end = i_start;
	if( i_end == -1 )
		i_end = vec.vec_val->size();

	size_t sz = vec.vec_val->size();

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'remove'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'remove'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'remove': start is greater than end." );

	// remove the value
	if( i_start == i_end )
		vec.vec_val->erase( vec.vec_val->begin() + i_start );
	else
		vec.vec_val->erase( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_find( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'find' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// value is first on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'find'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in vector built-in method 'find'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'find'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the value argument in vector built-in method 'find'." );
	}
	else
		o = &val;

	if( i_end == -1 )
		i_end = vec.vec_val->size();

	size_t sz = vec.vec_val->size();

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'find'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'find'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'find': start is greater than end." );

	// find the element that matches
	// find/find_xxx from <algorithm> won't help us, we need an index, not an
	// iterator
	DevaObject ret;
	bool found = false;
	for( int i = i_start; i < i_end; ++i )
	{
		if( vec.vec_val->operator[]( i ) == *o )
		{
			ret = DevaObject( "", (double)i );
			found = true;
			break;
		}
	}

	if( !found )
		ret = DevaObject( "", sym_null );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_vector_rfind( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'rfind' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'rfind'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in vector built-in method 'rfind'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'rfind'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the value argument in vector built-in method 'rfind'." );
	}
	else
		o = &val;

	if( i_end == -1 )
		i_end = vec.vec_val->size();

	size_t sz = vec.vec_val->size();

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'rfind'." );
	if( i_end > sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'rfind'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'rfind': start is greater than end." );

	// find the element that matches
	// find/find_xxx from <algorithm> won't help us, we need an index, not an
	// iterator
	DevaObject ret;
	bool found = false;
	for( int i = i_end-1; i >= i_start; --i )
	{
		if( vec.vec_val->operator[]( i ) == *o )
		{
			ret = DevaObject( "", (double)i );
			found = true;
			break;
		}
	}
	if( !found )
		ret = DevaObject( "", sym_null );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_vector_count( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'count' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'count'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in vector built-in method 'count'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'count'." );

	// value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Unknown symbol passed to vector built-in method 'count'." );
	}

	if( i_end == -1 )
		i_end = vec.vec_val->size();

	size_t sz = vec.vec_val->size();

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'count'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'count'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'count': start is greater than end." );

	// count the value
	int num = count( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end, val );

	// pop the return address
	ex->stack.pop_back();

	// return the count
	ex->stack.push_back( DevaObject( "", (double)num ) );
}

void do_vector_reverse( Executor *ex )
{
	if( Executor::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'reverse' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 0 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in vector built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'reverse'." );

	if( i_end == -1 )
		i_end = vec.vec_val->size();

	size_t sz = vec.vec_val->size();

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'reverse'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'reverse'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'reverse': start is greater than end." );

	reverse( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return null
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_sort( Executor *ex )
{
	if( Executor::args_on_stack > 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'sort' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	int i_start = 0;
	int i_end = -1;
	if( Executor::args_on_stack > 1 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_start = (int)start.num_val;
	}
	if( Executor::args_on_stack > 0 )
	{
		// end position to insert at is next on stack
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		// end position
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for end position argument in vector built-in method 'reverse'." );
		// TODO: start and end need to be integral values. error if they aren't
		i_end = (int)end.num_val;
	}

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'sort'." );

	if( i_end == -1 )
		i_end = vec.vec_val->size();

	size_t sz = vec.vec_val->size();

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'sort'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'sort'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'sort': start is greater than end." );

	sort( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return null
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_map( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'map' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// function value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'map'." );

	// function value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'function' argument in vector built-in method 'map'." );
	}
	else
		o = &val;

	// check fcn value
	if( o->Type() != sym_address )
		throw DevaRuntimeException( "Function expected as argument in vector built-in method 'map'." );

	// return vector
	DOVector* ret = new DOVector();
	ret->reserve( vec.vec_val->size() );

	// TODO: how can we handle methods?? (on vectors, maps, strings and UDTs)
	// (they need a "this" arg to be passed...)

	// walk each item in the vector
	for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
	{
		// push the item
		ex->stack.push_back( *i );
		// call the function given (*must* be a single arg fcn to be used with map
		// builtin)
		ex->ExecuteDevaFunction( o->name, 1 );
		// get the result (return value) and push it onto our return collection
		DevaObject retval = ex->stack.back();
		ex->stack.pop_back();
		ret->push_back( retval );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting vector
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_vector_filter( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'filter' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// function value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'filter'." );

	// function value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'function' argument in vector built-in method 'filter'." );
	}
	else
		o = &val;

	// check fcn value
	if( o->Type() != sym_address )
		throw DevaRuntimeException( "Function expected as argument in vector built-in method 'filter'." );

	// return vector
	DOVector* ret = new DOVector();
	// TODO: should the return vector be allocated as the FULL size of the input
	// vector? we know we're calling it to possibly filter out some items...
	// maybe it should start at *half* the size? or maybe just the default with
	// no extra space reserved??
	ret->reserve( vec.vec_val->size() );

	// TODO: how can we handle methods?? (on vectors, maps, strings and UDTs)
	// (they need a "this" arg to be passed...)

	// walk each item in the vector
	for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
	{
		// push the item
		ex->stack.push_back( *i );
		// call the function given (*must* be a single arg fcn to be used with filter
		// builtin)
		ex->ExecuteDevaFunction( o->name, 1 );
		// get the result (return value) and push it onto our return collection
		DevaObject retval = ex->stack.back();
		ex->stack.pop_back();
		// only add this item to the returned vector if the function returned a
		// 'true' value
		if( ex->evaluate_object_as_boolean( retval ) )
			ret->push_back( *i );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting vector
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_vector_reduce( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'reduce' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// function value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'reduce'." );
	if( vec.vec_val->size() < 2 )
		throw DevaRuntimeException( "Vector built-in method 'reduce' requires a vector with at least two elements." );

	// function value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'function' argument in vector built-in method 'reduce'." );
	}
	else
		o = &val;

	// check fcn value
	if( o->Type() != sym_address )
		throw DevaRuntimeException( "Function expected as argument in vector built-in method 'reduce'." );

	// TODO: how can we handle methods?? (on vectors, maps, strings and UDTs)
	// (they need a "this" arg to be passed...)

	// first iteration uses the first two items in the vector
	ex->stack.push_back( vec.vec_val->at( 1 ) );
	ex->stack.push_back( vec.vec_val->at( 0 ) );
	ex->ExecuteDevaFunction( o->name, 2 );
	DevaObject retval = ex->stack.back();
	ex->stack.pop_back();
	// walk the rest of the items in the vector
	if( vec.vec_val->size() > 2 )
	{
		for( DOVector::iterator i = vec.vec_val->begin()+2; i != vec.vec_val->end(); ++i )
		{
			// push the item
			ex->stack.push_back( *i );
			// use the retval from the previous iteration as the first arg to the fcn
			ex->stack.push_back( retval );
			// call the function given (*must* be a double arg fcn to be used with
			// reduce builtin)
			ex->ExecuteDevaFunction( o->name, 2 );
			// get the result (return value) and push it onto our return collection
			retval = ex->stack.back();
			ex->stack.pop_back();
		}
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting value
	ex->stack.push_back( DevaObject( "", retval ) );
}

// helper for slicing in steps
static size_t s_step = 1;
static size_t s_i = 0;
bool if_step( DevaObject )
{
	if( s_i++ % s_step == 0 )
		return false;
	else
		return true;
}

void do_vector_slice( Executor *ex )
{
	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'slice' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	size_t i_start = 0;
	size_t i_end = -1;
	size_t i_step = 1;
	if( Executor::args_on_stack > 0 )
	{
		// start position to insert at is next on stack
		DevaObject start = ex->stack.back();
		ex->stack.pop_back();
		// start position
		if( start.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'slice'." );
		// TODO: start needs to be integral values. error if they aren't
		i_start = (size_t)start.num_val;
	}
	if( Executor::args_on_stack > 1 )
	{
		// end of sub-vector to slice
		DevaObject end = ex->stack.back();
		ex->stack.pop_back();
		if( end.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'length' argument in vector built-in method 'slice'." );
		// TODO: length need to be integral values. error if they aren't
		i_end = (size_t)end.num_val;
	}
	if( Executor::args_on_stack > 2 )
	{
		// 'step' value to slice with
		DevaObject step = ex->stack.back();
		ex->stack.pop_back();
		if( step.Type() != sym_number )
			throw DevaRuntimeException( "Number expected in for 'length' argument in vector built-in method 'slice'." );
		// TODO: length need to be integral values. error if they aren't
		i_step = (size_t)step.num_val;
	}

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'slice'." );

	size_t sz = vec.vec_val->size();

	// default length is the entire vector
	if( i_end == -1 )
		i_end = sz;

	if( i_start >= sz || i_start < 0 )
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'slice'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'slice'." );
	if( i_end < i_start )
		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'slice': start is greater than end." );
	if( i_step < 1 )
		throw DevaRuntimeException( "Invalid 'step' argument in vector built-in method 'slice': step is less than one." );

	// slice the vector
	DevaObject ret;
	// 'step' is '1' (the default)
	if( i_step == 1 )
	{
		// create a new vector object that is a copy of the 'sub-vector' we're
		// looking for
//		DOVector* v = new DOVector( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );
		DOVector* v = new DOVector( *(vec.vec_val), i_start, i_end );
		ret = DevaObject( "", v );
	}
	// otherwise the vector class doesn't help us, have to do it manually
	else
	{
		DOVector* v = new DOVector();
		s_i = 0;
		s_step = i_step;
		remove_copy_if( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end, back_inserter( *v ), if_step );
		ret = DevaObject( "", v );
	}

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_vector_join( Executor *ex )
{
	if( Executor::args_on_stack > 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector built-in method 'join'." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'join'." );

    string sep;
	if( Executor::args_on_stack == 1 )
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
    for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
    {
        ostringstream s;
        if( i != vec.vec_val->begin() )
            s << sep;
        s << *i;
        ret += s.str();
    }

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", ret ) );
}


// 'enumerable interface'
void do_vector_rewind( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Vector built-in method 'rewind' takes no arguments." );

	// get the vector object off the top of the stack
	DevaObject vec_obj = get_this( ex, "rewind", sym_vector );

	smart_ptr<DOVector> vec = vec_obj.vec_val;
	vec->index = 0;

	// pop the return address
	ex->stack.pop_back();

	// push a null value onto the stack
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_next( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Vector built-in method 'next' takes no arguments." );

	// get the vector object off the top of the stack
	DevaObject vec_obj = get_this( ex, "next", sym_vector );
	smart_ptr<DOVector> vec = vec_obj.vec_val;

	size_t idx = vec->index;
	size_t size = vec->size();
	bool last = (idx == size);

	// return a vector with the first item being a boolean indicating whether
	// there are more items or not (i.e. returns false when done enumerating)
	// and the second item is the object (null if we're done enumerating)

	DOVector* ret = new DOVector();

	// if we have an object, return true and the object
	if( !last )
	{
		DevaObject out = vec->operator[]( vec->index );
		ret->push_back( DevaObject( "", true ) );
		ret->push_back( DevaObject( "", out ) );
	}
	// otherwise return false and null
	else
	{
		ret->push_back( DevaObject( "", false ) );
		ret->push_back( DevaObject( "", sym_null ) );
	}

	// move to the next item
	vec->index++;

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( DevaObject( "", ret ) );
}
