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

// builtins_helpers.h
// helper fcns for builtin functions/methods for the deva language
// created by jcs, january 3, 2011

// TODO:
// * 

#ifndef __BUILTINS_HELPERS_H__ 
#define __BUILTINS_HELPERS_H__

#include <string>
#include "executor.h"

using namespace std;


namespace deva
{

class BuiltinHelper
{
	string type;
	string name;
	Frame* frame;
	bool is_method;

public:
	BuiltinHelper( const char* t, const char* n, Frame* f ) : 
		type( (t ? t : "") ), 
		name( n ), 
		frame( f ), 
		is_method( !!t )
	{ }

	// expectations/assertions
	void CheckNumberOfArguments( int num_args_expected );
	void CheckNumberOfArguments( int min_num_args, int max_num_args );
	void ExpectType( Object* obj, ObjectType t );
	void ExpectMapType( Object* obj );
	void ExpectRefType( Object* obj );
	void ExpectNonRefType( Object* obj );
	void ExpectTypes( Object* obj, ObjectType t1, ObjectType t2 );
	void ExpectTypes( Object* obj, ObjectType t1, ObjectType t2, ObjectType t3 );
	void ExpectTypes( Object* obj, ObjectType t1, ObjectType t2, ObjectType t3, ObjectType t4 );
	void ExpectTypes( Object* obj, ObjectType t1, ObjectType t2, ObjectType t3, ObjectType t4, ObjectType t5 );
	void ExpectIntegralNumber( Object* obj );
	void ExpectPositiveIntegralNumber( Object* obj );

	// argument/data handling
	Object* GetLocalN( int local_num );
	inline void ReturnVal( Object o ) { IncRef( o ); ex->PushStack( o ); }
};


} // end namespace deva

#endif // __BUILTINS_HELPERS_H__
