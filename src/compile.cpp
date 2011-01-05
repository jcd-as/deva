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

// compile.cpp
// compilation global object/functions for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#include "compile_walker.h"
#include "semantics.h"
#include "compile.h"
#include "util.h"
#include "vector_builtins.h"

#include <iostream>
#include <cmath>
#include <climits>

namespace deva_compile
{

/////////////////////////////////////////////////////////////////////////////
// compilation functions and globals
/////////////////////////////////////////////////////////////////////////////
Compiler* compiler = NULL;
// tracking scopes inside loops (for break/continue statements)
static vector<dword> loop_scope_stack;
static vector< vector<dword>* > loop_break_locations;
// tracking scopes inside functions (for return statements)
static vector<dword> fcn_scope_stack;


Compiler::Compiler( Semantics* sem, Executor* ex ) : 
	max_scope_idx( 0 ),
	num_locals( 0 ),
	fcn_nesting( 0 ),
	in_class( false ),
	is_dot_rhs( false )
{
	// create the instruction stream
	is = new InstructionStream();

	// all global names and constants MUST be added here, as the compiler needs
	// to generate and use indices into their collections, and adding items to
	// the sorted collections will invalidate the indices already used

	// copy the global names and consts from the Semantics pass/object to the executor
	// (locals will be added as the compiler gets to each fcn declaration, see DefineFun)
	// constants
	for( set<Object>::iterator i = sem->constants.begin(); i != sem->constants.end(); ++i )
	{
		// copy (allocate new) strings
		if( i->type == obj_string )
		{
			// strip quotes and unescape
			string str( i->s );
			str = unescape( strip_quotes( str ) );
			char* s = new char[str.length()+1];
			strcpy( s, str.c_str() );
			// try to add the string constant, if we aren't allowed to (because
			// it's a duplicate), free the string
			if( !ex->AddConstant( Object( s ) ) )
				delete [] s;
		}
		else if( i->type == obj_symbol_name )
		{
			char* s = copystr( i->s );
			// try to add the symbol name, if we aren't allowed to (because
			// it's a duplicate), free the string
			if( !ex->AddConstant( Object( obj_symbol_name, s ) ) )
				delete [] s;
		}
		else
			ex->AddConstant( *i );
	}

	// add the builtins, vector builtins and map builtins to the constant pool
	for( int i = 0; i < num_of_builtins; i++ )
		ex->AddConstant( Object( obj_symbol_name, copystr( builtin_names[i].c_str() ) ) );
	for( int i = 0; i < num_of_vector_builtins; i++ )
		ex->AddConstant( Object( obj_symbol_name, copystr( vector_builtin_names[i].c_str() ) ) );
	// TODO: add map builtins
//	for( int i = 0; i < num_of_map_builtins; i++ )
//		ex->AddConstant( Object( obj_symbol_name, copystr( map_builtin_names[i].c_str() ) ) );

	// add our 'global' function, "@main"
	FunctionScope* scope = dynamic_cast<FunctionScope*>(sem->global_scope);
	Function* f = new Function();
	f->name = string( "@main" );
	f->filename = string( current_file );
	f->first_line = 0;
	f->num_args = 0;
	f->num_locals = scope->NumLocals();
	f->addr = 0;
	// add the global names for the global scope
	for( int i = 0; i < scope->GetNames().Size(); i++ )
	{
		Symbol* s = scope->GetNames().At( i );
		if( s->IsLocal() || s->IsArg() || s->IsConst() )
			f->local_names.Add( s->Name() );
	}
	ex->AddFunction( f );

	// set-up the scope stack to match
	AddScope();
}

// block
void Compiler::EnterBlock()
{
	// track the loop scopes
	if( loop_scope_stack.size() > 0 )
	{
		int loop_scopes = loop_scope_stack.back();
		loop_scope_stack.pop_back();
		loop_scopes++;
		loop_scope_stack.push_back( loop_scopes );
	}
	// track the fcn scopes
	if( fcn_scope_stack.size() > 0 )
	{
		int fcn_scopes = fcn_scope_stack.back();
		fcn_scope_stack.pop_back();
		fcn_scopes++;
		fcn_scope_stack.push_back( fcn_scopes );
	}
	// track the current scope
	AddScope();
	// emit an enter instruction
	Emit( op_enter );
}
void Compiler::ExitBlock()
{
	// track the loop scopes
	if( loop_scope_stack.size() > 0 )
	{
		int loop_scopes = loop_scope_stack.back();
		loop_scope_stack.pop_back();
		loop_scopes--;
		loop_scope_stack.push_back( loop_scopes );
	}
	// track the fcn scopes
	if( fcn_scope_stack.size() > 0 )
	{
		int fcn_scopes = fcn_scope_stack.back();
		fcn_scope_stack.pop_back();
		fcn_scopes--;
		fcn_scope_stack.push_back( fcn_scopes );
	}
	// track the current scope
	LeaveScope();
	// emit a leave instruction
	Emit( op_leave );
}

// define a function
void Compiler::DefineFun( char* name, char* classname, int line )
{
	// TODO: new/delete methods need code added that calls the base-class
	// new/delete methods

	// generate jump around fcn so 'main' (or module 'global' as the case
	// may be) doesn't execute its body inline
	// emit the jump over fcn body
	Emit( op_jmp, (dword)-1 );
	AddPatchLoc();

	// start a new count of scopes inside this fcn
	fcn_scope_stack.push_back( 0 );

	FunctionScope* scope = dynamic_cast<FunctionScope*>(CurrentScope());

	// create a new Function object
	Function* fcn = new Function();
	fcn->name = string( name );
	if( classname )
	{
		fcn->name += "@";
		fcn->name += classname;
	}
	fcn->filename = string( current_file );
	fcn->first_line = line;
	fcn->num_args = scope->NumArgs();
	fcn->num_locals = scope->NumLocals();
	// set the code address for this function
	fcn->addr = is->Length();

	// add the global names for this (function) scope and its local scope children
	for( int i = 0; i < scope->GetNames().Size(); i++ )
	{
		Symbol* s = scope->GetNames().At( i );
		if( s->IsLocal() || s->IsArg() || s->IsConst() )
			fcn->local_names.Add( s->Name() );
	}
	// add the default arg indices for this (function) scope
	for( int i = 0; i < scope->GetDefaultArgVals().Size(); i++ )
	{
		int idx = -1;
		// find the index for this constant
		Object obj = scope->GetDefaultArgVals().At( i );
		if( obj.type == obj_string )
		{
			// strip the string of quotes and unescape it
			string str( obj.s );
			str = unescape( strip_quotes( str ) );
			char* s = new char[str.length()+1];
			strcpy( s, str.c_str() );
			idx = GetConstant( Object( s ) );
			delete [] s;
		}
		else
			idx = GetConstant( obj );
		fcn->default_args.Add( idx );
	}

	// add to the list of fcn objects
	ex->AddFunction( fcn );
}

void Compiler::EndFun()
{
	// generate a 'return' statement and return value, 
	// in case there isn't one that will be hit
	// (check the immediately preceding instruction so we at least don't
	// generate two returns in a row...)
	if( is->Length() <= 5 || (Opcode)*(is->Current()-5) != op_return )
	{
		Emit( op_push_null );
		Emit( op_return, 0 );
	}

	// back-patch the jump over fcn body
	BackpatchToCur();
}

// define a class
void Compiler::DefineClass( char* name, int line )
{
	// TODO: 
	// - create the map for the class
	// - add the __name__, __class__, __module__ and __bases__ members
//	Emit( op_pushconst, /*__name__*/ name );
//	Emit( op_pushconst, /*__class__*/ cls );
//	Emit( op_pushconst, /*__module__*/ module );
//	Emit( op_pushconst, /*__bases__*/ bases );
//	Emit( op_new_class, 4 );
	Emit( op_new_class, 0 );
	// TODO:
	// - if new and delete methods aren't defined for this class, we need to add
	// them and generate code for them (that calls the base-class new/delete
	// methods)
}

// constants
void Compiler::Number( pANTLR3_BASE_TREE node )
{
	double d = atof( (char*)node->getText(node)->chars );
	double intpart; // for modf call

	// negate numbers that should be negative
	unsigned int type = node->getType( node );
	if( type == NUMBER )
	{
		// check for negate flag from pass one
		if( (int)(node->u) == 0x1 )
			d *= -1.0;
	}
	else
		throw ICE( "Expecting a number type." );

	if( d == 0.0 )
	{
		Emit( op_push_zero );
	}
	else if( d == 1.0 )
	{
		Emit( op_push_one );
	}
	else if( d <= INT_MAX && modf( d, &intpart ) == 0.0 )
	{
		// use op_push to push an integer value directly
		Emit( op_push, (dword)((int)intpart) );
	}
	else
	{
		// get the constant pool index for this number
		int idx = GetConstant( Object( d ) );
		if( idx < 0 )
			throw ICE( boost::format( "Cannot find constant '%1%'." ) % d );
		// emit op to push it onto the stack
		Emit( op_pushconst, (dword)idx );
	}
}

void Compiler::String( char* name )
{
	// string must be un-quoted and unescaped
	string str( name );
	str = unescape( strip_quotes( str ) );
	char* s = new char[str.length()+1];
	strcpy( s, str.c_str() );

	// get the constant pool index for this string
	int idx = GetConstant( Object( s ) );
	if( idx < 0 )
	{
		delete s;
		throw ICE( boost::format( "Cannot find constant '%1%'." ) % s );
	}
	// emit op to push it onto the stack
	Emit( op_pushconst, (dword)idx );
	delete [] s;
}

// identifier
void Compiler::Identifier( char* s, bool is_lhs_of_assign )
{
	// don't do anything for the left-hand sides of assignments, 
	// the assignment op will generate the appropriate store/tbl_store op
	if( is_lhs_of_assign )
		return;

	// emit op to push it onto the stack

	// is it a local?
	int idx = CurrentScope()->ResolveLocalToIndex( s );

	// no? look it up as a non-local
	if( idx == -1 )
	{
		// get the constant pool index for this identifier
		idx = GetConstant( Object( obj_symbol_name, s ) );
		if( idx < 0 )
		{
			// if we're on the right hand side of a dot op, try looking for it
			// as a string too, as it could be a map/class/instance member 
			// (where 'a.b' is just short-hand for 'a["b"]')
			if( is_dot_rhs )
			{
				idx = GetConstant( Object( s ) );
			}

			if( idx < 0 )
				throw ICE( boost::format( "Cannot find constant '%1%'." ) % s );
		}

		Emit( op_pushconst, (dword)idx );
	}
	// local
	else
	{
		// index under 10: use the short instructions
		if( idx < 10 )
		{
			switch( idx )
			{
			case 0:
				Emit( op_pushlocal0 );
				break;
			case 1:
				Emit( op_pushlocal1 );
				break;
			case 2:
				Emit( op_pushlocal2 );
				break;
			case 3:
				Emit( op_pushlocal3 );
				break;
			case 4:
				Emit( op_pushlocal4 );
				break;
			case 5:
				Emit( op_pushlocal5 );
				break;
			case 6:
				Emit( op_pushlocal6 );
				break;
			case 7:
				Emit( op_pushlocal7 );
				break;
			case 8:
				Emit( op_pushlocal8 );
				break;
			case 9:
				Emit( op_pushlocal9 );
				break;
			}
		}
		else
			Emit( op_pushlocal, (dword)idx );
	}
}

// unary operators
void Compiler::NegateOp( pANTLR3_BASE_TREE node )
{
	// test for simple case where node is a number
	unsigned int type = node->getType( node );
	bool is_negative_number = false;
	if( type == NUMBER )
	{
		// check for negate flag from pass one
		if( (int)(node->u) == 0x1 )
			is_negative_number = true;
	}

	// don't need to do anything for negative number constants
	if( !is_negative_number )
		Emit( op_neg );
}

void Compiler::NotOp( pANTLR3_BASE_TREE node )
{
	// TODO: test for simple case where node is a boolean, string or number
	Emit( op_not );
}

// assignments and variable decls
void Compiler::LocalVar( char* n )
{
	// get the index for the name
	int idx = CurrentScope()->ResolveLocalToIndex( n );
	if( idx == -1 )
		throw ICE( boost::format( "Cannot locate local symbol '%1%'." ) % n );
	// emit a 'op_def_localN' op
	if( idx < 10 )
	{
		switch( idx )
		{
		case 0:
			Emit( op_def_local0 );
			break;
		case 1:
			Emit( op_def_local1 );
			break;
		case 2:
			Emit( op_def_local2 );
			break;
		case 3:
			Emit( op_def_local3 );
			break;
		case 4:
			Emit( op_def_local4 );
			break;
		case 5:
			Emit( op_def_local5 );
			break;
		case 6:
			Emit( op_def_local6 );
			break;
		case 7:
			Emit( op_def_local7 );
			break;
		case 8:
			Emit( op_def_local8 );
			break;
		case 9:
			Emit( op_def_local9 );
			break;
		}
	}
	else
		Emit( op_def_local, (dword)idx );
}

void Compiler::ExternVar( char* n, bool is_assign )
{
	// find the name in the constant pool
	int idx = GetConstant( Object( obj_symbol_name, n ) );
	// not found? error
	if( idx == -1 )
		throw ICE( boost::format( "Cannot find constant '%1%'." ) % n );

	// if this is an assignment, generate a store op
	if( is_assign )
		Emit( op_storeconst, (dword)idx );
}

void Compiler::Assign( pANTLR3_BASE_TREE lhs_node )
{
	// the immediate lhs always determines whether this is a simple store (an
	// ID) or a table store (Key/DOT_OP)
	unsigned int type = lhs_node->getType( lhs_node );

	// is the lhs node an identifier?
	if( type == ID )
	{
		// get the text
		char* lhs = (char*)lhs_node->getText( lhs_node )->chars;
		// is it a local?
		int idx = CurrentScope()->ResolveLocalToIndex( lhs );

		// no? look it up as a non-local
		if( idx == -1 )
		{
			idx = GetConstant( Object( obj_symbol_name, lhs ) );
			// not found? error
			if( idx == -1 )
				throw ICE( boost::format( "Non-local symbol '%1%' not found." ) % lhs );

			Emit( op_storeconst, (dword)idx );
		}
		else
		{
			// index under 10: use the short instructions
			if( idx < 10 )
			{
				switch( idx )
				{
				case 0:
					Emit( op_storelocal0 );
					break;
				case 1:
					Emit( op_storelocal1 );
					break;
				case 2:
					Emit( op_storelocal2 );
					break;
				case 3:
					Emit( op_storelocal3 );
					break;
				case 4:
					Emit( op_storelocal4 );
					break;
				case 5:
					Emit( op_storelocal5 );
					break;
				case 6:
					Emit( op_storelocal6 );
					break;
				case 7:
					Emit( op_storelocal7 );
					break;
				case 8:
					Emit( op_storelocal8 );
					break;
				case 9:
					Emit( op_storelocal9 );
					break;
				}
			}
			else
				Emit( op_storelocal, (dword)idx );
		}
	}
	// is the lhs a key (or dot) op?
	else if( type == Key || type == DOT_OP )
	{
		// key op will have 2, 3 or 4 children, depending on whether it is a
		// simple index or a 2 or 3 index slice...
		int num_children = lhs_node->getChildCount( lhs_node );
		if( num_children == 2 )
		{
			// simple index
			Emit( op_tbl_store );
		}
		else if( num_children == 3 )
		{
			// 2 index slice
			Emit( op_storeslice2 );
		}
		else if( num_children == 4 )
		{
			// 3 index slice
			Emit( op_storeslice3 );
		}
	}
}

// function call
void Compiler::CallOp( pANTLR3_BASE_TREE fcn, pANTLR3_BASE_TREE args, pANTLR3_BASE_TREE parent )
{
	// how many args are being pushed?
	int num_children = args->getChildCount( args );
	Emit( op_call, (dword)num_children );
	// if the return value is unused, a pop instruction needs to be generated
	if( parent )
	{
		// TODO: actually, i think anything except NULL indicates a pop...
		unsigned int type = parent->getType( parent );
		if( type != ASSIGN_OP 
			&& type != ADD_EQ_OP
			&& type != SUB_EQ_OP
			&& type != MUL_EQ_OP
			&& type != DIV_EQ_OP
			&& type != MOD_EQ_OP
			&& type != Const
			&& type != Local
			&& type != Extern
		  )
			Emit( op_pop );
	}
}

// augmented assignment operators
void Compiler::AugmentedAssignOp(  pANTLR3_BASE_TREE lhs_node, Opcode op )
{
	// the immediate lhs always determines whether this is a simple store (an
	// ID) or a table store (Key/DOT_OP)
	unsigned int type = lhs_node->getType( lhs_node );

	// is the lhs node an identifier?
	if( type == ID )
	{
		// get the text
		char* lhs = (char*)lhs_node->getText( lhs_node )->chars;

		// is it a local?
		int idx = CurrentScope()->ResolveLocalToIndex( lhs );

		// no? look it up as a non-local
		if( idx == -1 )
		{
			// look it up in the constant pool
			int idx = GetConstant( Object( obj_symbol_name, lhs ) );
			// not found? error
			if( idx == -1 )
				throw ICE( boost::format( "Non-local symbol '%1%' not found." ) % lhs );

			switch( op )
			{
			case op_add:
				Emit( op_add_assign, (dword)idx );
				break;
			case op_sub:
				Emit( op_sub_assign, (dword)idx );
				break;
			case op_mul:
				Emit( op_mul_assign, (dword)idx );
				break;
			case op_div:
				Emit( op_div_assign, (dword)idx );
				break;
			case op_mod:
				Emit( op_mod_assign, (dword)idx );
				break;
			default:
				throw ICE( "Unknown augmented assign operator." );
				break;
			}
		}
		else
		{
			switch( op )
			{
			case op_add:
				Emit( op_add_assign_local, (dword)idx );
				break;
			case op_sub:
				Emit( op_sub_assign_local, (dword)idx );
				break;
			case op_mul:
				Emit( op_mul_assign_local, (dword)idx );
				break;
			case op_div:
				Emit( op_div_assign_local, (dword)idx );
				break;
			case op_mod:
				Emit( op_mod_assign_local, (dword)idx );
				break;
			default:
				throw ICE( "Unknown augmented assign operator." );
				break;
			}
		}
	}
	// is the lhs a key (or dot) op?
	else if( type == Key || type == DOT_OP )
	{
		switch( op )
		{
		case op_add:
			Emit( op_add_tbl_store );
			break;
		case op_sub:
			Emit( op_sub_tbl_store );
			break;
		case op_mul:
			Emit( op_mul_tbl_store );
			break;
		case op_div:
			Emit( op_div_tbl_store );
			break;
		case op_mod:
			Emit( op_mod_tbl_store );
			break;
		default:
			throw ICE( "Unknown augmented assign operator." );
			break;
		}
	}
}

// Key ('[]') and Dot ('.') ops
void Compiler::KeyOp( pANTLR3_BASE_TREE key_exp, bool is_lhs_of_assign )
{
	// do nothing for left-hand side of assign,
	// assignment op will take care of generating the tbl_store
	if( is_lhs_of_assign )
		return;

	// TODO: handle slices...
	Emit( op_tbl_load );
}

void Compiler::ReturnOp( bool no_val /*= false*/ )
{
	if( no_val )
		Emit( op_push_null );

	dword scopecount = fcn_scope_stack.back();
	Emit( op_return, scopecount );
}

void Compiler::ContinueOp()
{
	dword scopecount = loop_scope_stack.back();
	dword address = labelstack.back();
	Emit( op_exit_loop, address, scopecount );
}

void Compiler::BreakOp()
{
	dword scopecount = loop_scope_stack.back();

	// save the location for back-patching
	// (to be double-safe, check to ensure we're in a loop...)
	if( loop_break_locations.size() == 0 )
		throw ICE( "Invalid break statement: Not inside a loop.." );

	Emit( op_exit_loop, (dword)-1, scopecount );
	loop_break_locations.back()->push_back( is->Length() - (2 * sizeof(dword)) );
}


void Compiler::ImportOp( pANTLR3_BASE_TREE node )
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

	// find the name in the constant pool
	int idx = GetConstant( Object( (char*)modname.c_str() ) );
	// not found? error
	if( idx == -1 )
		throw ICE( boost::format( "Symbol '%1%' not found." ) % modname );
	Emit( op_import, (dword)idx );
}

// 'new'
void Compiler::NewOp()
{
	// generate a new_instance op for the call to 'new' to use
	// TODO: populate with __class__ etc
	Emit( op_new_instance, 0 );
}

// vector and map creation ops
void Compiler::VecOp( pANTLR3_BASE_TREE node )
{
	int num_children = node->getChildCount( node );
	Emit( op_new_vec, (dword)num_children );
}

void Compiler::MapOp( pANTLR3_BASE_TREE node )
{
	int num_children = node->getChildCount( node );
	Emit( op_new_map, (dword)num_children );
}

// if/else statements
void Compiler::IfOpJump()
{
	// generate a jmpf to the else-label
	Emit( op_jmpf, (dword)-1 );
	AddPatchLoc();
}

void Compiler::EndIfOpJump()
{
	// back-patch the 'if' jmpf to current loc
	BackpatchToCur();
}

void Compiler::ElseOpJump()
{
	// generate the jmp to the end-else-label (for the preceding 'if')
	Emit( op_jmp, (dword)-1 );
	// back-patch the 'if' jmpf to current loc
	BackpatchToCur();
	// generate the else-label
	AddPatchLoc();
}

void Compiler::ElseOpEndLabel()
{
	// backpatch the else jmp to current loc
	BackpatchToCur();
}

// while statement
void Compiler::WhileOpStart()
{
	// start a new count of scopes inside this loop
	loop_scope_stack.push_back( 0 );

	// start a new loop context for back-patching 'break' statements
	loop_break_locations.push_back( new vector<dword>() );

	// mark the pre-condition-IL label
	AddLabel();
}

void Compiler::WhileOpConditionJump()
{
	// emit the conditional jump
	Emit( op_jmpf, (dword)-1 );
	AddPatchLoc();
}

void Compiler::WhileOpEnd()
{
	// emit the jump-to-start
	Emit( op_jmp, (dword)-1 );
	AddPatchLoc();
	BackpatchToLastLabel();
			
	// back-patch the conditional jump
	BackpatchToCur();

	loop_scope_stack.pop_back();

	// back-patch any 'break' statements
	// pop the loop from the loop stack
	vector<size_t>* breaks = loop_break_locations.back();
	loop_break_locations.pop_back();
	for( vector<size_t>::iterator it = breaks->begin(); it != breaks->end(); ++it )
	{
		// back-patch the instruction at the break to point to the current
		// location in the instruction stream (end of this loop)
		is->Set( *it, (size_t)is->Length() );
	}
	delete breaks;
}

// maps
void Compiler::InOp( char* key, char* val, pANTLR3_BASE_TREE container )
{
	//TODO: implement
}

// vectors
void Compiler::InOp( char* key, pANTLR3_BASE_TREE container )
{
	// is the key a local?
	int key_idx = CurrentScope()->ResolveLocalToIndex( key );

	// not found? error out, for loop vars are always locals
	if( key_idx == -1 )
	{
		throw ICE( boost::format( "For loop variable '%1%' not found in the local symbols." ) % key );
	}

	// the vector to iterate is on top of the stack, dup it for the calls to
	// 'rewind' and 'next'
	Emit( op_dup1 );
	
	// generate the call to 'rewind'
	int rewind_idx = GetConstant( Object( obj_symbol_name, const_cast<char*>("rewind") ) );
	// not found? error
	if( rewind_idx == -1 )
		throw ICE( "Non-local symbol 'rewind' not found in Compiler::InOp()." );
	Emit( op_pushconst, (dword)rewind_idx );
	Emit( op_tbl_load );
	Emit( op_call, 0 );
	Emit( op_pop );	// pop the unused return value from 'rewind'

	// vector to iterate is now on top of the stack again

	// mark the label for the loop beginning
	AddLabel();

	// generate the for_iter instruction
	Emit( op_for_iter, (dword)-1 );

	// mark it for back-patching
	AddPatchLoc();

	// the key is now on the stack, store it into the local loop var

	// index under 10: use the short instructions
	if( key_idx < 10 )
	{
		switch( key_idx )
		{
		case 0:
			Emit( op_storelocal0 );
			break;
		case 1:
			Emit( op_storelocal1 );
			break;
		case 2:
			Emit( op_storelocal2 );
			break;
		case 3:
			Emit( op_storelocal3 );
			break;
		case 4:
			Emit( op_storelocal4 );
			break;
		case 5:
			Emit( op_storelocal5 );
			break;
		case 6:
			Emit( op_storelocal6 );
			break;
		case 7:
			Emit( op_storelocal7 );
			break;
		case 8:
			Emit( op_storelocal8 );
			break;
		case 9:
			Emit( op_storelocal9 );
			break;
		}
	}
	else
		Emit( op_storelocal, (dword)key_idx );

	// loop body:
}

void Compiler::ForOpEnd()
{
	// emit the jump-to-start
	Emit( op_jmp, (dword)-1 );
	AddPatchLoc();
	BackpatchToLastLabel();

	// backpatch the for_iter instruction with this (loop complete) location
	BackpatchToCur();

	// pop the remaining iterable item off the stack
	Emit( op_pop );
}


} // namespace deva_compile
