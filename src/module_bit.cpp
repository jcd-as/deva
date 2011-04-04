// Copyright (c) 2011 Jbithua C. Shepard
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

// module_bit.cpp
// native bit operations module for deva language, v2
// created by jcs, april 2, 2011

// TODO:
// * 

#include "module_bit.h"
#include "builtins_helpers.h"


namespace deva
{


const string module_bit_names[] =
{
	string( "and" ),
	string( "or" ),
	string( "xor" ),
	string( "complement" ),
	string( "shift_left" ),
	string( "shift_right" )
};
	
NativeFunction module_bit_fcns[] = 
{
	{do_bit_and, false},
	{do_bit_or, false},
	{do_bit_xor, false},
	{do_bit_complement, false},
	{do_bit_shift_left, false},
	{do_bit_shift_right, false},
};

const int num_of_module_bit_fcns = sizeof( module_bit_names ) / sizeof( module_bit_names[0] );


// is this name a module bit function?
bool IsModuleBitFunction( const string & name )
{
	const string* i = find( module_bit_names, module_bit_names + num_of_module_bit_fcns, name );
	if( i != module_bit_names + num_of_module_bit_fcns ) return true;
		else return false;
}

NativeFunction GetModuleBitFunction( const string & name )
{
	const string* i = find( module_bit_names, module_bit_names + num_of_module_bit_fcns, name );
	if( i == module_bit_names + num_of_module_bit_fcns )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&module_bit_names;
	int idx = l / sizeof( string );
	if( idx > num_of_module_bit_fcns )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	else
	{
		// return the function object
		return module_bit_fcns[idx];
	}
}


/////////////////////////////////////////////////////////////////////////////
// module bit functions
/////////////////////////////////////////////////////////////////////////////
void do_bit_and( Frame* f )
{
	BuiltinHelper helper( "bit", "", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( o, obj_number );

	size_t n = (size_t)(o->d);
	size_t op = (size_t)(a->d);
	size_t ret = n & op;

	helper.ReturnVal( Object( (double)ret ) );
}

void do_bit_or( Frame* f )
{
	BuiltinHelper helper( "bit", "or", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( o, obj_number );

	size_t n = (size_t)(o->d);
	size_t op = (size_t)(a->d);
	size_t ret = n | op;

	helper.ReturnVal( Object( (double)ret ) );
}

void do_bit_xor( Frame* f )
{
	BuiltinHelper helper( "bit", "xor", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( o, obj_number );

	size_t n = (size_t)(o->d);
	size_t op = (size_t)(a->d);
	size_t ret = n ^ op;

	helper.ReturnVal( Object( (double)ret ) );
}

void do_bit_complement( Frame* f )
{
	BuiltinHelper helper( "bit", "complement", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	size_t n = (size_t)(o->d);
	size_t ret = ~n;

	helper.ReturnVal( Object( (double)ret ) );
}

void do_bit_shift_left( Frame* f )
{
	BuiltinHelper helper( "bit", "shift_left", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( o, obj_number );

	size_t n = (size_t)(o->d);
	size_t op = (size_t)(a->d);
	size_t ret = n << op;

	helper.ReturnVal( Object( (double)ret ) );
}

void do_bit_shift_right( Frame* f )
{
	BuiltinHelper helper( "bit", "shift_right", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( o, obj_number );

	size_t n = (size_t)(o->d);
	size_t op = (size_t)(a->d);
	size_t ret = n >> op;

	helper.ReturnVal( Object( (double)ret ) );
}

} // namespace deva

