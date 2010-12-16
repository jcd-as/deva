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

#include <semantic_walker.h>
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
void Semantics::PushScope( char* fcn_name /*= NULL*/ )
{
	static int counter = 0;
	static char scopeId[32] = {0};
	// if we're entering a function body, create a function scope
	if( fcn_name )
	{
		if( in_class )
		{
			string method_name;
			method_name.reserve( strlen( fcn_name + 1 ) );
		   	method_name = "@";
		   	method_name += fcn_name;
			current_scope = new FunctionScope( method_name, semantics->current_scope, true );
		}
		else
			current_scope = new FunctionScope( fcn_name, semantics->current_scope );
	}
	// otherwise create a local scope
	else
	{
		sprintf( scopeId, "%i", counter );
		semantics->current_scope = new LocalScope( scopeId, semantics->current_scope );
	}
	semantics->scopes.push_back( semantics->current_scope );
	counter++;
}

// 'pop' scope on exiting block
void Semantics::PopScope()
{
	semantics->current_scope = semantics->current_scope->EnclosingScope();
}

// define a variable in the current scope
void Semantics::DefineVar( char* name, int line, VariableModifier mod /*= mod_none*/ )
{
	Symbol *sym = new Symbol( name, sym_variable, mod );
	if( !semantics->current_scope->Define( sym ) )
	{
		delete sym;
		throw DevaSemanticException( str( boost::format( "Symbol '%1%' already defined." ) % name).c_str(), line );
	}
}

// resolve a variable, in the current scope
void Semantics::ResolveVar( char* name, int line )
{
	// look up the variable in the current scope
	if( show_warnings && !semantics->current_scope->Resolve( name, sym_end ) )
	{
		emit_warning( (char*)str(boost::format( "Symbol '%1%' not defined." ) % name).c_str(), line );
		// add it as an 'undeclared' var, it's possible that it was emitted by
		// an eval() call or such and will exist at run-time
		DefineVar( name, line );
	}
}

// define a function in the current scope
void Semantics::DefineFun( char* name, int line )
{
	// TODO: functions should OVERRIDE existing fcns with the same name...
	// does it matter? we're only tracking the name & type anyway...
	Symbol *sym = new Symbol( name, sym_function, mod_none );
	if( !semantics->current_scope->Define( sym ) )
	{
		delete sym;
	}
}

// resolve a function, in the current scope
void Semantics::ResolveFun( char* name, int line )
{
	// look up the function in the current scope
	if( show_warnings && !semantics->current_scope->Resolve( name , sym_function ) )
		emit_warning( (char*)str(boost::format( "Function '%1%' not defined." ) % name).c_str(), line );
}


// node validation //////////////////////////////////////////////////////////

// ensure no arg names are duplicated
void Semantics::AddArg( char* arg, int line )
{
	string sa( arg );
	if( semantics->arg_names.count( sa ) != 0 )
		throw DevaSemanticException( boost::format( "Function argument names must be unique: '%1%' multiply defined." ) % arg, line ); 
	else
		semantics->arg_names.insert( sa );

	// add arg to the symbol table
	DefineVar( arg, line, mod_arg );
}

// validate lhs of assignment
void Semantics::CheckLhsForAssign( pANTLR3_BASE_TREE lhs )
{
	unsigned int type = lhs->getType( lhs );

	// lhs must be an ID, Key expression or '.'
	if( type != ID && type != Key && type != DOT_OP )
		throw DevaSemanticException( "Invalid l-value on left-hand side of assignment.", lhs->getLine(lhs) );

	// if lhs is an ID, check with the symbol table and ensure it is defined
	if( type == ID )
	{
		ResolveVar( (char*)lhs->getText(lhs)->chars, lhs->getLine(lhs) );
	}
}

// validate relational expression
void Semantics::CheckRelationalOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs )
{
	// both sides must be ID, null, number, string, Negate, Key, DOT_OP, Call
	// or math op
	unsigned int lhs_type = lhs->getType( lhs );
	unsigned int rhs_type = rhs->getType( rhs );

	if( lhs_type != ID 
		&& lhs_type != Negate 
		&& lhs_type != ADD_OP
		&& lhs_type != SUB_OP
		&& lhs_type != MUL_OP
		&& lhs_type != DIV_OP
		&& lhs_type != MOD_OP
		&& lhs_type != Key 
		&& lhs_type != DOT_OP 
		&& lhs_type != Call
		&& lhs_type != NULLVAL 
		&& lhs_type != BOOL 
		&& lhs_type != NUMBER 
		&& lhs_type != STRING )
		throw DevaSemanticException( "Invalid left-hand side of relational operator.", lhs->getLine(lhs) );
	else if( rhs_type != ID 
		&& rhs_type != Negate 
		&& rhs_type != ADD_OP
		&& rhs_type != SUB_OP
		&& rhs_type != MUL_OP
		&& rhs_type != DIV_OP
		&& rhs_type != MOD_OP
		&& rhs_type != Key 
		&& rhs_type != DOT_OP 
		&& rhs_type != Call
		&& rhs_type != NULLVAL 
		&& rhs_type != BOOL 
		&& rhs_type != NUMBER 
		&& rhs_type != STRING )
		throw DevaSemanticException( "Invalid right-hand side of relational operator.", rhs->getLine(rhs) );
}

// validate equality expression
void Semantics::CheckEqualityOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs )
{
	// no-op: parser should have rejected all disallowed expressions
}


// validate logical expression
void Semantics::CheckLogicalOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs )
{
	// no-op: parser should have rejected all disallowed expressions except
	// strings
	unsigned int lhs_type = lhs->getType( lhs );
	unsigned int rhs_type = rhs->getType( rhs );

	if( lhs_type == STRING || rhs_type == STRING )
		throw DevaSemanticException( "Cannot use a string in a logical expression.", lhs->getLine(lhs) );
}

// validate mathematical expression
void Semantics::CheckMathOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs )
{
	// both sides must be ID, number, negate op, Key, DOT_OP, Call
	// or another math op
	unsigned int lhs_type = lhs->getType( lhs );
	unsigned int rhs_type = rhs->getType( rhs );

	if( lhs_type != Negate
		&& lhs_type != ADD_OP
		&& lhs_type != SUB_OP
		&& lhs_type != MUL_OP
		&& lhs_type != DIV_OP
		&& lhs_type != MOD_OP
		&& lhs_type != ID 
		&& lhs_type != Negate 
		&& lhs_type != Key 
		&& lhs_type != DOT_OP 
		&& lhs_type != Call
		&& lhs_type != NUMBER )
		throw DevaSemanticException( "Invalid left-hand side of arithmetic operator.", lhs->getLine(lhs) );
	else if( rhs_type != Negate
		&& rhs_type != ADD_OP
		&& rhs_type != SUB_OP
		&& rhs_type != MUL_OP
		&& rhs_type != DIV_OP
		&& rhs_type != MOD_OP
		&& rhs_type != ID 
		&& rhs_type != Negate 
		&& rhs_type != Key 
		&& rhs_type != DOT_OP 
		&& rhs_type != Call
		&& rhs_type != NUMBER )
		throw DevaSemanticException( "Invalid right-hand side of arithmetic operator.", rhs->getLine(rhs) );
}

void Semantics::CheckAddOp( pANTLR3_BASE_TREE lhs, pANTLR3_BASE_TREE rhs )
{
	// both sides must be ID, number, unary op, Key, DOT_OP or Call
	// OR for add-op only, a string
	// both sides must be ID, number, negate op, Key, DOT_OP or Call
	unsigned int lhs_type = lhs->getType( lhs );
	unsigned int rhs_type = rhs->getType( rhs );

	if( lhs_type != Negate
		&& lhs_type != ADD_OP
		&& lhs_type != SUB_OP
		&& lhs_type != MUL_OP
		&& lhs_type != DIV_OP
		&& lhs_type != MOD_OP
		&& lhs_type != ID 
		&& lhs_type != Negate 
		&& lhs_type != Key 
		&& lhs_type != DOT_OP 
		&& lhs_type != Call
		&& lhs_type != NUMBER
	 	&& lhs_type != STRING )
		throw DevaSemanticException( "Invalid left-hand side of addition operator.", lhs->getLine(lhs) );
	else if( rhs_type != Negate
		&& rhs_type != ADD_OP
		&& rhs_type != SUB_OP
		&& rhs_type != MUL_OP
		&& rhs_type != DIV_OP
		&& rhs_type != MOD_OP
		&& rhs_type != ID 
		&& rhs_type != Negate 
		&& rhs_type != Key 
		&& rhs_type != DOT_OP 
		&& rhs_type != Call
		&& rhs_type != NUMBER
	 	&& rhs_type != STRING )
		throw DevaSemanticException( "Invalid right-hand side of addition operator.", rhs->getLine(rhs) );
}

// validate negate expression
void Semantics::CheckNegateOp( pANTLR3_BASE_TREE in )
{
	// input must be ID, number, negate op, Key, DOT_OP or Call
	unsigned int type = in->getType( in );

	if( type != Negate
		&& type != ID 
		&& type != Negate 
		&& type != Key 
		&& type != DOT_OP 
		&& type != Call
		&& type != NUMBER )
		throw DevaSemanticException( "Invalid operand for negate ('-') operator.", in->getLine(in) );
}

// validate not expression
void Semantics::CheckNotOp( pANTLR3_BASE_TREE in )
{
	// input must be a relational/equality exp, logical exp, ID, bool, number, not op, Key, DOT_OP or Call
	// (coercible to a boolean)
	unsigned int type = in->getType( in );

	if( type != NOT_OP
		&& type != GT_OP
		&& type != LT_OP
		&& type != GT_EQ_OP
		&& type != LT_EQ_OP
		&& type != EQ_OP
		&& type != NOT_EQ_OP
		&& type != AND_OP
		&& type != OR_OP
		&& type != ADD_OP
		&& type != SUB_OP
		&& type != MUL_OP
		&& type != DIV_OP
		&& type != MOD_OP
		&& type != ID 
		&& type != Negate 
		&& type != Key 
		&& type != DOT_OP 
		&& type != Call
		&& type != NUMBER
		&& type != BOOL )
		throw DevaSemanticException( "Invalid operand for logical not ('!') operator.", in->getLine(in) );
}

// validate if or while conditional
void Semantics::CheckConditional( pANTLR3_BASE_TREE condition )
{
	// input must be a relational/equality/math exp, logical exp, ID, bool, number, not op, Key, DOT_OP or Call
	// (coercible to a boolean)
	unsigned int type = condition->getType( condition );

	if( type != NOT_OP
		&& type != GT_OP
		&& type != LT_OP
		&& type != GT_EQ_OP
		&& type != LT_EQ_OP
		&& type != EQ_OP
		&& type != NOT_EQ_OP
		&& type != AND_OP
		&& type != OR_OP
		&& type != ADD_OP
		&& type != SUB_OP
		&& type != MUL_OP
		&& type != DIV_OP
		&& type != MOD_OP
		&& type != ID 
		&& type != Negate 
		&& type != Key 
		&& type != DOT_OP 
		&& type != Call
		&& type != NUMBER
		&& type != BOOL )
		throw DevaSemanticException( "Invalid conditional.", condition->getLine(condition) );
}

// validate key expressions (slices)
void Semantics::CheckKeyExp( pANTLR3_BASE_TREE idx1, pANTLR3_BASE_TREE idx2 /*= NULL*/, pANTLR3_BASE_TREE idx3 /*= NULL*/ )
{
	// nothing to validate for non-slices
	if( idx2 == NULL )
		return;

	// idx1 and idx2 can be END_OP ('$') or anything resulting in a number
	unsigned int idx1_type = idx1->getType( idx1 );
	if( idx1_type != END_OP
		&& idx1_type != Negate
		&& idx1_type != ADD_OP
		&& idx1_type != SUB_OP
		&& idx1_type != MUL_OP
		&& idx1_type != DIV_OP
		&& idx1_type != MOD_OP
		&& idx1_type != ID 
		&& idx1_type != Negate 
		&& idx1_type != Key 
		&& idx1_type != DOT_OP 
		&& idx1_type != Call
		&& idx1_type != NUMBER )
		throw DevaSemanticException( "Invalid first index in slice.", idx1->getLine(idx1) );

	unsigned int idx2_type = idx2->getType( idx2 );
	if( idx2_type != END_OP
		&& idx2_type != Negate
		&& idx2_type != ADD_OP
		&& idx2_type != SUB_OP
		&& idx2_type != MUL_OP
		&& idx2_type != DIV_OP
		&& idx2_type != MOD_OP
		&& idx2_type != ID 
		&& idx2_type != Negate 
		&& idx2_type != Key 
		&& idx2_type != DOT_OP 
		&& idx2_type != Call
		&& idx2_type != NUMBER )
		throw DevaSemanticException( "Invalid second index in slice.", idx2->getLine(idx2) );

	// idx3 must be something resulting in a number
	if( idx3 )
	{
		unsigned int idx3_type = idx3->getType( idx3 );
		if( idx3_type != Negate
			&& idx3_type != ADD_OP
			&& idx3_type != SUB_OP
			&& idx3_type != MUL_OP
			&& idx3_type != DIV_OP
			&& idx3_type != MOD_OP
			&& idx3_type != ID 
			&& idx3_type != Negate 
			&& idx3_type != Key 
			&& idx3_type != DOT_OP 
			&& idx3_type != Call
			&& idx3_type != NUMBER )
		throw DevaSemanticException( "Invalid third index in slice.", idx3->getLine(idx3) );
	}
}

// validate break/continue
void Semantics::CheckBreakContinue( pANTLR3_BASE_TREE node )
{
	// invalid if we're not inside a loop
	if( in_loop == 0 )
		throw DevaSemanticException( "Invalid 'break' or 'continue': must be inside a loop.", node->getLine(node) );
}

// check statement for no effect (e.g. 'a;')
void Semantics::CheckForNoEffect( pANTLR3_BASE_TREE node )
{
	// if statement (node) isn't an assignment, augmented (math) assignment, declaration (const/local/extern) 
	// or call, it has no effect
	unsigned int type = node->getType( node );
	if( type != ASSIGN_OP 
		&& type != ADD_EQ_OP
		&& type != SUB_EQ_OP
		&& type != MUL_EQ_OP
		&& type != DIV_EQ_OP
		&& type != MOD_EQ_OP
		&& type != Call
		&& type != Const
		&& type != Local
		&& type != Extern
	  )
		throw DevaSemanticException( "Invalid statement: statement has no effect.", node->getLine( node ) );
}		

