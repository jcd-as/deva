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

// frame.cpp
// callstack frame object for the deva language
// created by jcs, january 07, 2010 

// TODO:
// * 

#include "frame.h"
#include "executor.h"
#include <vector>


using namespace std;


namespace deva
{


Frame::~Frame()
{
	// free the local strings
	for( vector<char*>::iterator i = strings.begin(); i != strings.end(); ++i )
	{
		delete [] *i;
	}
	// dec ref the args
	int i = 0;
//	if( is_native )
//	{
//		if( native_function.is_method )
//			i++;
//	}
//	else if( function->IsMethod() )
//			i++;
	for( ; i < num_args; i++ )
	{
		DecRef( locals[i] );
	}
	// free the locals array storage
	delete [] locals;
}

// resolve symbols through the scope table
Object* Frame::FindSymbol( const char* name ) const
{
	// TODO: full look-up logic: ???
	// module names, functions(???)
	Object* o = scopes->FindSymbol( name );
	if( o )
		return o;
	// check builtins
	if( IsBuiltin( string( name ) ) )
		return GetBuiltinObjectRef( string( name ) );
	else if( IsVectorBuiltin( string( name ) ) )
		return GetVectorBuiltinObjectRef( string( name ) );
	else if( IsMapBuiltin( string( name ) ) )
		return GetMapBuiltinObjectRef( string( name ) );
	else
	{
		o = ex->FindFunction( string( name ) );
		if( o )
			return o;
	}
	return NULL;
}


} // end namespace deva

