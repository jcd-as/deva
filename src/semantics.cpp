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
#include "builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"

namespace deva_compile
{

/////////////////////////////////////////////////////////////////////////////
// semantic checking functions and globals
/////////////////////////////////////////////////////////////////////////////
Semantics* semantics = NULL;


void Semantics::DumpSymbolTable()
{
	cout << "Symbol table:" << endl;
	for( vector<deva_compile::Scope*>::iterator i = scopes.begin(); i != scopes.end(); ++i )
	{
		if( *i )
			(*i)->Print();
	}
}

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
	// disallow non-keyword "keywords":
	if( strcmp( name, "delete" ) == 0 )
		throw SemanticException( "Syntax error: keyword used as identifier.", line );

	// disallow 'self' outside of a method
	if( strcmp( name, "self" ) == 0 && !(in_class && in_fcn) )
		throw SemanticException( "Syntax error: 'self' only allowed inside methods.", line );

	// disallow defining builtins as non-locals... ???
	if( mod == mod_none || mod == mod_external )
	{
		if( IsBuiltin( string( name ) ) || IsVectorBuiltin( string( name ) ) || IsMapBuiltin( string( name ) ) )
			return;
	}

	Symbol *sym = new Symbol( name, sym_variable, mod );
	if( !current_scope->Define( sym ) )
	{
		delete sym;
	}
	// add the var name to the constant pool
	constants.insert( Object( obj_symbol_name, name ) );

	// if this is a module name, also add it to the list of modules
	if( mod == mod_module_name )
		module_names.insert( name );
}

// resolve a variable, in the current scope
void Semantics::ResolveVar( char* name, int line )
{
	// TODO: modules???
	// accept builtins
	if( IsBuiltin( string( name ) ) || IsVectorBuiltin( string( name ) ) || IsMapBuiltin( string( name ) ) )
		return;

	// inside a method, always accept 'self'
	if( in_class && strcmp( name, "self" ) == 0 )
		return;

	// if we're on the rhs of a dot-op we need to add symbols referenced
	if( rhs_of_dot )
	{
		AddString( name );
	}
	// look up the variable in the current scope
	else if( !current_scope->Resolve( name, sym_end ) )
	{
		throw SemanticException( str(boost::format( "Symbol '%1%' not defined." ) % name).c_str(), line );
	}
}

// define a function in the current scope
void Semantics::DefineFun( char* name, char* classname, int line )
{
	// 'new' & 'delete' methods (constructor/destructor) disallowed outside of classes
	if( strcmp( name, "new" ) == 0 && !classname )
	{
		throw SemanticException( "'new' method is not allowed outside of a class definition.", line );
	}
	if( strcmp( name, "delete" ) == 0 && !classname )
	{
		throw SemanticException( "'delete' method is not allowed outside of a class definition.", line );
	}

	Symbol *sym = new Symbol( name, sym_function, mod_none );
	if( !current_scope->Define( sym ) )
	{
		delete sym;
	}
	// add the name to the constant pool
	constants.insert( Object( obj_symbol_name, name ) );
}

// resolve a function, in the current scope
void Semantics::ResolveFun( char* name, int line )
{
	// TODO: modules???
	// if we're on the rhs of a dot-op we need to add symbols referenced
	if( rhs_of_dot )
	{
		AddString( name );
		DefineVar( name, line );
	}

	// if it is a builtin fcn, nothing to do
	if( IsBuiltin( string( name ) ) || IsVectorBuiltin( string( name ) ) || IsMapBuiltin( string( name ) ) )
	{
		return;
	}

	// look up the function in the current scope
	if( !current_scope->Resolve( name, sym_end ) )
	{
		throw SemanticException( str(boost::format( "Symbol '%1%' not defined." ) % name).c_str(), line );
	}
}


// node validation //////////////////////////////////////////////////////////

// ensure no arg names are duplicated
void Semantics::AddArg( char* arg, int line )
{
	string sa( arg );
	if( arg_names.count( sa ) != 0 )
		throw SemanticException( boost::format( "Function argument names must be unique: '%1%' multiply defined." ) % arg, line ); 
	else
		arg_names.insert( sa );

	// add arg to the symbol table
	DefineVar( arg, line, mod_arg );
}

// add number constant
void Semantics::AddNumber( double arg )
{
	constants.insert( Object( arg ) );
}

// add string constant
void Semantics::AddString( char* arg )
{
	constants.insert( Object( arg ) );
}

// validate lhs of assignment
void Semantics::CheckLhsForAssign( pANTLR3_BASE_TREE lhs )
{
	unsigned int type = lhs->getType( lhs );

	// lhs must be an ID, Key expression or '.'
	if( type != ID && type != Key && type != DOT_OP )
		throw SemanticException( "Invalid l-value on left-hand side of assignment.", lhs->getLine(lhs) );

	char* name = (char*)lhs->getText(lhs)->chars;
	// if lhs is an ID, check with the symbol table and ensure it is defined
	if( type == ID )
	{
		ResolveVar( name, lhs->getLine(lhs) );

		// disallow const assignments
		const Symbol* sym = current_scope->Resolve( name, sym_end );
		if( sym && sym->IsConst() )
			throw SemanticException( "Cannot modify the value of a 'const' variable.", lhs->getLine(lhs) );
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
			throw SemanticException( "Augmented assignment operators cannot be applied to slices.", lhs->getLine( lhs ) );
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
		throw SemanticException( "Invalid left-hand side of relational operator.", lhs->getLine(lhs) );
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
		throw SemanticException( "Invalid right-hand side of relational operator.", rhs->getLine(rhs) );
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
		throw SemanticException( "Cannot use a string in a logical expression.", lhs->getLine(lhs) );
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
		throw SemanticException( "Invalid left-hand side of arithmetic operator.", lhs->getLine(lhs) );
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
		throw SemanticException( "Invalid right-hand side of arithmetic operator.", rhs->getLine(rhs) );
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
		throw SemanticException( "Invalid left-hand side of addition operator.", lhs->getLine(lhs) );
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
		throw SemanticException( "Invalid right-hand side of addition operator.", rhs->getLine(rhs) );
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
		throw SemanticException( "Invalid operand for negate ('-') operator.", in->getLine(in) );

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
		&& type != NULLVAL
		&& type != STRING
		&& type != BOOL )
		throw SemanticException( "Invalid operand for logical not ('!') operator.", in->getLine(in) );
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
		throw SemanticException( "Invalid conditional.", condition->getLine(condition) );
}

// validate key expressions (slices)
void Semantics::CheckKeyExp( pANTLR3_BASE_TREE idx1, pANTLR3_BASE_TREE idx2 /*= NULL*/, pANTLR3_BASE_TREE idx3 /*= NULL*/ )
{
	// for non-slices, if the index is a string, it can be accessed via '.',
	// so we need to add it as a symbol as well as a string
	if( idx2 == NULL )
	{
		unsigned int idx_type = idx1->getType( idx1 );
		if( idx_type == STRING )
			constants.insert( Object( obj_symbol_name, (char*)idx1->getText( idx1 )->chars ) );
		return;
	}

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
		throw SemanticException( "Invalid first index in slice.", idx1->getLine(idx1) );

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
		throw SemanticException( "Invalid second index in slice.", idx2->getLine(idx2) );

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
		throw SemanticException( "Invalid third index in slice.", idx3->getLine(idx3) );
	}
}

// validate break/continue
void Semantics::CheckBreakContinue( pANTLR3_BASE_TREE node )
{
	// invalid if we're not inside a loop
	if( in_loop == 0 )
		throw SemanticException( "Invalid 'break' or 'continue': must be inside a loop.", node->getLine(node) );
}

// validate return
void Semantics::CheckReturn( pANTLR3_BASE_TREE node )
{
	// TODO: this reports the wrong line num (0):
	if( !in_fcn )
		throw SemanticException( "Invalid 'return': must be inside a function.", node->getLine(node) );
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
		throw SemanticException( "Invalid statement: statement has no effect.", node->getLine( node ) );
}		

// check default arg val ID to make sure it's a const
void Semantics::CheckDefaultArgVal( char* n, int line )
{
	const Symbol* s = current_scope->Resolve( n, sym_end );
	if( !s || !s->IsConst() )
		throw SemanticException( "Invalid default argument value: must be a constant or a 'const' variable.", line );
}

// add a default arg val (for the previous default arg)
void Semantics::DefaultArgVal( pANTLR3_BASE_TREE node, bool negate /*= false*/ )
{
	// must be at least one arg
	if( arg_names.size() == 0 )
		throw ICE( "Default argument value with no arguments." );
	// must not be more default values than args
	if( arg_names.size() <= default_arg_values.size() )
		throw ICE( "More default argument values than arguments." );

	// if this is the first default arg for this fcn, mark it
	if( default_arg_values.size() == 0 )
		first_default_arg = arg_names.size() - 1;

	unsigned int type = node->getType( node );
	char* text = (char*)node->getText( node )->chars;
	Object val;
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
		// this MUST be a const variable!
		const Symbol *sym = current_scope->Resolve( text );
		if( !sym->IsConst() )
			throw SemanticException( "Only 'const' variables can be used as default argument values.", node->getLine( node ) );
		val.type = obj_symbol_name;
		val.s = text;
		break;
	}
	default_arg_values.push_back( val );
}

void Semantics::CheckAndResetFun( int line )
{
	// validate the default/non-default args
	if( first_default_arg != -1 && default_arg_values.size() != arg_names.size() - first_default_arg )
		throw SemanticException( "Non-default argument follows default argument", line );

	// copy the default args to the scope
	FunctionScope* scope = dynamic_cast<FunctionScope*>(current_scope);
	if( !scope )
		throw ICE( "Local scope found where function scope expected" );
	for( size_t i = 0; i < default_arg_values.size(); i++ )
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
	// this string will be freed by the Compiler object after it adds it to the
	// Executor's module list...
	char* mod = new char[modname.length() + 1];
	strcpy( mod, modname.c_str() );
	DefineVar( mod, node->getLine( node ), mod_module_name );
}

} // namespace deva_compile
