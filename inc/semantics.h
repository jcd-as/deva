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
#include <map>
#include <vector>

#include "symbol.h"
#include "scope.h"
#include "error.h"
#include "object.h"

using namespace std;

struct Semantics
{
	// warnings on?
	bool show_warnings;

	// scopes
	Scope* global_scope;
	Scope* current_scope;
	vector<Scope*> scopes; // list of all scopes

	// functions and arguments
	set<string> arg_names;
	vector<DevaObject> default_arg_values;
	int first_default_arg;

	// constants
	set<DevaObject> constants;

	// 'global' variables (extern, undeclared)
	set<string> names;

	// function calls
	bool making_call;

	// classes
	//char* class_name;
	bool in_class;

	// loop tracking
	int in_loop;

	// constructor
	Semantics( bool warn ) : show_warnings( warn ),
	   	global_scope( NULL ), current_scope( NULL ),
		first_default_arg( -1 ),
		making_call( false ),
		in_class( false ),
		in_loop( false )
	{
		// setup the global scope (a fcn scope called "@main")
		PushScope( (char*)"@main" );
//		current_scope = new FunctionScope( "@main", current_scope );
		global_scope = current_scope;
//		scopes.push_back( current_scope );
	}
	// destructor
	~Semantics()
	{
		for( vector<Scope*>::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			delete *i;
			*i = NULL;
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// methods
	/////////////////////////////////////////////////////////////////////////////
	
	// scope & symbol table handling ////////////////////////////////////////////

	// 'push' new scope on entering block
	void PushScope( char* name = NULL );
	// 'pop' scope on exiting block
	void PopScope();

	// define a variable in the current scope
	void DefineVar( char* name, int line, VariableModifier mod = mod_none );

	// resolve a variable, in the current scope
	void ResolveVar( char* name, int line );

	// define a function in the current scope
	void DefineFun( char* name, int line );
	
	// resolve a function, in the current scope
	void ResolveFun( char* name, int line );

	// node validation //////////////////////////////////////////////////////////

	// validate function arguments
	void AddArg( char* arg, int line );

	// add number constant
	void AddNumber( double arg );

	// add string constant
	void AddString( char* arg );

	// validate lhs of assignment
	void CheckLhsForAssign( pANTLR3_BASE_TREE lhs ); 

	// validate relational expression
	void CheckRelationalOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs );

	// validate relational expression
	void CheckEqualityOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs );

	// validate logical expression
	void CheckLogicalOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs );

	// validate mathematical expression (except add)
	void CheckMathOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs );

	// validate mathematical add expression
	void CheckAddOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs );

	// validate negate expression
	void CheckNegateOp( pANTLR3_BASE_TREE lhs );

	// validate not expression
	void CheckNotOp( pANTLR3_BASE_TREE lhs );

	// validate if or while conditional
	void CheckConditional( pANTLR3_BASE_TREE condition );

	// validate key expressions (slices)
	void CheckKeyExp( pANTLR3_BASE_TREE idx1, pANTLR3_BASE_TREE idx2 = NULL, pANTLR3_BASE_TREE idx3 = NULL );

	// validate break/continue
	void CheckBreakContinue( pANTLR3_BASE_TREE node );

	// check statement for no effect (e.g. 'a;')
	void CheckForNoEffect( pANTLR3_BASE_TREE node );

	// check default arg val ID to make sure it's a const
	void CheckDefaultArgVal( char* n, int line );

	// add a default arg val (for the previous default arg)
	void DefaultArgVal( pANTLR3_BASE_TREE node );

	// check the fcn default args' placement and reset fcn tracking vars
	void CheckAndResetFcn( int line );
};


// global semantics object
extern Semantics* semantics;


#endif // __SEMANTICS_H__
