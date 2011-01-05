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

