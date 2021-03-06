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


#include "symbol.h"
#include "scope.h"
#include "error.h"
#include "object.h"
#include "builtins.h"
#include "util.h"

#include <set>
#include <map>
#include <vector>

using namespace std;
using namespace deva;

namespace deva_compile
{

struct Semantics
{
	// ignore undefined variables (for executing code in an existing
	// environment, ala eval() or an interactive shell)
	bool ignore_undefined_vars;

	// warnings on?
//	bool show_warnings;

	// scopes
	Scope* global_scope;
	Scope* current_scope;
	vector<Scope*> scopes; // list of all scopes

	// functions and arguments
	set<string> arg_names;
	vector<Object> default_arg_values;
	int first_default_arg;

	// constants (numbers, strings & symbol names)
	set<Object> constants;

	// module names
	set<char*> module_names;

	// classes
	bool in_class;

	// function calls
	bool making_call;
	int in_fcn;

	// map key
	bool in_map_key;

	// setting/getting a prop on a map/class/instance
	bool rhs_of_dot;

	// loop tracking
	// vector of 'in-loop' counters, one for each function scope
	// if the back item is non-zero then we are inside a loop construct
	vector<int> in_for_loop;
	vector<int> in_while_loop;

	// constructor
	Semantics( bool ignore_undef = NULL ) : ignore_undefined_vars( ignore_undef ), //show_warnings( warn ),
	   	global_scope( NULL ), current_scope( NULL ),
		first_default_arg( -1 ),
		in_class( false ),
		making_call( false ),
		in_fcn( 0 ),
		in_map_key( false ),
		rhs_of_dot( false )
	{
		// setup the global scope (a fcn scope called "@main")
		PushScope( (char*)"@main" );
		global_scope = current_scope;
		in_for_loop.push_back( 0 );
		in_while_loop.push_back( 0 );
	}
	// destructor
	~Semantics()
	{
		for( vector<Scope*>::iterator i = scopes.begin(); i != scopes.end(); ++i )
		{
			delete *i;
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// methods
	/////////////////////////////////////////////////////////////////////////////
	
	void DumpSymbolTable();

	// loop tracking ////////////////////////////////////////////////////////////
	bool InForLoop() { return in_for_loop.back() > 0; }
	bool InWhileLoop() { return in_while_loop.back() > 0; }
	void IncForLoopCounter(){ int i = in_for_loop.back(); in_for_loop.pop_back(); i++; in_for_loop.push_back( i ); }
	void DecForLoopCounter(){ int i = in_for_loop.back(); in_for_loop.pop_back(); i--; in_for_loop.push_back( i ); }
	void IncWhileLoopCounter(){ int i = in_while_loop.back(); in_while_loop.pop_back(); i++; in_while_loop.push_back( i ); }
	void DecWhileLoopCounter(){ int i = in_while_loop.back(); in_while_loop.pop_back(); i--; in_while_loop.push_back( i ); }

	// scope & symbol table handling ////////////////////////////////////////////

	// 'push' new scope on entering block
	void PushScope( char* name = NULL, char* class_name = NULL );
	// 'pop' scope on exiting block
	void PopScope();

	// define a variable in the current scope
	void DefineVar( char* name, int line, VariableModifier mod = mod_none, SymbolType type = sym_variable );

	// resolve a variable, in the current scope
	void ResolveVar( char* name, int line );

	// define a function in the current scope
	void DefineFun( char* name, char* classname, int line );
	
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

	// validate lhs of augmented assignment ('+= etc)
	void CheckLhsForAugmentedAssign( pANTLR3_BASE_TREE lhs ); 

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

	// validate return
	void CheckReturn( pANTLR3_BASE_TREE node );

	// check statement for no effect (e.g. 'a;')
	void CheckForNoEffect( pANTLR3_BASE_TREE node );

	// check default arg val ID to make sure it's a const
	void CheckDefaultArgVal( char* n, int line );

	// add a default arg val (for the previous default arg)
	void DefaultArgVal( pANTLR3_BASE_TREE node, bool negate = false );

	// check the fcn default args' placement and reset fcn tracking vars
	void CheckAndResetFun( int line );

	// compose and define the module name
	void CheckImport( pANTLR3_BASE_TREE node );
};


// global semantics object
extern Semantics* semantics;

} // namespace deva_compile

#endif // __SEMANTICS_H__
