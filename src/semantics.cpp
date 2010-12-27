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


#include <semantic_walker.h>
#include <set>
#include <boost/format.hpp>

#include "semantics.h"
#include "exceptions.h"
#include "util.h"

namespace deva_compile
{

/////////////////////////////////////////////////////////////////////////////
// semantic checking functions and globals
/////////////////////////////////////////////////////////////////////////////
Semantics* semantics = NULL;


// scope & symbol table handling ////////////////////////////////////////////

// 'push' new scope on entering block
void Semantics::PushScope( char* fcn_name /*= NULL*/, char* class_name /*= NULL*/ )
{
	static int counter = 0;
	static char scopeId[32] = {0};
	// if we're entering a function body, create a function scope
	if( fcn_name )
	{
		if( class_name )
		{
			string method_name;
			method_name.reserve( strlen( fcn_name ) + strlen( class_name ) + 1 );
		   	method_name = fcn_name;
		   	method_name += "@";
			method_name += class_name;
			current_scope = new FunctionScope( method_name, current_scope, true );
		}
		else
			current_scope = new FunctionScope( fcn_name, current_scope );
	}
	// otherwise create a local scope
	else
	{
		sprintf( scopeId, "%i", counter );
		current_scope = new LocalScope( scopeId, current_scope );
	}
	scopes.push_back( current_scope );
	counter++;
}

// 'pop' scope on exiting block
void Semantics::PopScope()
{
	current_scope = current_scope->EnclosingScope();
}

// define a variable in the current scope
void Semantics::DefineVar( char* name, int line, VariableModifier mod /*= mod_none*/ )
{
	Symbol *sym = new Symbol( name, sym_variable, mod );
	if( !current_scope->Define( sym ) )
	{
		delete sym;
		throw DevaSemanticException( str( boost::format( "Symbol '%1%' already defined." ) % name).c_str(), line );
	}
	// add the var name to the constant pool
	constants.insert( DevaObject( name ) );
}

// resolve a variable, in the current scope
void Semantics::ResolveVar( char* name, int line )
{
	// look up the variable in the current scope
	if( !current_scope->Resolve( name, sym_end ) )
	{
		if( show_warnings )
			emit_warning( (char*)str(boost::format( "Symbol '%1%' not defined." ) % name).c_str(), line );
		// add it as an 'undeclared' var, it's possible that it was emitted by
		// an eval() call or such and will exist at run-time
		DefineVar( name, line );
	}
}

// define a function in the current scope
void Semantics::DefineFun( char* name, char* classname, int line )
{
	char* fcn_name;
	if( classname )
	{
		int namelen = strlen( name );
		int len = namelen + strlen( classname );
		fcn_name = new char[len + 2]; // room for name, '@', classname and null-terminator
		memset( fcn_name, 0, len + 2 ); // zero fill
		strcpy( fcn_name, name );
		fcn_name[namelen] = '@';
		strcat( fcn_name, classname );
	}
	else
	{
		fcn_name = name;
	}
	Symbol *sym = new Symbol( fcn_name, sym_function, mod_none );
	if( !current_scope->Define( sym ) )
	{
		delete sym;
	}
	// add the name to the constant pool (the 'short' name, not the method name)
	constants.insert( DevaObject( name ) );
}

// resolve a function, in the current scope
void Semantics::ResolveFun( char* name, int line )
{
	// look up the function in the current scope
	if( !current_scope->Resolve( name , sym_function ) )
	{
		if( show_warnings )
			emit_warning( (char*)str(boost::format( "Function '%1%' not defined." ) % name).c_str(), line );
		// add it to the constant pool, assuming it will be located at run-time
		constants.insert( DevaObject( name ) );
	}
}


// node validation //////////////////////////////////////////////////////////

// ensure no arg names are duplicated
void Semantics::AddArg( char* arg, int line )
{
	string sa( arg );
	if( arg_names.count( sa ) != 0 )
		throw DevaSemanticException( boost::format( "Function argument names must be unique: '%1%' multiply defined." ) % arg, line ); 
	else
		arg_names.insert( sa );

	// add arg to the symbol table
	DefineVar( arg, line, mod_arg );
}

// add number constant
void Semantics::AddNumber( double arg )
{
	constants.insert( DevaObject( arg ) );
}

// add string constant
void Semantics::AddString( char* arg )
{
	// strip the string of quotes and unescape it
//	string str( arg );
//	str = unescape( strip_quotes( str ) );
//	char* s = new char[str.length()+1];
//	strcpy( s, str.c_str() );
//	constants.insert( DevaObject( s ) );
	constants.insert( DevaObject( arg ) );
}

// validate lhs of assignment
void Semantics::CheckLhsForAssign( pANTLR3_BASE_TREE lhs )
{
	unsigned int type = lhs->getType( lhs );

	// lhs must be an ID, Key expression or '.'
	if( type != ID && type != Key && type != DOT_OP )
		throw DevaSemanticException( "Invalid l-value on left-hand side of assignment.", lhs->getLine(lhs) );

	char* name = (char*)lhs->getText(lhs)->chars;
	// if lhs is an ID, check with the symbol table and ensure it is defined
	if( type == ID )
	{
		ResolveVar( name, lhs->getLine(lhs) );

		// disallow const assignments
		const Symbol* sym = current_scope->Resolve( name, sym_end );
		if( sym && sym->IsConst() )
			throw DevaSemanticException( "Cannot modify the value of a 'const' variable.", lhs->getLine(lhs) );
	}
}

// validate lhs of augmented assignment
void Semantics::CheckLhsForAugmentedAssign( pANTLR3_BASE_TREE lhs )
{
	unsigned int type = lhs->getType( lhs );

	CheckLhsForAssign( lhs );

	// disallow slices with augmented assigns
	if( type == Key )
	{
		int num_children = lhs->getChildCount( lhs );
		if( num_children > 2 )
			throw DevaSemanticException( "Augmented assignment operators cannot be applied to slices.", lhs->getLine( lhs ) );
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

	// if a NUMBER, set flag
	if( type == NUMBER )
		in->u = (void*)0x1;
	else
		in->u = (void*)0x0;
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

// check default arg val ID to make sure it's a const
void Semantics::CheckDefaultArgVal( char* n, int line )
{
	const Symbol* s = current_scope->Resolve( n, sym_end );
	if( !s || !s->IsConst() )
		throw DevaSemanticException( "Invalid default argument value: must be a constant or a 'const' variable.", line );
}

// add a default arg val (for the previous default arg)
void Semantics::DefaultArgVal( pANTLR3_BASE_TREE node, bool negate /*= false*/ )
{
	// must be at least one arg
	if( arg_names.size() == 0 )
		throw DevaICE( "Default argument value with no arguments." );
	// must not be more default values than args
	if( arg_names.size() <= default_arg_values.size() )
		throw DevaICE( "More default argument values than arguments." );

	// if this is the first default arg for this fcn, mark it
	if( default_arg_values.size() == 0 )
		first_default_arg = arg_names.size() - 1;

	unsigned int type = node->getType( node );
	char* text = (char*)node->getText( node )->chars;
	DevaObject val;
	switch( type )
	{
	case BOOL:
		val.type = obj_boolean;
		if( strcmp( text, "true" ) == 0 )
			val.b = true;
		else
			val.b = false;
		break;
	case NULLVAL:
		val.type = obj_null;
		break;
	case STRING:
		{
		val.type = obj_string;
		val.s = text;
		}
		break;
	case NUMBER:
		val.type = obj_number;
		val.d = atof( text );
		if( negate ) val.d *= -1.0;
		break;
	case ID:
		break;
	}
	default_arg_values.push_back( val );
}

void Semantics::CheckAndResetFun( int line )
{
	// validate the default/non-default args
	if( first_default_arg != -1 && default_arg_values.size() != arg_names.size() - first_default_arg )
		throw DevaSemanticException( "Non-default argument follows default argument", line );

	// copy the default args to the scope
	FunctionScope* scope = dynamic_cast<FunctionScope*>(current_scope);
	if( !scope )
		throw DevaICE( "Local scope found where function scope expected" );
	for( int i = 0; i < default_arg_values.size(); i++ )
	{
		scope->GetDefaultArgVals().Add( default_arg_values[i]  );
	}

	// reset our fcn tracking vars
	arg_names.clear();
	default_arg_values.clear(); 
	first_default_arg = -1;
}

// compose and define the module name
void Semantics::CheckImport( pANTLR3_BASE_TREE node )
{
	string modname;

	// get each child and append its name to the module name string
	int num_children = node->getChildCount( node );
	for( int i = 0; i < num_children; i++ )
	{
		pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)node->getChild( node, i );
		char* childname = (char*)child->getText( child )->chars;
		modname += childname;
	}
	char* mod = new char[modname.length() + 1];
	strcpy( mod, modname.c_str() );
	DefineVar( mod, node->getLine( node ) );
}

} // namespace deva_compile
