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

// semantics.h
// semantic global object/functions for the deva language
// created by jcs, december 09, 2010 

// TODO:
// * 

#ifndef __SEMANTICS_H__
#define __SEMANTICS_H__


#include <set>
#include <vector>

#include "symbol.h"
#include "scope.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////
struct Semantics
{
	// scopes
	Scope* global_scope;
	Scope* current_scope;
	vector<Scope*> scopes; // list of all scopes

	// functions and arguments
	int deva_in_function;
	char* deva_function_name;
	set<string> deva_arg_names;

	// classes
	//char* deva_class_name;

	// variables


	// destructor
	~Semantics()
	{
		for( vector<Scope*>::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			delete *i;
			*i = NULL;
		}
	}
};

extern Semantics* semantics;


/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////

// TODO: move this function to error reporting header
void devaDisplayRecognitionError( pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames );


// scope & symbol table handling ////////////////////////////////////////////

// 'push' new scope on entering block
void devaPushScope( char* name );
// 'pop' scope on exiting block
void devaPopScope();

// define a variable in the current scope
void devaDefineVar( char* name, int line, VariableModifier mod = mod_none );

// resolve a variable, in the current scope
void devaResolveVar( char* name, int line );


// node validation //////////////////////////////////////////////////////////

// validate function arguments
void devaAddArg( char* arg, int line );


#endif // __SEMANTICS_H__
