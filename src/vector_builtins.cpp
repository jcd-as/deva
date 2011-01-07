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

// vector_builtins.cpp
// builtin vector methods for the deva language
// created by jcs, january 3, 2011

// TODO:
// * 

#include "vector_builtins.h"
#include "builtins_helpers.h"
#include <algorithm>
#include <sstream>

using namespace std;


namespace deva
{


bool IsVectorBuiltin( const string & name )
{
	const string* i = find( vector_builtin_names, vector_builtin_names + num_of_vector_builtins, name );
	if( i != vector_builtin_names + num_of_vector_builtins ) return true;
	else return false;
}

NativeFunction GetVectorBuiltin( const string & name )
{
	const string* i = find( vector_builtin_names, vector_builtin_names + num_of_vector_builtins, name );
	if( i == vector_builtin_names + num_of_vector_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&vector_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_vector_builtins )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	else
	{
		// return the function
		NativeFunction nf;
		nf.p = vector_builtin_fcns[idx];
		nf.is_method = true;
		return nf;
	}
}


/////////////////////////////////////////////////////////////////////////////
// vector builtins
/////////////////////////////////////////////////////////////////////////////

void do_vector_append( Frame *frame )
{
	BuiltinHelper helper( "vector", "append", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* vec = helper.GetLocalN( 1 );
	helper.ExpectType( vec, obj_vector );
	Object* po = helper.GetLocalN( 0 );

	vec->v->push_back( *po );
	// inc ref the item being appended
	IncRef( *po );

	helper.ReturnVal( Object( obj_null ) );
}

void do_vector_length( Frame *frame )
{
	BuiltinHelper helper( "vector", "length", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* po = helper.GetLocalN( 0 );
	helper.ExpectType( po, obj_vector );

	int len = po->v->size();

	helper.ReturnVal( Object( (double)len ) );
}


void do_vector_copy( Frame *frame )
{
	BuiltinHelper helper( "vector", "copy", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* po = helper.GetLocalN( 0 );
	helper.ExpectType( po, obj_vector );

	Object copy = Object( CreateVector( *po->v ) );

	helper.ReturnVal( copy );
}

void do_vector_concat( Frame *frame )
{
	BuiltinHelper helper( "vector", "concat", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* self = helper.GetLocalN( 1 );
	helper.ExpectType( self, obj_vector );
	Object* in = helper.GetLocalN( 0 );
	helper.ExpectType( in, obj_vector );

	// nothing in <algorithm> to help us with this...
	// make sure there's enough reserve size
	if( self->v->capacity() < self->v->size() + in->v->size() )
		self->v->reserve( self->v->size() + in->v->size() );
	// append each element
	for( Vector::iterator i = in->v->begin(); i != in->v->end(); ++i )
	{
		self->v->push_back( *i );
		// inc ref each item being added
		IncRef( *i );
	}

	helper.ReturnVal( Object( obj_null ) );
}

struct MinComparator
{
	static ObjectType type;
	bool operator()( const Object & lhs, const Object & rhs )
	{
		if( lhs.type != type || rhs.type != type )
			throw RuntimeException( "Vector built-in method 'min' called on a vector with non-homogenous contents." );
		return lhs < rhs;
	}
};
ObjectType MinComparator::type = obj_end;

void do_vector_min( Frame *frame )
{
	BuiltinHelper helper( "vector", "min", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	if( self->v->size() == 0 )
		throw RuntimeException( "Vector builtin method 'min' called on an empty vector." );

	// find the min element
	MinComparator::type = self->v->operator[]( 0 ).type;
	Vector::iterator it = min_element( self->v->begin(), self->v->end(), MinComparator() );

	helper.ReturnVal( *it );
}

void do_vector_max( Frame *frame )
{
	BuiltinHelper helper( "vector", "max", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	if( self->v->size() == 0 )
		throw RuntimeException( "Vector builtin method 'max' called on an empty vector." );

	// find the max element
	MinComparator::type = self->v->operator[]( 0 ).type;
	Vector::iterator it = max_element( self->v->begin(), self->v->end(), MinComparator() );

	helper.ReturnVal( *it );
}

void do_vector_pop( Frame *frame )
{
	BuiltinHelper helper( "vector", "pop", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	if( self->v->size() == 0 )
		throw RuntimeException( "Vector builtin method 'pop' called on an empty vector." );

	Object o = self->v->back();
	self->v->pop_back();

	helper.ReturnVal( o );
}

void do_vector_insert( Frame *frame )
{
	BuiltinHelper helper( "vector", "insert", frame );

	helper.CheckNumberOfArguments( 3 );
	Object* self = helper.GetLocalN( 2 );
	helper.ExpectType( self, obj_vector );
	Object* pos = helper.GetLocalN( 0 );
	helper.ExpectIntegralNumber( pos );
	Object* val = helper.GetLocalN( 1 );

	int i = (int)pos->d;
	if( i > self->v->size() )
		throw RuntimeException( "Position argument greater than vector size in vector built-in method 'insert'." );

	// insert the value
	self->v->insert( self->v->begin() + i, *val );

	helper.ReturnVal( Object( obj_null ) );
}

void do_vector_remove( Frame *frame )
{
	BuiltinHelper helper( "vector", "remove", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( frame->NumArgsPassed() - 1 );
	helper.ExpectType( self, obj_vector );
	Object* startobj = helper.GetLocalN( 0 );

	helper.ExpectIntegralNumber( startobj );

	int start = (int)startobj->d;
	int end = -1;
	if( frame->NumArgsPassed() == 3 )
	{
		Object* endobj = helper.GetLocalN( 1 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'remove'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'remove'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'remove': start is greater than end." );

	// remove the value
	if( start == end )
		self->v->erase( self->v->begin() + start );
	else
		self->v->erase( self->v->begin() + start, self->v->begin() + end );

	helper.ReturnVal( Object( obj_null ) );
}

void do_vector_find( Frame *frame )
{
	BuiltinHelper helper( "vector", "find", frame );

	helper.CheckNumberOfArguments( 2, 4 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 0 );

	int start = 0;
	int end = -1;
	if( num_args > 2 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 3 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		start = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'find'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'find'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'find': start is greater than end." );

	// find the element that matches
	// find/find_xxx from <algorithm> won't help us, we need an index, not an iterator
	Object ret;
	bool found = false;
	for( int i = start; i < end; ++i )
	{
		if( self->v->operator[]( i ) == *value )
		{
			ret = Object( (double)i );
			found = true;
			break;
		}
	}

	if( !found )
		ret = Object( obj_null );

	helper.ReturnVal( ret );
}

void do_vector_rfind( Frame *frame )
{
	BuiltinHelper helper( "vector", "rfind", frame );

	helper.CheckNumberOfArguments( 2, 4 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 0 );

	int start = 0;
	int end = -1;
	if( num_args > 2 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 3 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'rfind'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'rfind'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'rfind': start is greater than end." );

	// find the element that matches
	// find/find_xxx from <algorithm> won't help us, we need an index, not an iterator
	Object ret;
	bool found = false;
	for( int i = end-1; i >= start; --i )
	{
		if( self->v->operator[]( i ) == *value )
		{
			ret = Object( (double)i );
			found = true;
			break;
		}
	}

	if( !found )
		ret = Object( obj_null );

	helper.ReturnVal( ret );
}

void do_vector_count( Frame *frame )
{
	BuiltinHelper helper( "vector", "count", frame );

	helper.CheckNumberOfArguments( 2, 4 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 0 );

	int start = 0;
	int end = -1;
	if( num_args > 2 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 3 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'count'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'count'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'count': start is greater than end." );

	// count the value
	int num = count( self->v->begin() + start, self->v->begin() + end, *value );

	helper.ReturnVal( Object( (double)num ) );
}

void do_vector_reverse( Frame *frame )
{
	BuiltinHelper helper( "vector", "reverse", frame );

	helper.CheckNumberOfArguments( 1, 3 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );
	Object* startobj = helper.GetLocalN( 0 );

	int start = 0;
	int end = -1;
	if( num_args > 1 )
	{
		Object* startobj = helper.GetLocalN( 0 );
		helper.ExpectIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 2 )
	{
		Object* endobj = helper.GetLocalN( 1 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'reverse'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'reverse'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'reverse': start is greater than end." );

	reverse( self->v->begin() + start, self->v->begin() + end );

	helper.ReturnVal( Object( obj_null ) );
}


void do_vector_sort( Frame *frame )
{
	BuiltinHelper helper( "vector", "sort", frame );

	helper.CheckNumberOfArguments( 1, 3 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );
	Object* startobj = helper.GetLocalN( 0 );

	int start = 0;
	int end = -1;
	if( num_args > 1 )
	{
		Object* startobj = helper.GetLocalN( 0 );
		helper.ExpectIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 2 )
	{
		Object* endobj = helper.GetLocalN( 1 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'sort'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'sort'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'sort': start is greater than end." );

	sort( self->v->begin() + start, self->v->begin() + end );

	helper.ReturnVal( Object( obj_null ) );
}

/*
void do_vector_map( Frame *frame )
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

	// TODO: (a built-in will be of type sym_unknown)

	// return vector
	DOVector* ret = new DOVector();
	ret->reserve( vec.vec_val->size() );

	// handle 'class methods' ('static' methods in c++ lingo)
	// the "@" sign in the fcn name indicates it is a method
	// get the lhs and rhs of the "@" (method & object)
	bool is_method = false;
	string fcn, object;
	DevaObject* ob_ptr;
	size_t idx = o->name.find( "@" );
	if( idx != string::npos )
	{
		is_method = true;

		fcn.assign( o->name, 0, idx );
		// TODO: ensure idx + 1 < o->name.length()
		object.assign( o->name, idx + 1, o->name.length() );

		// find the object in the current scope
		ob_ptr = ex->find_symbol( DevaObject( object, sym_unknown ) );
		if( !ob_ptr )
			throw DevaRuntimeException( "Symbol not found for the object instance of the method passed to the 'function' argument in vector built-in method 'map'." );
		// TODO: how to ensure object is a class, not an instance??
		// (the ob_ptr will always point to a sym_class object at this point)
	}

	// walk each item in the vector
	for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
	{
		// push the item
		ex->stack.push_back( *i );
		// push 'self', for methods
		if( is_method )
		{
			// push the object ("self") first
			ex->stack.push_back( *ob_ptr );
		}
		// call the function given (*must* be a single arg fcn to be used with map
		// builtin)
		if( is_method )
			ex->ExecuteDevaFunction( o->name, 2 );
		else
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

void do_vector_filter( Frame *frame )
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

	// TODO: (a built-in will be of type sym_unknown)

	// return vector
	DOVector* ret = new DOVector();
	// TODO: should the return vector be allocated as the FULL size of the input
	// vector? we know we're calling it to possibly filter out some items...
	// maybe it should start at *half* the size? or maybe just the default with
	// no extra space reserved??
	ret->reserve( vec.vec_val->size() );

	// handle 'class methods' ('static' methods in c++ lingo)
	// the "@" sign in the fcn name indicates it is a method
	// get the lhs and rhs of the "@" (method & object)
	bool is_method = false;
	string fcn, object;
	DevaObject* ob_ptr;
	size_t idx = o->name.find( "@" );
	if( idx != string::npos )
	{
		is_method = true;

		fcn.assign( o->name, 0, idx );
		// TODO: ensure idx + 1 < o->name.length()
		object.assign( o->name, idx + 1, o->name.length() );

		// find the object in the current scope
		ob_ptr = ex->find_symbol( DevaObject( object, sym_unknown ) );
		if( !ob_ptr )
			throw DevaRuntimeException( "Symbol not found for the object instance of the method passed to the 'function' argument in vector built-in method 'filter'." );
		// TODO: how to ensure object is a class, not an instance??
		// (the ob_ptr will always point to a sym_class object at this point)
	}

	// walk each item in the vector
	for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
	{
		// push the item
		ex->stack.push_back( *i );
		// push 'self' for methods
		if( is_method )
		{
			// push the object ("self") first
			ex->stack.push_back( *ob_ptr );
		}
		// call the function given (*must* be a single arg fcn to be used with filter
		// builtin)
		if( is_method )
			ex->ExecuteDevaFunction( o->name, 2 );
		else
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

void do_vector_reduce( Frame *frame )
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

	// TODO: (a built-in will be of type sym_unknown)

	// handle 'class methods' ('static' methods in c++ lingo)
	// the "@" sign in the fcn name indicates it is a method
	// e.g. 'method@class'. BUT...we need the instance object!
	// get the lhs and rhs of the "@" (method & object)
	bool is_method = false;
	string fcn, object;
	DevaObject* ob_ptr;
	size_t idx = o->name.find( "@" );
	if( idx != string::npos )
	{
		is_method = true;

		fcn.assign( o->name, 0, idx );
		// TODO: ensure idx + 1 < o->name.length()
		object.assign( o->name, idx + 1, o->name.length() );

		// find the object in the current scope
		ob_ptr = ex->find_symbol( DevaObject( object, sym_unknown ) );
		if( !ob_ptr )
			throw DevaRuntimeException( "Symbol not found for the object instance of the method passed to the 'function' argument in vector built-in method 'reduce'." );
		// TODO: how to ensure object is a class, not an instance??
		// (the ob_ptr will always point to a sym_class object at this point)
	}

	// first iteration uses the first two items in the vector
	ex->stack.push_back( vec.vec_val->at( 1 ) );
	ex->stack.push_back( vec.vec_val->at( 0 ) );
	// push 'self'
	if( is_method )
	{
		// push the object ("self") first
		ex->stack.push_back( *ob_ptr );
	}
	// call the deva function
	if( is_method )
		ex->ExecuteDevaFunction( o->name, 3 );
	else
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
			// push 'self'
			if( is_method )
			{
				// push the object ("self") first
				ex->stack.push_back( *ob_ptr );
			}
			// call the function given (*must* be a double arg fcn to be used with
			// reduce builtin)
			if( is_method )
				ex->ExecuteDevaFunction( o->name, 3 );
			else
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

void do_vector_any( Frame *frame )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'any' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// function value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'any'." );

	// function value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'function' argument in vector built-in method 'any'." );
	}
	else
		o = &val;

	// check fcn value
	if( o->Type() != sym_address )
		throw DevaRuntimeException( "Function expected as argument in vector built-in method 'any'." );

	// TODO: (a built-in will be of type sym_unknown)


	// handle 'class methods' ('static' methods in c++ lingo)
	// the "@" sign in the fcn name indicates it is a method
	// get the lhs and rhs of the "@" (method & object)
	bool is_method = false;
	string fcn, object;
	DevaObject* ob_ptr;
	size_t idx = o->name.find( "@" );
	if( idx != string::npos )
	{
		is_method = true;

		fcn.assign( o->name, 0, idx );
		// TODO: ensure idx + 1 < o->name.length()
		object.assign( o->name, idx + 1, o->name.length() );

		// find the object in the current scope
		ob_ptr = ex->find_symbol( DevaObject( object, sym_unknown ) );
		if( !ob_ptr )
			throw DevaRuntimeException( "Symbol not found for the object instance of the method passed to the 'function' argument in vector built-in method 'any'." );
		// TODO: how to ensure object is a class, not an instance??
		// (the ob_ptr will always point to a sym_class object at this point)
	}

	bool any_true = false;

	// walk each item in the vector
	for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
	{
		// push the item
		ex->stack.push_back( *i );
		// push 'self', for methods
		if( is_method )
		{
			// push the object ("self") first
			ex->stack.push_back( *ob_ptr );
		}
		// call the function given (*must* be a single arg fcn to be used with
		// the 'any' builtin)
		if( is_method )
			ex->ExecuteDevaFunction( o->name, 2 );
		else
			ex->ExecuteDevaFunction( o->name, 1 );
		// get the result (return value)
		DevaObject retval = ex->stack.back();
		ex->stack.pop_back();
		// return type must be boolean
		if( retval.Type() != sym_boolean )
			throw DevaRuntimeException( "Function used with the vector built-in method 'any' returned a non-boolean." );
		// if it's 'true', bail
		if( retval.bool_val )
		{
			any_true = true;
			break;
		}
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting vector
	if( any_true )
		ex->stack.push_back( DevaObject( "", true ) );
	else
		ex->stack.push_back( DevaObject( "", false ) );
}

void do_vector_all( Frame *frame )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to vector 'all' built-in method." );

	// get the vector object off the top of the stack
	DevaObject vec = ex->stack.back();
	ex->stack.pop_back();

	// function value is next on stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	
	// vector
	if( vec.Type() != sym_vector )
		throw DevaICE( "Vector expected in vector built-in method 'all'." );

	// function value
	DevaObject* o;
	if( val.Type() == sym_unknown )
	{
		o = ex->find_symbol( val );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for the 'function' argument in vector built-in method 'all'." );
	}
	else
		o = &val;

	// check fcn value
	if( o->Type() != sym_address )
		throw DevaRuntimeException( "Function expected as argument in vector built-in method 'all'." );

	// TODO: (a built-in will be of type sym_unknown)


	// handle 'class methods' ('static' methods in c++ lingo)
	// the "@" sign in the fcn name indicates it is a method
	// get the lhs and rhs of the "@" (method & object)
	bool is_method = false;
	string fcn, object;
	DevaObject* ob_ptr;
	size_t idx = o->name.find( "@" );
	if( idx != string::npos )
	{
		is_method = true;

		fcn.assign( o->name, 0, idx );
		// TODO: ensure idx + 1 < o->name.length()
		object.assign( o->name, idx + 1, o->name.length() );

		// find the object in the current scope
		ob_ptr = ex->find_symbol( DevaObject( object, sym_unknown ) );
		if( !ob_ptr )
			throw DevaRuntimeException( "Symbol not found for the object instance of the method passed to the 'function' argument in vector built-in method 'all'." );
		// TODO: how to ensure object is a class, not an instance??
		// (the ob_ptr will always point to a sym_class object at this point)
	}

	bool all_true = true;

	// walk each item in the vector
	for( DOVector::iterator i = vec.vec_val->begin(); i != vec.vec_val->end(); ++i )
	{
		// push the item
		ex->stack.push_back( *i );
		// push 'self', for methods
		if( is_method )
		{
			// push the object ("self") first
			ex->stack.push_back( *ob_ptr );
		}
		// call the function given (*must* be a single arg fcn to be used with
		// the 'all' builtin)
		if( is_method )
			ex->ExecuteDevaFunction( o->name, 2 );
		else
			ex->ExecuteDevaFunction( o->name, 1 );
		// get the result (return value)
		DevaObject retval = ex->stack.back();
		ex->stack.pop_back();
		// return type must be boolean
		if( retval.Type() != sym_boolean )
			throw DevaRuntimeException( "Function used with the vector built-in method 'all' returned a non-boolean." );
		// if it's 'false', bail
		if( !retval.bool_val )
		{
			all_true = false;
			break;
		}
	}

	// pop the return address
	ex->stack.pop_back();

	// return the resulting vector
	if( all_true )
		ex->stack.push_back( DevaObject( "", true ) );
	else
		ex->stack.push_back( DevaObject( "", false ) );
}
*/

// helper for slicing in steps
static size_t s_step = 1;
static size_t s_i = 0;
bool if_step( Object )
{
	if( s_i++ % s_step == 0 )
		return false;
	else
		return true;
}

void do_vector_slice( Frame *frame )
{
//	if( Executor::args_on_stack > 3 || Executor::args_on_stack < 1 )
//		throw DevaRuntimeException( "Incorrect number of arguments to vector 'slice' built-in method." );
//
//	// get the vector object off the top of the stack
//	DevaObject vec = ex->stack.back();
//	ex->stack.pop_back();
//
//	size_t i_start = 0;
//	size_t i_end = -1;
//	size_t i_step = 1;
//	if( Executor::args_on_stack > 0 )
//	{
//		// start position to insert at is next on stack
//		DevaObject start = ex->stack.back();
//		ex->stack.pop_back();
//		// start position
//		if( start.Type() != sym_number )
//			throw DevaRuntimeException( "Number expected in for start position argument in vector built-in method 'slice'." );
//		// TODO: start needs to be integral values. error if they aren't
//		i_start = (size_t)start.num_val;
//	}
//	if( Executor::args_on_stack > 1 )
//	{
//		// end of sub-vector to slice
//		DevaObject end = ex->stack.back();
//		ex->stack.pop_back();
//		if( end.Type() != sym_number )
//			throw DevaRuntimeException( "Number expected in for 'length' argument in vector built-in method 'slice'." );
//		// TODO: length need to be integral values. error if they aren't
//		i_end = (size_t)end.num_val;
//	}
//	if( Executor::args_on_stack > 2 )
//	{
//		// 'step' value to slice with
//		DevaObject step = ex->stack.back();
//		ex->stack.pop_back();
//		if( step.Type() != sym_number )
//			throw DevaRuntimeException( "Number expected in for 'length' argument in vector built-in method 'slice'." );
//		// TODO: length need to be integral values. error if they aren't
//		i_step = (size_t)step.num_val;
//	}
//
//	// vector
//	if( vec.Type() != sym_vector )
//		throw DevaICE( "Vector expected in vector built-in method 'slice'." );
//
//	size_t sz = vec.vec_val->size();
//
//	// default length is the entire vector
//	if( i_end == -1 )
//		i_end = sz;
//
//	if( i_start >= sz || i_start < 0 )
//		throw DevaRuntimeException( "Invalid 'start' argument in vector built-in method 'slice'." );
//	if( i_end > sz || i_end < 0 )
//		throw DevaRuntimeException( "Invalid 'end' argument in vector built-in method 'slice'." );
//	if( i_end < i_start )
//		throw DevaRuntimeException( "Invalid arguments in vector built-in method 'slice': start is greater than end." );
//	if( i_step < 1 )
//		throw DevaRuntimeException( "Invalid 'step' argument in vector built-in method 'slice': step is less than one." );
//
//	// slice the vector
//	DevaObject ret;
//	// 'step' is '1' (the default)
//	if( i_step == 1 )
//	{
//		// create a new vector object that is a copy of the 'sub-vector' we're
//		// looking for
////		DOVector* v = new DOVector( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end );
//		DOVector* v = new DOVector( *(vec.vec_val), i_start, i_end );
//		ret = DevaObject( "", v );
//	}
//	// otherwise the vector class doesn't help us, have to do it manually
//	else
//	{
//		DOVector* v = new DOVector();
//		s_i = 0;
//		s_step = i_step;
//		remove_copy_if( vec.vec_val->begin() + i_start, vec.vec_val->begin() + i_end, back_inserter( *v ), if_step );
//		ret = DevaObject( "", v );
//	}
//
//	// pop the return address
//	ex->stack.pop_back();
//
//	ex->stack.push_back( ret );
	BuiltinHelper helper( "vector", "slice", frame );

	helper.CheckNumberOfArguments( 3, 4 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 0 );

	int start = 0;
	int end = -1;
	int step = 1;

	Object* startobj = helper.GetLocalN( 0 );
	helper.ExpectIntegralNumber( startobj );
	start = (int)startobj->d;

	Object* endobj = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( endobj );
	end = (int)endobj->d;
	
	if( num_args > 3 )
	{
		Object* stepobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( stepobj );
		step = (int)stepobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'slice'." );
	if( end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'slice'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'slice': start is greater than end." );
	if( step < 0 )
		throw RuntimeException( "Invalid 'step' argument in vector built-in method 'slice': must be a positive integral number." );

	// slice the vector
	Object ret;
	// if 'step' is '1' (the default)
	if( step == 1 )
	{
		// create a new vector object that is a copy of the 'sub-vector' we're
		// looking for
		Vector* v = CreateVector( *(self->v), start, end );
		ret = Object( v );
	}
	// otherwise the vector class doesn't help us, have to do it manually
	else
	{
		Vector* v = CreateVector();
		s_i = 0;
		s_step = step;
		remove_copy_if( self->v->begin() + start, self->v->begin() + end, back_inserter( *v ), if_step );
		ret = Object( v );
	}

	helper.ReturnVal( ret );
}

void do_vector_join( Frame *frame )
{
	BuiltinHelper helper( "vector", "join", frame );

	helper.CheckNumberOfArguments( 1, 2 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( num_args - 1 );
	helper.ExpectType( self, obj_vector );

	const char* separator = "";
	if( num_args == 2 )
	{
		Object* sep = helper.GetLocalN( 0 );
		helper.ExpectType( sep, obj_string );
		separator = sep->s;
	}

	// build the return string
	string ret;
	for( Vector::iterator i = self->v->begin(); i != self->v->end(); ++i )
	{
		ostringstream s;
		if( i != self->v->begin() )
			s << separator;
		s << *i;
		ret += s.str();
	}
	// add the string to the parent frame
	char* s = copystr( ret.c_str() );
	frame->GetParent()->AddString( s );

	helper.ReturnVal( Object( s ) );
}

// 'enumerable interface'
void do_vector_rewind( Frame *frame )
{
	BuiltinHelper helper( "vector", "rewind", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* po = helper.GetLocalN( 0 );
	helper.ExpectType( po, obj_vector );

	po->v->index = 0;

	helper.ReturnVal( Object( obj_null ) );
}

void do_vector_next( Frame *frame )
{
	BuiltinHelper helper( "vector", "rewind", frame );

	helper.CheckNumberOfArguments( 1 );
	Object* po = helper.GetLocalN( 0 );
	helper.ExpectType( po, obj_vector );

	size_t idx = po->v->index;
	size_t size = po->v->size();
	bool last = (idx == size);

	// return a vector with the first item being a boolean indicating whether
	// there are more items or not (i.e. returns false when done enumerating)
	// and the second item is the object (null if we're done enumerating)

	Vector* ret = CreateVector();

	// if we have an object, return true and the object
	if( !last )
	{
		Object out = po->v->operator[]( po->v->index );
		ret->push_back( Object( true ) );
		ret->push_back( out );
	}
	// otherwise return false and null
	else
	{
		ret->push_back( Object( false ) );
		ret->push_back( Object( obj_null ) );
	}

	// move to the next item
	po->v->index++;

	helper.ReturnVal( Object( ret ) );
}


} // end namespace deva

