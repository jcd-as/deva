// Copyright (c) 2011 Jmathhua C. Shepard
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

// module_math.cpp
// native math operations module for deva language, v2
// created by jcs, april 3, 2011

// TODO:
// * 

#include "module_math.h"
#include "builtins_helpers.h"
#include <cmath>


namespace deva
{


const string module_math_names[] =
{
	string( "cos" ),
	string( "sin" ),
	string( "tan" ),
	string( "acos" ),
	string( "asin" ),
	string( "atan" ),
	string( "cosh" ),
	string( "sinh" ),
	string( "tanh" ),
	string( "exp" ),
	string( "log" ),
	string( "log10" ),
	string( "abs" ),
	string( "sqrt" ),
	string( "pow" ),
	string( "modf" ),
	string( "fmod" ),
	string( "floor" ),
	string( "ceil" ),
	string( "pi" ),
	string( "radians" ),
	string( "degrees" ),
	string( "round" )
};
	
NativeFunction module_math_fcns[] = 
{
	{do_math_cos, false},
	{do_math_sin, false},
	{do_math_tan, false},
	{do_math_acos, false},
	{do_math_asin, false},
	{do_math_atan, false},
	{do_math_cosh, false},
	{do_math_sinh, false},
	{do_math_tanh, false},
	{do_math_exp, false},
	{do_math_log, false},
	{do_math_log10, false},
	{do_math_abs, false},
	{do_math_sqrt, false},
	{do_math_pow, false},
	{do_math_modf, false},
	{do_math_fmod, false},
	{do_math_floor, false},
	{do_math_ceil, false},
	{do_math_pi, false},
	{do_math_radians, false},
	{do_math_degrees, false},
	{do_math_round, false}
};

const int num_of_module_math_fcns = sizeof( module_math_names ) / sizeof( module_math_names[0] );


// is this name a module math function?
bool IsModuleMathFunction( const string & name )
{
	const string* i = find( module_math_names, module_math_names + num_of_module_math_fcns, name );
	if( i != module_math_names + num_of_module_math_fcns ) return true;
		else return false;
}

NativeFunction GetModuleMathFunction( const string & name )
{
	const string* i = find( module_math_names, module_math_names + num_of_module_math_fcns, name );
	if( i == module_math_names + num_of_module_math_fcns )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	// compute the index of the function in the look-up table(s)
	long l = (long)i;
	l -= (long)&module_math_names;
	int idx = l / sizeof( string );
	if( idx > num_of_module_math_fcns )
	{
		NativeFunction nf;
		nf.p = NULL;
		return nf;
	}
	else
	{
		// return the function object
		return module_math_fcns[idx];
	}
}


/////////////////////////////////////////////////////////////////////////////
// module math functions
/////////////////////////////////////////////////////////////////////////////
void do_math_cos( Frame* f )
{
	BuiltinHelper helper( "math", "cos", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = cos( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_sin( Frame* f )
{
	BuiltinHelper helper( "math", "sin", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = sin( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_tan( Frame* f )
{
	BuiltinHelper helper( "math", "tan", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = tan( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_acos( Frame* f )
{
	BuiltinHelper helper( "math", "acos", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = acos( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_asin( Frame* f )
{
	BuiltinHelper helper( "math", "asin", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = asin( o-> d);

	helper.ReturnVal( Object( ret ) );
}

void do_math_atan( Frame* f )
{
	BuiltinHelper helper( "math", "atan", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = atan( o-> d);

	helper.ReturnVal( Object( ret ) );
}

void do_math_cosh( Frame* f )
{
	BuiltinHelper helper( "math", "cosh", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = cosh( o-> d);

	helper.ReturnVal( Object( ret ) );
}

void do_math_sinh( Frame* f )
{
	BuiltinHelper helper( "math", "sinh", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = sinh( o-> d); 

	helper.ReturnVal( Object( ret ) );
}

void do_math_tanh( Frame* f )
{
	BuiltinHelper helper( "math", "tanh", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = tanh( o-> d);

	helper.ReturnVal( Object( ret ) );
}

void do_math_exp( Frame* f )
{
	BuiltinHelper helper( "math", "exp", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = exp( o->d );  

	helper.ReturnVal( Object( ret ) );
}

void do_math_log( Frame* f )
{
	BuiltinHelper helper( "math", "log", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = log( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_log10( Frame* f )
{
	BuiltinHelper helper( "math", "log10", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = log10( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_abs( Frame* f )
{
	BuiltinHelper helper( "math", "abs", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = abs( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_sqrt( Frame* f )
{
	BuiltinHelper helper( "math", "sqrt", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = sqrt( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_pow( Frame* f )
{
	BuiltinHelper helper( "math", "pow", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( a, obj_number );

	double ret = pow( o->d, a->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_modf( Frame* f )
{
	BuiltinHelper helper( "math", "modf", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double intpart;
	double fracpart = modf( o->d, &intpart );

	Vector* ret = CreateVector();
	ret->push_back( Object( intpart ) );
	ret->push_back( Object( fracpart ) );

	helper.ReturnVal( Object( ret ) );
}

void do_math_fmod( Frame* f )
{
	BuiltinHelper helper( "math", "fmod", f, true );
	helper.CheckNumberOfArguments( 2 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	Object* a = helper.GetLocalN( 1 );
	helper.ExpectType( a, obj_number );

	double ret = fmod( o->d, a->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_floor( Frame* f )
{
	BuiltinHelper helper( "math", "floor", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = floor( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_ceil( Frame* f )
{
	BuiltinHelper helper( "math", "ceil", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = ceil( o->d );

	helper.ReturnVal( Object( ret ) );
}

void do_math_pi( Frame* f )
{
	BuiltinHelper helper( "math", "pi", f, true );
	helper.CheckNumberOfArguments( 0 );

	helper.ReturnVal( Object( 3.14159265359 ) );
}

void do_math_radians( Frame* f )
{
	BuiltinHelper helper( "math", "radians", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = (3.14159265359 / 180.0) * o->d;

	helper.ReturnVal( Object( ret ) );
}

void do_math_degrees( Frame* f )
{
	BuiltinHelper helper( "math", "degrees", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double ret = (180.0 / 3.14159265359) * o->d;

	helper.ReturnVal( Object( ret ) );
}

void do_math_round( Frame* f )
{
	BuiltinHelper helper( "math", "round", f, true );
	helper.CheckNumberOfArguments( 1 );

	Object* o = helper.GetLocalN( 0 );
	helper.ExpectType( o, obj_number );

	double intpart;
	double fracpart = modf( o->d, &intpart );

	if( fracpart >= 0.5 )
		intpart += 1.0;

	helper.ReturnVal( Object( intpart ) );
}


} // namespace deva


