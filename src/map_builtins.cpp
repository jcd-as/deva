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

// map_builtins.cpp
// builtin map methods for the deva language
// created by jcs, january 5, 2011

// TODO:
// * 

#include "map_builtins.h"

#include "builtins_helpers.h"
#include <algorithm>

using namespace std;


namespace deva
{


bool IsMapBuiltin( const string & name )
{
	const string* i = find( map_builtin_names, map_builtin_names + num_of_map_builtins, name );
	if( i != map_builtin_names + num_of_map_builtins ) return true;
	else return false;
}

NativeFunction GetMapBuiltin( const string & name )
{
	const string* i = find( map_builtin_names, map_builtin_names + num_of_map_builtins, name );
	if( i == map_builtin_names + num_of_map_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&map_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_map_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	else
	{
		// return the function
		NativeFunction nf;
		nf.p = map_builtin_fcns[idx];
		nf.is_method = true;
		return nf;
	}
}

// get the Object* for a builtin
Object* GetMapBuiltinObjectRef( const string & name )
{
	const string* i = find( map_builtin_names, map_builtin_names + num_of_map_builtins, name );
	if( i == map_builtin_names + num_of_map_builtins )
	{
		return NULL;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&map_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_map_builtins )
	{
		return NULL;
	}
	else
	{
		// return the function
		return &map_builtin_fcn_objs[idx];
	}
}

/////////////////////////////////////////////////////////////////////////////
// map builtins
/////////////////////////////////////////////////////////////////////////////


void do_map_length( Frame *frame )
{
	BuiltinHelper helper( "map", "length", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );

	int len = self->m->size();

	helper.ReturnVal( Object( (double)len ) );
}

// 'enumerable interface'
void do_map_rewind( Frame *frame )
{
	BuiltinHelper helper( "map", "rewind", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );

	self->m->index = 0;

	helper.ReturnVal( Object( obj_null ) );
}

void do_map_next( Frame *frame )
{
	BuiltinHelper helper( "map", "next", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );

	size_t idx = self->m->index;
	size_t size = self->m->size();
	bool last = (idx == size);

	// return a vector with the first item being a boolean indicating whether
	// there are more items or not (i.e. returns false when done enumerating)
	// and the second item is the key,value pair as a vector (or null if done)

	Vector* ret = CreateVector();

	// if we have an object, return true and the objects
	if( !last )
	{
		Vector* key_val = CreateVector();

		// get the value from the map
		if( idx >= self->m->size() )
			throw ICE( "Index out-of-range in Map built-in method 'next'." );
		Map::iterator it;
		// this loop is equivalent of "it = mp->begin() + idx;" 
		// (which is required because map iterators don't support the + op)
		it = self->m->begin();
		for( int i = 0; i < idx; ++i ) ++it;

		if( it == self->m->end() )
			throw ICE( "Index out-of-range in Map built-in method 'next'." );

		pair<Object, Object> p = *it;

		key_val->push_back( p.first );
		key_val->push_back( p.second );
		ret->push_back( Object( true ) );
		Object keyobj( key_val );
		IncRef( keyobj );
		ret->push_back( keyobj );
	}
	// otherwise return false and null
	else
	{
		ret->push_back( Object( false ) );
		ret->push_back( Object( obj_null ) );
	}

	// move to the next item
	self->m->index++;

	helper.ReturnVal( Object( ret ) );
}

void do_map_copy( Frame* frame )
{
	BuiltinHelper helper( "map", "copy", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );

	Object copy;
	Map* m = CreateMap( *self->m );
	if( self->type == obj_map )
		copy = Object( m );
	else if( self->type == obj_class )
		copy = Object::CreateClass( m );
	else if( self->type == obj_instance )
		copy = Object::CreateInstance( m );

	helper.ReturnVal( copy );
}

void do_map_remove( Frame* frame )
{
	BuiltinHelper helper( "map", "remove", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );
	Object* o = helper.GetLocalN( 1 );

	// remove the value
	self->m->erase( *o );

	helper.ReturnVal( Object( obj_null ) );
}

void do_map_find( Frame* frame )
{
	BuiltinHelper helper( "map", "find", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );
	Object* o = helper.GetLocalN( 1 );

	Object ret;
	Map::iterator it = self->m->find( *o );
	if( it == self->m->end() )
		ret = Object( obj_null );
	else
		ret = Object( it->second );

	helper.ReturnVal( ret );
}

void do_map_keys( Frame* frame )
{
	BuiltinHelper helper( "map", "keys", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );

	size_t sz = self->m->size();
	Vector* v = CreateVector();
	v->reserve( sz );
	// for each pair<> element in the map
	for( Map::iterator i = self->m->begin(); i != self->m->end(); ++i )
	{
		// add the key for this element
		v->push_back( Object( i->first ) );
	}

	helper.ReturnVal( Object( v ) );
}

void do_map_values( Frame* frame )
{
	BuiltinHelper helper( "map", "values", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );

	size_t sz = self->m->size();
	Vector* v = CreateVector();
	v->reserve( sz );
	// for each pair<> element in the map
	for( Map::iterator i = self->m->begin(); i != self->m->end(); ++i )
	{
		// add the key for this element
		v->push_back( Object( i->second ) );
	}

	helper.ReturnVal( Object( v ) );
}

void do_map_merge( Frame* frame )
{
//	if( Executor::args_on_stack != 1 )
//		throw DevaRuntimeException( "Incorrect number of arguments to map 'merge' built-in method." );
//
//	// get the map object off the top of the stack
//	DevaObject mp = ex->stack.back();
	BuiltinHelper helper( "map", "merge", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectMapType( self );
	Object* o = helper.GetLocalN( 1 );

	self->m->insert( o->m->begin(), o->m->end() );

	helper.ReturnVal( Object( obj_null ) );
}


} // end namespace deva
