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

// builtins_helpers.cpp
// helper fcns for builtin functions/methods for the deva language
// created by jcs, january 3, 2011

// TODO:
// * 

#include "builtins_helpers.h"

namespace deva
{


void BuiltinHelper::CheckNumberOfArguments( int num_args_expected )
{
	int args_passed = frame->NumArgsPassed();
	if( args_passed > num_args_expected )
		throw RuntimeException( boost::format( "Too many arguments passed to %1% %2% %3%." ) % type % name % (is_method ? "method" : "builtin") );
	else if( args_passed < num_args_expected )
		throw RuntimeException( boost::format( "Too few arguments passed to %1% %2% %3%." ) % type % name % (is_method ? "method" : "builtin") );
}

void BuiltinHelper::CheckNumberOfArguments( int min_num_args, int max_num_args )
{
	int args_passed = frame->NumArgsPassed();
	if( args_passed > max_num_args )
		throw RuntimeException( boost::format( "Too many arguments passed to %1% %2% %3%." ) % type % name % (is_method ? "method" : "builtin") );
	else if( args_passed < min_num_args )
		throw RuntimeException( boost::format( "Too few arguments passed to %1% %2% %3%." ) % type % name % (is_method ? "method" : "builtin") );
}

void BuiltinHelper::ExpectType( Object* obj, ObjectType t )
{
	if( obj->type != t )
		throw ICE( boost::format( "%1% expected in %2% %3% %4%." ) % object_type_names[t] % type % (is_method ? "method" : "builtin") % name );
}

Object* BuiltinHelper::GetLocalN( int local_num )
{
	// get the arg from the frame
	Object* o = frame->GetLocalRef( local_num );
	Object* po = NULL;

	// if it's a symbol name, look up the symbol
	if( o->type == obj_symbol_name )
	{
		po = frame->FindSymbol( o->s );
		if( !po )
			throw RuntimeException( boost::format( "Undefined symbol '%1%'." ) % o->s );
	}
	else
		po = o;
	return po;
}


} // end namespace deva

