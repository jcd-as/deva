// vector_builtins.cpp
// built-in methods on vector obejcts, for the deva language virtual machine
// created by jcs, october 21, 2009 

// TODO:
// * builtins for: concat(vec), min, max, find, count(n) (return number of n's
//   found), insert, pop, remove(pos), reverse, sort 
//

#include "vector_builtins.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// to add new builtins you must:
// 1) add a new fcn to the vector_builtin_names and vector_builtin_fcns arrays below
// 2) implement the function in this file
// 3) add the function as a friend to Executor (executor.h) so that it can
//    access the private members of Executor (like the stack)

// pre-decls for builtin executors
void do_vector_append( Executor *ex );
void do_vector_length( Executor *ex );
void do_vector_copy( Executor *ex );

// tables defining the built-in function names...
static const string vector_builtin_names[] = 
{
    string( "vector_append" ),
    string( "vector_length" ),
    string( "vector_copy" ),
};
// ...and function pointers to the executor functions for them
typedef void (*vector_builtin_fcn)(Executor*);
vector_builtin_fcn vector_builtin_fcns[] = 
{
    do_vector_append,
    do_vector_length,
    do_vector_copy,
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
		throw DevaICE( "No such built-in function." );
    // compute the index of the function in the look-up table(s)
    long l = (long)i;
    l -= (long)&vector_builtin_names;
    int idx = l / sizeof( string );
    if( idx > num_of_vector_builtins )
		throw DevaICE( "Out-of-array-bounds looking for built-in function." );
    else
        // call the function
        vector_builtin_fcns[idx]( ex );
}

// the built-in executor functions:
void do_vector_append( Executor *ex )
{
	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method append." );

	vec.vec_val->push_back( val );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_vector_length( Executor *ex )
{
	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	int len;
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method length." );

	len = vec.vec_val->size();

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", (double)len ) );
}

void do_vector_copy( Executor *ex )
{
	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

    if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method copy." );

    DevaObject copy;
	// create a new vector object that is a copy of the one we received,
	vector<DevaObject>* v = new vector<DevaObject>( *(vec.vec_val) );
	copy = DevaObject( "", v );

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( copy );
}

