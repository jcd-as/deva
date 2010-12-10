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

// semantics.cpp
// semantic checking functions for the deva language
// created by jcs, december 10, 2010 

// TODO:
// * 

#include <antlr3.h>
#include <set>
#include <boost/format.hpp>

#include "semantics.h"
#include "exceptions.h"

/////////////////////////////////////////////////////////////////////////////
// semantic checking functions and globals
/////////////////////////////////////////////////////////////////////////////
Semantics* semantics = NULL;

// scope & symbol table handling ////////////////////////////////////////////

// 'push' new scope on entering block
void devaPushScope( char* fcn_name )
{
	// if we're entering a function body, create a function scope
	if( semantics->deva_in_function > 0 )
		semantics->current_scope = new FunctionScope( fcn_name ? fcn_name : "", semantics->current_scope );
	// otherwise create a local scope
	else
		semantics->current_scope = new LocalScope( "", semantics->current_scope );
	semantics->scopes.push_back( semantics->current_scope );
}

// 'pop' scope on exiting block
void devaPopScope()
{
	semantics->current_scope = semantics->current_scope->EnclosingScope();
}

// define a variable in the current scope
void devaDefineVar( char* name, int line, VariableModifier mod /*= mod_none*/ )
{
	Symbol *sym = new Symbol( sym_variable, mod, name );
	if( !semantics->current_scope->Define( sym ) )
	{
		delete sym;
		throw DevaSemanticException( str( boost::format( "Symbol '%1%' already defined." ) % name).c_str(), line );
	}
}

// resolve a variable, in the current scope
void devaResolveVar( char* name, int line )
{
	// look up the variable in the current scope
	if( !semantics->current_scope->Resolve( name ) )
//		throw DevaSemanticException( str( boost::format( "Symbol '%1%' not defined." ) % name).c_str(), line );
		throw DevaSemanticException( boost::format( "Symbol '%1%' not defined." ) % name, line );
}


// node validation //////////////////////////////////////////////////////////

//set<string> deva_arg_names;
// ensure no arg names are duplicated
void devaAddArg( char* arg, int line )
{
	string sa( arg );
	if( semantics->deva_arg_names.count( sa ) != 0 )
		throw DevaSemanticException( boost::format( "Function argument names must be unique: '%1%' multiply defined." ) % arg, line ); 
	else
		semantics->deva_arg_names.insert( sa );
}

