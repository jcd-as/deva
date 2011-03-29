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


const string vector_builtin_names[] = 
{
	string( "append" ),
	string( "length" ),
	string( "copy" ),
	string( "concat" ),
	string( "min" ),
	string( "max" ),
	string( "pop" ),
	string( "insert" ),
	string( "remove" ),
	string( "find" ),
	string( "rfind" ),
	string( "count" ),
	string( "reverse" ),
	string( "sort" ),
	string( "map" ),
	string( "filter" ),
	string( "reduce" ),
	string( "any" ),
	string( "all" ),
	string( "slice" ),
	string( "join" ),
	string( "rewind" ),
	string( "next" ),
};
// ...and function pointers to the executor functions for them
NativeFunctionPtr vector_builtin_fcns[] = 
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
	do_vector_any,
	do_vector_all,
	do_vector_slice,
	do_vector_join,
	do_vector_rewind,
	do_vector_next,
};
Object vector_builtin_fcn_objs[] = 
{
	Object( do_vector_append ),
	Object( do_vector_length ),
	Object( do_vector_copy ),
	Object( do_vector_concat ),
	Object( do_vector_min ),
	Object( do_vector_max ),
	Object( do_vector_pop ),
	Object( do_vector_insert ),
	Object( do_vector_remove ),
	Object( do_vector_find ),
	Object( do_vector_rfind ),
	Object( do_vector_count ),
	Object( do_vector_reverse ),
	Object( do_vector_sort ),
	Object( do_vector_map ),
	Object( do_vector_filter ),
	Object( do_vector_reduce ),
	Object( do_vector_any ),
	Object( do_vector_all ),
	Object( do_vector_slice ),
	Object( do_vector_join ),
	Object( do_vector_rewind ),
	Object( do_vector_next ),
};
const int num_of_vector_builtins = sizeof( vector_builtin_names ) / sizeof( vector_builtin_names[0] );


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

Object* GetVectorBuiltinObjectRef( const string & name )
{
	const string* i = find( vector_builtin_names, vector_builtin_names + num_of_vector_builtins, name );
	if( i == vector_builtin_names + num_of_vector_builtins )
	{
		return NULL;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&vector_builtin_names;
	int idx = l / sizeof( string );
	if( idx > num_of_vector_builtins )
	{
		return NULL;
	}
	else
	{
		// return the function object
		return &vector_builtin_fcn_objs[idx];
	}
}


/////////////////////////////////////////////////////////////////////////////
// vector builtins
/////////////////////////////////////////////////////////////////////////////

void do_vector_append( Frame *frame )
{
	BuiltinHelper helper( "vector", "append", frame );

	helper.CheckNumberOfArguments( 2 );
	Object* vec = helper.GetLocalN( 0 );
	helper.ExpectType( vec, obj_vector );
	Object* po = helper.GetLocalN( 1 );

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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );
	Object* in = helper.GetLocalN( 1 );
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );
	Object* pos = helper.GetLocalN( 1 );
	helper.ExpectIntegralNumber( pos );
	Object* val = helper.GetLocalN( 2 );

	size_t i = (size_t)pos->d;
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );
	Object* startobj = helper.GetLocalN( 1 );

	helper.ExpectPositiveIntegralNumber( startobj );

	int start = (int)startobj->d;
	int end = -1;
	if( frame->NumArgsPassed() == 3 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'remove'." );
	if( (size_t)end > sz || end < 0 )
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 1 );

	int start = 0;
	int end = -1;
	if( num_args > 2 )
	{
		Object* startobj = helper.GetLocalN( 2 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 3 )
	{
		Object* endobj = helper.GetLocalN( 3 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'find'." );
	if( (size_t)end > sz || end < 0 )
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 1 );

	int start = 0;
	int end = -1;
	if( num_args > 2 )
	{
		Object* startobj = helper.GetLocalN( 2 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 3 )
	{
		Object* endobj = helper.GetLocalN( 3 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'rfind'." );
	if( (size_t)end > sz || end < 0 )
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* value = helper.GetLocalN( 1 );

	int start = 0;
	int end = -1;
	if( num_args > 2 )
	{
		Object* startobj = helper.GetLocalN( 2 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 3 )
	{
		Object* endobj = helper.GetLocalN( 3 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'count'." );
	if( (size_t)end > sz || end < 0 )
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	int start = 0;
	int end = -1;
	if( num_args > 1 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 2 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'reverse'." );
	if( (size_t)end > sz || end < 0 )
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	int start = 0;
	int end = -1;
	if( num_args > 1 )
	{
		Object* startobj = helper.GetLocalN( 1 );
		helper.ExpectPositiveIntegralNumber( startobj );
		start = (int)startobj->d;
	}
	if( num_args > 2 )
	{
		Object* endobj = helper.GetLocalN( 2 );
		helper.ExpectIntegralNumber( endobj );
		end = (int)endobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'sort'." );
	if( (size_t)end > sz || end < 0 )
		throw RuntimeException( "Invalid 'end' argument in vector built-in method 'sort'." );
	if( end < start )
		throw RuntimeException( "Invalid arguments in vector built-in method 'sort': start is greater than end." );

	sort( self->v->begin() + start, self->v->begin() + end );

	helper.ReturnVal( Object( obj_null ) );
}


void do_vector_map( Frame *frame )
{
	BuiltinHelper helper( "vector", "map", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* o = helper.GetLocalN( 1 );

	helper.ExpectTypes( o, obj_function, obj_native_function );

	// return vector
	Vector* ret = CreateVector();
	ret->reserve( self->v->size() );

	bool is_method = false;

	int num_args = frame->NumArgsPassed();

	if( o->type == obj_function )
		is_method = o->f->IsMethod();
	else if( o->type == obj_native_function )
		is_method = o->nf.is_method;

	bool has_self = is_method && num_args == 3;

	// a non-method can't have an object passed as argument #2
	if( has_self == 3 )
		throw RuntimeException( "Too many arguments passed to vector built-in method 'map' for a first argument which is not a method." );

	Object* method_self;
	if( num_args == 3 )
	{
		method_self = helper.GetLocalN( 2 ); 
		helper.ExpectTypes( method_self, obj_string, obj_vector, obj_map, obj_class, obj_instance );
	}

	// walk each item in the vector
	for( Vector::iterator i = self->v->begin(); i != self->v->end(); ++i )
	{
		// push the item
		ex->PushStack( *i );
		// push 'self', for methods
		if( has_self )
		{
			// push the object ("self") first
			IncRef( *method_self );
			ex->PushStack( *method_self );
		}
		// call the function given (*must* be a single arg fcn to be used with map
		// builtin)
		if( o->type == obj_function )
			ex->ExecuteFunction( o->f, 1, has_self ? true : false );
		else if( o->type == obj_native_function )
			ex->ExecuteFunction( o->nf, 1, has_self ? true : false );
		// get the result (return value) and push it onto our return collection
		Object retval = ex->PopStack();
		// if we got a string back, we need to allocate a copy in our parent's
		// frame so it doesn't get deleted when we return
		if( retval.type == obj_string )
		{
			const char* str = frame->GetParent()->AddString( string( retval.s ) );
			retval.s = const_cast<char*>(str);
		}
		ret->push_back( retval );
	}

	helper.ReturnVal( Object( ret ) );
}

void do_vector_filter( Frame *frame )
{
	BuiltinHelper helper( "vector", "filter", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* o = helper.GetLocalN( 1 );
	helper.ExpectTypes( o, obj_function, obj_native_function );

	bool is_method = false;
	int num_args = frame->NumArgsPassed();

	if( o->type == obj_function )
		is_method = o->f->IsMethod();
	else if( o->type == obj_native_function )
		is_method = o->nf.is_method;

	bool has_self = is_method && num_args == 3;

	// a non-method can't have an object passed as argument #2
	if( has_self == 3 )
		throw RuntimeException( "Too many arguments passed to vector built-in method 'filter' for a first argument which is not a method." );

	// TODO: allow methods
	Object* method_self;
	if( num_args == 3 )
	{
		method_self = helper.GetLocalN( 2 ); 
		helper.ExpectTypes( method_self, obj_string, obj_vector, obj_map, obj_class, obj_instance );
	}

	// return vector
	Vector* ret = CreateVector();
	ret->reserve( self->v->size() );

	// walk each item in the vector
	for( Vector::iterator i = self->v->begin(); i != self->v->end(); ++i )
	{
		// push the item
		IncRef( *i );
		ex->PushStack( *i );
		// push 'self', for methods
		if( has_self )
		{
			// TODO:
			// if this is a method, 'self' for the method is on the stack
			// the item and 'self' are now in reverse order and need to be swapped
			// 'self' also needs to be pushed and inc ref'd for each additional time through the
			// loop... (beyond the first, when 'self' is already there)

			// push the object ("self") first
			IncRef( *method_self );
			ex->PushStack( *method_self );
		}
		// call the function given (*must* be a single arg fcn to be used with map
		// builtin)
		if( o->type == obj_function )
			ex->ExecuteFunction( o->f, 1, has_self ? true : false );
		else if( o->type == obj_native_function )
			ex->ExecuteFunction( o->nf, 1, has_self ? true : false );
		// get the result (return value), but only add this item to the returned 
		// vector if the function returned a 'true' value
		Object retval = ex->PopStack();
		if( retval.CoerceToBool() )
		{
			// if we got a string, we need to allocate a copy in our parent's
			// frame so it doesn't get deleted when we return
			if( i->type == obj_string )
			{
				const char* str = frame->GetParent()->AddString( string( i->s ) );
				i->s = const_cast<char*>(str);
			}
			IncRef( *i );
			ret->push_back( *i );
		}
		DecRef( retval );
	}

	helper.ReturnVal( Object( ret ) );
}

void do_vector_reduce( Frame *frame )
{
	BuiltinHelper helper( "vector", "reduce", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* o = helper.GetLocalN( 1 );

	helper.ExpectTypes( o, obj_function, obj_native_function );

	bool is_method = false;

	int num_args = frame->NumArgsPassed();

	if( o->type == obj_function )
		is_method = o->f->IsMethod();
	else if( o->type == obj_native_function )
		is_method = o->nf.is_method;

	bool has_self = is_method && num_args == 3;

	// a non-method can't have an object passed as argument #2
	if( has_self == 3 )
		throw RuntimeException( "Too many arguments passed to vector built-in method 'map' for a first argument which is not a method." );

	Object* method_self;
	if( num_args == 3 )
	{
		method_self = helper.GetLocalN( 2 ); 
		helper.ExpectTypes( method_self, obj_string, obj_vector, obj_map, obj_class, obj_instance );
	}

	size_t sz = self->v->size();
	// first iteration uses the last two items in the vector
	ex->PushStack( self->v->operator[]( sz-2 ) );
	ex->PushStack( self->v->operator[]( sz-1 ) );
	// push 'self', for methods
	if( has_self )
	{
		// push the object ("self") first
		IncRef( *method_self );
		ex->PushStack( *method_self );
	}
	// call the function
	if( o->type == obj_function )
		ex->ExecuteFunction( o->f, 2, has_self ? true : false );
	else if( o->type == obj_native_function )
		ex->ExecuteFunction( o->nf, 2, has_self ? true : false );
	Object retval = ex->PopStack();
	// walk the rest of the items in the vector
	if( self->v->size() > 2 )
	{
		for( int i = sz-3; i >= 0; i-- )
		{
			// push the item
			ex->PushStack( self->v->operator[]( i ) );
			// use the retval from the previous iteration as the first arg to the fcn
			ex->PushStack( retval );
			// push 'self', for methods
			if( has_self )
			{
				// push the object ("self") first
				IncRef( *method_self );
				ex->PushStack( *method_self );
			}
			// call the function given (*must* be a double arg fcn to be used with
			// reduce builtin)
			if( o->type == obj_function )
				ex->ExecuteFunction( o->f, 2, has_self ? true : false );
			else if( o->type == obj_native_function )
				ex->ExecuteFunction( o->nf, 2, has_self ? true : false );
			// get the result (return value) and push it onto our return collection
			retval = ex->PopStack();
		}
	}

	// if we're returning a string, ensure memory will exist in frame returned to
	if( retval.type == obj_string )
	{
		const char* str = frame->GetParent()->AddString( string( retval.s ) );
		retval.s = const_cast<char*>(str);
	}

	helper.ReturnVal( retval );
}

void do_vector_any( Frame *frame )
{
	BuiltinHelper helper( "vector", "any", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* o = helper.GetLocalN( 1 );

	helper.ExpectTypes( o, obj_function, obj_native_function );

	bool is_method = false;

	int num_args = frame->NumArgsPassed();

	if( o->type == obj_function )
		is_method = o->f->IsMethod();
	else if( o->type == obj_native_function )
		is_method = o->nf.is_method;

	bool has_self = is_method && num_args == 3;

	// a non-method can't have an object passed as argument #2
	if( has_self == 3 )
		throw RuntimeException( "Too many arguments passed to vector built-in method 'map' for a first argument which is not a method." );

	Object* method_self;
	if( num_args == 3 )
	{
		method_self = helper.GetLocalN( 2 ); 
		helper.ExpectTypes( method_self, obj_string, obj_vector, obj_map, obj_class, obj_instance );
	}

	bool value = false;

	// walk each item in the vector
	for( Vector::iterator i = self->v->begin(); i != self->v->end(); ++i )
	{
		// push the item
		ex->PushStack( *i );
		// push 'self', for methods
		if( has_self )
		{
			// push the object ("self") first
			IncRef( *method_self );
			ex->PushStack( *method_self );
		}
		// call the function given (*must* be a single arg fcn to be used with map
		// builtin)
		if( o->type == obj_function )
			ex->ExecuteFunction( o->f, 1, has_self ? true : false );
		else if( o->type == obj_native_function )
			ex->ExecuteFunction( o->nf, 1, has_self ? true : false );
		// get the result (return value)
		Object retval = ex->PopStack();
		// if it evaluates to true, bail
		if( retval.CoerceToBool() )
		{
			value = true;
			break;
		}
	}

	helper.ReturnVal( Object( value ) );
}

void do_vector_all( Frame *frame )
{
	BuiltinHelper helper( "vector", "all", frame );

	helper.CheckNumberOfArguments( 2, 3 );
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	Object* o = helper.GetLocalN( 1 );

	helper.ExpectTypes( o, obj_function, obj_native_function );

	bool is_method = false;

	int num_args = frame->NumArgsPassed();

	if( o->type == obj_function )
		is_method = o->f->IsMethod();
	else if( o->type == obj_native_function )
		is_method = o->nf.is_method;

	bool has_self = is_method && num_args == 3;

	// a non-method can't have an object passed as argument #2
	if( has_self == 3 )
		throw RuntimeException( "Too many arguments passed to vector built-in method 'map' for a first argument which is not a method." );

	Object* method_self;
	if( num_args == 3 )
	{
		method_self = helper.GetLocalN( 2 ); 
		helper.ExpectTypes( method_self, obj_string, obj_vector, obj_map, obj_class, obj_instance );
	}

	bool value = true;

	// walk each item in the vector
	for( Vector::iterator i = self->v->begin(); i != self->v->end(); ++i )
	{
		// push the item
		ex->PushStack( *i );
		// push 'self', for methods
		if( has_self )
		{
			// push the object ("self") first
			IncRef( *method_self );
			ex->PushStack( *method_self );
		}
		// call the function given (*must* be a single arg fcn to be used with map
		// builtin)
		if( o->type == obj_function )
			ex->ExecuteFunction( o->f, 1, has_self ? true : false );
		else if( o->type == obj_native_function )
			ex->ExecuteFunction( o->nf, 1, has_self ? true : false );
		// get the result (return value)
		Object retval = ex->PopStack();
		// if it evaluates to false, bail
		if( !retval.CoerceToBool() )
		{
			value = false;
			break;
		}
	}

	helper.ReturnVal( Object( value ) );
}


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
	BuiltinHelper helper( "vector", "slice", frame );

	helper.CheckNumberOfArguments( 3, 4 );
	int num_args = frame->NumArgsPassed();
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	int start = 0;
	int end = -1;
	int step = 1;

	Object* startobj = helper.GetLocalN( 1 );
	helper.ExpectPositiveIntegralNumber( startobj );
	start = (int)startobj->d;

	Object* endobj = helper.GetLocalN( 2 );
	helper.ExpectIntegralNumber( endobj );
	end = (int)endobj->d;
	
	if( num_args > 3 )
	{
		Object* stepobj = helper.GetLocalN( 3 );
		helper.ExpectIntegralNumber( stepobj );
		step = (int)stepobj->d;
	}

	size_t sz = self->v->size();

	if( end == -1 )
		end = sz;

	if( (size_t)start >= sz || start < 0 )
		throw RuntimeException( "Invalid 'start' argument in vector built-in method 'slice'." );
	if( (size_t)end > sz || end < 0 )
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
	Object* self = helper.GetLocalN( 0 );
	helper.ExpectType( self, obj_vector );

	const char* separator = "";
	if( num_args == 2 )
	{
		Object* sep = helper.GetLocalN( 1 );
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
	BuiltinHelper helper( "vector", "next", frame );

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

