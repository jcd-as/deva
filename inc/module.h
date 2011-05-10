// Copyright (c) 2011 Joshua C. Shepard
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

// module.h
// module object for deva language, v2
// created by jcs, march 31, 2011

// TODO:
// * 

#ifndef __MODULE_H__
#define __MODULE_H__

#include "scopetable.h"

namespace deva
{

class Code;
class Frame;

struct Module
{
	const Code* code;
	Scope* scope;
	Frame* frame;

	Module( const Code* c, Scope* s, Frame* f ) : code( c ), scope( s ), frame( f ) {}
	inline void DeleteScopeData() {  scope->DeleteData(); }
	inline void DeleteScope() { delete scope; }
	inline void DeleteFrame() { delete frame; }
};

//typedef NativeFunction (*module_fcn_finder)(const string&);

struct NativeModule
{
	string name;
//	module_fcn_finder fcn;
	// ptr to array of functions
	NativeFunction* functions;
	// ptr to array of function names
	const string* function_names;
	int num_functions;

	NativeModule( const char* const n, NativeFunction* fcns, const string* fcn_names, int num ) : name( n ), functions( fcns ), function_names( fcn_names ), num_functions( num ) {}

	NativeFunction GetFunction( const string & name )
	{
		const string* i = find( function_names, function_names + num_functions, name );
		if( i == function_names + num_functions )
		{
			NativeFunction nf;
			nf.p = NULL;
			return nf;
		}
		// compute the index of the function in the look-up table(s)
		long l = (long)i;
		l -= (long)function_names;
		int idx = l / sizeof( string );
		if( idx > num_functions )
		{
			NativeFunction nf;
			nf.p = NULL;
			return nf;
		}
		else
		{
			// return the function object
			return functions[idx];
		}
	}
};


} // namespace std

#endif // __MODULE_H__

