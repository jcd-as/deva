// Copyright (c) 2009 Joshua C. Shepard
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
// built-in module 'math' for the deva language 
// created by jcs, november 9, 2009 

// TODO:
// * 

#include "module_math.h"
#include <cmath>

void do_math_cos( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'cos'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'cos'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'cos' must be a number." );

	double ret = cos( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_sin( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'sin'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'sin'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'sin' must be a number." );

	double ret = sin( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_tan( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'tan'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'tan'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'tan' must be a number." );

	double ret = tan( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_acos( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'acos'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'acos'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'acos' must be a number." );

	double ret = acos( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_asin( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'asin'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'asin'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'asin' must be a number." );

	double ret = asin( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_atan( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'atan'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'atan'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'atan' must be a number." );

	double ret = atan( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_cosh( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'cosh'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'cosh'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'cosh' must be a number." );

	double ret = cosh( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_sinh( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'sinh'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'sinh'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'sinh' must be a number." );

	double ret = sinh( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_tanh( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'tanh'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'tanh'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'tanh' must be a number." );

	double ret = tanh( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_ceil( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'ceil'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'ceil'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'ceil' must be a number." );

	double ret = ceil( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_floor( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'floor'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'floor'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'floor' must be a number." );

	double ret = floor( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_abs( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'abs'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'abs'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'abs' must be a number." );

	double ret = fabs( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_exp( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'exp'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'exp'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'exp' must be a number." );

	double ret = exp( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_log( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'log'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'log'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'log' must be a number." );

	double ret = log( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_log10( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'log10'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'log10'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'log10' must be a number." );

	double ret = log10( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_sqrt( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'sqrt'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'sqrt'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'sqrt' must be a number." );

	double ret = sqrt( o->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_pow( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'pow'." );

	// base is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// exponent is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'numerator' argument in module 'math' function 'pow'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'denominator' argument in module 'math' function 'pow'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'numerator' argument to module 'math' function 'pow' must be a number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'denominator' argument to module 'math' function 'pow' must be a number." );

	double ret = pow( o->num_val, a->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_modf( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'modf'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'modf'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'modf' must be a number." );

	double intpart;
	double fracpart = modf( o->num_val, &intpart );

	DOVector* ret = new DOVector();
	ret->push_back( DevaObject( "", intpart ) );
	ret->push_back( DevaObject( "", fracpart ) );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_fmod( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'fmod'." );

	// numerator is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// denominator is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'numerator' argument in module 'math' function 'fmod'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'denominator' argument in module 'math' function 'fmod'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'numerator' argument to module 'math' function 'fmod' must be a number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'denominator' argument to module 'math' function 'fmod' must be a number." );

	double ret = fmod( o->num_val, a->num_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_pi( Executor* ex )
{
	if( Executor::args_on_stack != 0 )
		throw DevaRuntimeException( "Module 'math' function 'pi' takes no arguments." );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", 3.14159265359 ) );
}

void do_math_radians( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'radians'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'radians'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'radians' must be a number." );

	double ret = (3.14159265359 / 180.0) * o->num_val;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_degrees( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'degrees'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'degrees'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'degrees' must be a number." );

	double ret = (180 / 3.14159265359) * o->num_val;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", ret ) );
}

void do_math_round( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'math' function 'round'." );

	// number is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'math' function 'round'" );
	}
	if( !o )
		o = &num;

	// check type
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'math' function 'round' must be a number." );

	double intpart;
	double fracpart = modf( o->num_val, &intpart );

	if( fracpart >= 0.5 )
		intpart += 1.0;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", intpart ) );
}

void AddMathModule( Executor* ex )
{
	map<string, builtin_fcn> fcns = map<string, builtin_fcn>();
	fcns.insert( make_pair( string( "cos@math" ), do_math_cos ) );
	fcns.insert( make_pair( string( "sin@math" ), do_math_sin ) );
	fcns.insert( make_pair( string( "tan@math" ), do_math_tan ) );
	fcns.insert( make_pair( string( "acos@math" ), do_math_acos ) );
	fcns.insert( make_pair( string( "asin@math" ), do_math_asin ) );
	fcns.insert( make_pair( string( "atan@math" ), do_math_atan ) );
	fcns.insert( make_pair( string( "cosh@math" ), do_math_cosh ) );
	fcns.insert( make_pair( string( "sinh@math" ), do_math_sinh ) );
	fcns.insert( make_pair( string( "tanh@math" ), do_math_tanh ) );
	fcns.insert( make_pair( string( "exp@math" ), do_math_exp ) );
	fcns.insert( make_pair( string( "log@math" ), do_math_log ) );
	fcns.insert( make_pair( string( "log10@math" ), do_math_log10 ) );
	fcns.insert( make_pair( string( "abs@math" ), do_math_abs ) );
	fcns.insert( make_pair( string( "sqrt@math" ), do_math_sqrt ) );
	fcns.insert( make_pair( string( "pow@math" ), do_math_pow ) );
	fcns.insert( make_pair( string( "modf@math" ), do_math_modf) );
	fcns.insert( make_pair( string( "fmod@math" ), do_math_fmod ) );
	fcns.insert( make_pair( string( "floor@math" ), do_math_floor ) );
	fcns.insert( make_pair( string( "ceil@math" ), do_math_ceil ) );
	fcns.insert( make_pair( string( "pi@math" ), do_math_pi ) );
	fcns.insert( make_pair( string( "radians@math" ), do_math_radians ) );
	fcns.insert( make_pair( string( "degrees@math" ), do_math_degrees ) );
	fcns.insert( make_pair( string( "round@math" ), do_math_round ) );
	ex->ImportBuiltinModuleFunctions( string( "math" ), fcns );
}
