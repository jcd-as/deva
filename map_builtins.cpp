// map_builtins.cpp
// built-in methods on map objects, for the deva language virtual machine
// created by jcs, october 21, 2009 

// TODO:
// * keys (return keys as vector), values (return values as
// vector)
//

#include "map_builtins.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// to add new builtins you must:
// 1) add a new fcn to the map_builtin_names and map_builtin_fcns arrays below
// 2) implement the function in this file
// 3) add the function as a friend to Executor (executor.h) so that it can
//    access the private members of Executor (namely the stack)

// pre-decls for builtin executors
void do_map_length( Executor *ex );
void do_map_copy( Executor *ex );
void do_map_remove( Executor *ex );
void do_map_find( Executor *ex );
void do_map_keys( Executor *ex );
void do_map_values( Executor *ex );

// tables defining the built-in function names...
static const string map_builtin_names[] = 
{
    string( "map_length" ),
    string( "map_copy" ),
    string( "map_remove" ),
    string( "map_find" ),
    string( "map_keys" ),
    string( "map_values" ),
};
// ...and function pointers to the executor functions for them
typedef void (*map_builtin_fcn)(Executor*);
map_builtin_fcn map_builtin_fcns[] = 
{
    do_map_length,
    do_map_copy,
    do_map_remove,
    do_map_find,
	do_map_keys,
	do_map_values,
};
const int num_of_map_builtins = sizeof( map_builtin_names ) / sizeof( map_builtin_names[0] );

// is this name a built-in function?
bool is_map_builtin( const string & name )
{
    const string* i = find( map_builtin_names, map_builtin_names + num_of_map_builtins, name );
    if( i != map_builtin_names + num_of_map_builtins ) return true;
	else return false;
}

// execute built-in function
void execute_map_builtin( Executor *ex, const string & name )
{
    const string* i = find( map_builtin_names, map_builtin_names + num_of_map_builtins, name );
    if( i == map_builtin_names + num_of_map_builtins )
		throw DevaICE( "No such map built-in method." );
    // compute the index of the function in the look-up table(s)
    long l = (long)i;
    l -= (long)&map_builtin_names;
    int idx = l / sizeof( string );
    if( idx > num_of_map_builtins )
		throw DevaICE( "Out-of-array-bounds looking for map built-in method." );
    else
        // call the function
        map_builtin_fcns[idx]( ex );
}

// the built-in executor functions:
void do_map_length( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to map 'length' built-in method." );

	// get the map object off the top of the stack
	DevaObject map = ex->stack.back();
	ex->stack.pop_back();

	int len;
	if( map.Type() != sym_map )
		throw DevaICE( "Map expected in map built-in method 'length'." );

	len = map.map_val->size();

	// pop the return address
	ex->stack.pop_back();

	// return the length
	ex->stack.push_back( DevaObject( "", (double)len ) );
}

void do_map_copy( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to map 'copy' built-in method." );

	// get the map object off the top of the stack
	DevaObject mp = ex->stack.back();
	ex->stack.pop_back();

    if( mp.Type() != sym_map )
		throw DevaICE( "Map expected in map built-in method 'copy'." );

    DevaObject copy;
	// create a new map object that is a copy of the one we received,
	map<DevaObject, DevaObject>* m = new map<DevaObject, DevaObject>( *(mp.map_val) );
	copy = DevaObject( "", m );

	// pop the return address
	ex->stack.pop_back();

	// return the copy
	ex->stack.push_back( copy );
}

void do_map_remove( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to map 'remove' built-in method." );

	// get the map object off the top of the stack
	DevaObject map = ex->stack.back();
	ex->stack.pop_back();

	// key of item to remove is next on stack
	DevaObject key = ex->stack.back();
	ex->stack.pop_back();

	// map
	if( map.Type() != sym_map )
		throw DevaICE( "Map expected in map built-in method 'remove'." );

	// key
	DevaObject* o;
	if( key.Type() == sym_unknown )
	{
		o = ex->find_symbol( key );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'key' argument in map built-in method 'find'." );
	}
	else
		o = &key;

	// remove the value
	map.map_val->erase( *o );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_map_find( Executor *ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to map 'find' built-in method." );

	// get the map object off the top of the stack
	DevaObject mp = ex->stack.back();
	ex->stack.pop_back();

	// key is next on stack
	DevaObject key = ex->stack.back();
	ex->stack.pop_back();
	
	// map
	if( mp.Type() != sym_map )
		throw DevaICE( "Map expected in map built-in method 'find'." );

	// key
	DevaObject* o;
	if( key.Type() == sym_unknown )
	{
		o = ex->find_symbol( key );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'key' argument in map built-in method 'find'." );
	}
	else
		o = &key;

	DevaObject ret;
	map<DevaObject, DevaObject>::iterator it = mp.map_val->find( *o );
	if( it == mp.map_val->end() )
		ret = DevaObject( "", sym_null );
	else
		ret = DevaObject( it->second );

	// pop the return address
	ex->stack.pop_back();

	ex->stack.push_back( ret );
}

void do_map_keys( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to map 'keys' built-in method." );

	// get the map object off the top of the stack
	DevaObject mp = ex->stack.back();
	ex->stack.pop_back();

	if( mp.Type() != sym_map )
		throw DevaICE( "Map expected in map built-in method 'keys'." );

	size_t sz = mp.map_val->size();
	vector<DevaObject>* v = new vector<DevaObject>();
	v->reserve( sz );
	// for each pair<> element in the map
	for( map<DevaObject, DevaObject>::iterator i = mp.map_val->begin(); i != mp.map_val->end(); ++i )
	{
		// add the key for this element
		v->push_back( DevaObject( "", i->first ) );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the vector of keys
	ex->stack.push_back( DevaObject( "", v ) );
}

void do_map_values( Executor *ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Incorrect number of arguments to map 'values' built-in method." );

	// get the map object off the top of the stack
	DevaObject mp = ex->stack.back();
	ex->stack.pop_back();

	if( mp.Type() != sym_map )
		throw DevaICE( "Map expected in map built-in method 'keys'." );

	size_t sz = mp.map_val->size();
	vector<DevaObject>* v = new vector<DevaObject>();
	v->reserve( sz );
	// for each pair<> element in the map
	for( map<DevaObject, DevaObject>::iterator i = mp.map_val->begin(); i != mp.map_val->end(); ++i )
	{
		// add the value of this element
		v->push_back( i->second );
	}

	// pop the return address
	ex->stack.pop_back();

	// return the vector of values
	ex->stack.push_back( DevaObject( "", v ) );
}

