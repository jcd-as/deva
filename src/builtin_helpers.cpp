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

// builtin_helpers.cpp
// helper functions for writing built-in functions/methods
// created by jcs, january 10, 2010 

// TODO:
// * 

#include "builtin_helpers.h"

// get the 'this' object off the top of the stack
DevaObject get_this( Executor *ex, const char* fcn, SymbolType type, bool accept_class_or_instance /*= false*/ )
{
	// get the arg from the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// if it is a variable, look it up
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( boost::format( "Symbol not found for parameter in call to function '%1%'" ) % fcn );
	}
	if( !o )
		o = &obj;

	// check the type
	if( o->Type() != type )
	{
		if( type == sym_map && accept_class_or_instance )
		{
			if( o->Type() != sym_class && o->Type() != sym_instance )
				throw DevaRuntimeException( boost::format( "map, class or instance expected in '%1%' method '%2%'." ) % SymbolTypeNames[type] % fcn );
		}
		else
			throw DevaRuntimeException( boost::format( "%1% expected in '%2%' method '%3%'." ) % SymbolTypeNames[type] % SymbolTypeNames[type] % fcn );
	}

	return *o;
}

// get a fcn argument off the top of the stack
DevaObject get_arg( Executor* ex, const char* fcn, const char* arg )
{
	// get the arg from the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// if it is a variable, look it up
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( boost::format( "Symbol not found for '%1% parameter in call to function '%2%'" ) % arg % fcn );
	}
	if( !o )
		return obj;
	else
		return *o;
}

// get a fcn argument off the top of the stack and verify it is of the correct
// type
DevaObject get_arg_of_type( Executor* ex, const char* fcn, const char* arg, SymbolType type )
{
	// get the arg from the top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();

	// if it is a variable, look it up
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( boost::format( "Symbol not found for '%1%' parameter in call to function '%2%'" ) % arg % fcn );
	}
	if( o )
		obj = *o;

	// check the type
	if( obj.Type() != type )
		throw DevaRuntimeException( boost::format( "%1% expected for argument '%2%' in function '%3%'." ) % SymbolTypeNames[type] % arg % fcn );

	return obj;
}
