// vector_builtins.cpp
// built-in methods on vector objects, for the deva language virtual machine
// created by jcs, october 21, 2009 

// TODO:
// * 
//

#include "vector_builtins.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// to add new builtins you must:
// 1) add a new fcn to the vector_builtin_names and vector_builtin_fcns arrays below
// 2) implement the function in this file
// 3) add the function as a friend to Executor (executor.h) so that it can
//    access the private members of Executor (namely the stack)

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

	vec.vec_val->push_back( val );

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

	// start position to insert at is next on stack
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
		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'remove'." );
	if( i_end > sz || i_end < 0 )
		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'remove'." );

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

	// count the value
	int num = count( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end, val );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
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

	reverse( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the length
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

	sort( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", sym_null ) );
}
