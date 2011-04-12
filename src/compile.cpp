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
#include "map_builtins.h"

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


Compiler::Compiler( const char* mod_name, Semantics* sem ) : 
	emit_debug_info( true ),
	max_scope_idx( 0 ),
	module_name( mod_name ),
	num_locals( 0 ),
	fcn_nesting( 0 ),
	is_method( false ),
	in_class( false ),
	in_constructor( false ),
	is_dot_rhs( false )
{
	// create the instruction stream
	is = new InstructionStream();

	// and the line map (note that ownership of the line map is passed out when
	// a Code object is created from this compiler module, so it is not deleted
	// here, but in the Code destructor)
	lines = new LineMap();

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
			// strip quotes and unescape
			string str( i->s );
			str = unescape( strip_quotes( str ) );
			char* s = new char[str.length()+1];
			strcpy( s, str.c_str() );
			// try to add the symbol name, if we aren't allowed to (because
			// it's a duplicate), free the string
			if( !ex->AddConstant( Object( obj_symbol_name, s ) ) )
				delete [] s;
		}
		else
			ex->AddConstant( *i );
	}

	// copy the module names to the executor
	for( set<char*>::iterator i = sem->module_names.begin(); i != sem->module_names.end(); ++i )
	{
		ex->AddModuleName( string( *i ) );
		// free the module name string now that we are truly done with it
		delete [] *i;
	}

	// add our 'global' function, "[module]@main"
	FunctionScope* scope = dynamic_cast<FunctionScope*>(sem->global_scope);
	Function* f = new Function();
	f->name = string( "@main" );
	f->filename = string( current_file );
	f->first_line = 0;
	f->num_args = 0;
	f->num_locals = scope->NumLocals();
	f->classname = string( "" );
	f->addr = 0;
	// add the locals
	for( size_t i = 0; i < scope->GetLocals().size(); i++ )
	{
		f->local_names.push_back( scope->GetLocals().operator[]( i ) );
	}
	// compiling a module?
	f->module = NULL;
	f->modulename = module_name;
	ex->AddFunction( "@main", f );

	// with its loop-tracking variables
	in_for_loop.push_back( 0 );
	in_while_loop.push_back( 0 );

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
	// generate def_function/def_method op
	
	int i = GetConstant( Object( obj_symbol_name, name ) );
	if( i == -1 )
		throw ICE( boost::format( "Cannot find constant '%1%' for function name." ) % name );

	string mod;
	if( module_name )
		mod = module_name;
	else
		mod = "";

	EmitLineNum( line );

	// methods emit an op_def_method instruction
	if( classname )
	{
		// get the constant index of this class
		int i2 = GetConstant( Object( obj_symbol_name, classname ) );
		if( i2 == -1 )
			throw ICE( boost::format( "Cannot find constant '%1%' for class name." ) % classname );

		// get the constant index of this module
		int i3 = GetConstant( Object( obj_symbol_name, mod.c_str() ) );
		if( i3 == -1 )
			throw ICE( boost::format( "Cannot find constant '%1%' for module name." ) % mod );

		// add the size of 'op_def_method <Op0> <Op1> <Op2> <Op3>' and 'jmp <Op0>'
		int sz = sizeof( dword ) * 5 + 2;
		EmitLineNum( line );
		Emit( op_def_method, (dword)i, (dword)i2, (dword)i3, (dword)(is->Length() + sz) );
	}
	// non-methods: emit an op_def_function instruction
	else
	{
		// get the constant index of this module
		int i2 = GetConstant( Object( obj_symbol_name, mod.c_str() ) );
		if( i2 == -1 )
			throw ICE( boost::format( "Cannot find constant '%1%' for module name." ) % mod );

		// get the constant index of this function
		// add the size of 'op_def_function <Op0> <Op1> <Op2>' and 'jmp <Op0>'
		int sz = sizeof( dword ) * 4 + 2;
		EmitLineNum( line );
		Emit( op_def_function, (dword)i, (dword)i2, (dword)(is->Length() + sz) );
	}
	
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
		fcn->classname = string( classname );
	}
	else
		fcn->classname = string( "" );
	fcn->filename = string( current_file );
	fcn->first_line = line;
	fcn->num_args = classname ? scope->NumArgs() + 1 : scope->NumArgs();
	fcn->num_locals = scope->NumLocals();
	// set the code address for this function
	fcn->addr = (dword)is->Length();

	// compiling a module?
	fcn->module = NULL;
	fcn->modulename = module_name;

	// add the locals
	for( size_t i = 0; i < scope->GetLocals().size(); i++ )
	{
		fcn->local_names.push_back( scope->GetLocals().operator[]( i ) );
	}
	// add the default arg indices for this (function) scope
	for( size_t i = 0; i < scope->GetDefaultArgVals().size(); i++ )
	{
		int idx = -1;
		// find the index for this constant
		Object obj = scope->GetDefaultArgVals().at( i );
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
		if( idx == -1 )
			throw ICE( boost::format( "Cannot find constant '%1%'." ) % obj );
		fcn->default_args.push_back( idx );
	}

	// add to the list of fcn objects
	string n = name;
	if( classname )
	{
		n += "@";
		n += classname;
	}
	ex->AddFunction( n.c_str(), fcn );
}

void Compiler::EndFun()
{
	// generate a 'return' statement and return value, 
	// in case there isn't one that will be hit
	// (check the immediately preceding instruction so we at least don't
	// generate two returns in a row...)
	if( is->Length() <= 6 || (Opcode)*(is->Current()-6) != op_return )
	{
		Emit( op_push_null );
		Emit( op_return, 0 );
	}

	// back-patch the jump over fcn body
	BackpatchToCur();

	fcn_scope_stack.pop_back();
}

// define a class
void Compiler::DefineClass( char* name, int line, pANTLR3_BASE_TREE bases )
{
	// get the number of base classes and emit the new_class instruction
	int num_bases = bases->getChildCount( bases );

	// get the name of the class on the stack
	int idx = GetConstant( Object( name ) );
	if( idx < 0 )
		throw ICE( boost::format( "Cannot find constant '%1%'." ) % name );

	// emit the new_class op
	Emit( op_pushconst, (dword)idx );
	Emit( op_new_class, num_bases );

	// store the new class into the appropriate local (to 'main')
	LocalVar( name, line );
}

// constants
void Compiler::Number( pANTLR3_BASE_TREE node, int line )
{
	double d = parse_number( (char*)node->getText(node)->chars );
	double intpart; // for modf call

	// negate numbers that should be negative
	unsigned int type = node->getType( node );
	if( type == NUMBER )
	{
		// check for negate flag from pass one
		if( (long)(node->u) == 0x1 )
			d *= -1.0;
	}
	else
		throw ICE( "Expecting a number type." );

	EmitLineNum( line );
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
		Emit( op_push, (int)intpart );
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

void Compiler::String( char* name, int line )
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
		throw ICE( boost::format( "Cannot find constant '%1%'." ) % name );
	}
	// emit op to push it onto the stack
	EmitLineNum( line );
	Emit( op_pushconst, (dword)idx );
	delete [] s;
}

// identifier
void Compiler::Identifier( char* s, bool is_lhs_of_assign, int line )
{
	// don't do anything for the left-hand sides of assignments, 
	// the assignment op will generate the appropriate store/tbl_store op
	if( is_lhs_of_assign )
		return;

	// emit op to push it onto the stack

	EmitLineNum( line );

	// are we the rhs of a dot-op? don't try to look for the name as a local, we
	// need it to remain a symbol
	if( is_dot_rhs )
	{
		// get the constant pool index for this identifier
		int idx = -1;
		// try as a string first
		// ('a.b' is just short-hand for 'a["b"]')
		idx = GetConstant( Object( s ) );
		// then try as a symbol name (for builtins)
		if( idx == -1 )
			idx = GetConstant( Object( obj_symbol_name, s ) );
		if( idx == -1 )
			throw ICE( boost::format( "Cannot find constant '%1%'." ) % s );

		Emit( op_pushconst, (dword)idx );
	}
	else
	{
		// is it a local?
		int idx = CurrentScope()->ResolveLocalToIndex( s );

		// check to see if the local has been defined yet...
		// if not, we can't use it, have to look it up as a non-local
		// (*unless* it's an argument or 'self', which aren't 'defined' 
		// the way the 'normal' locals are)
		int num_args = 0;
		if( idx != -1 )
		{
			bool is_self = false;

			// are we inside a method or a fcn?
			FunctionScope* scope = CurrentScope()->getParentFun();
			if( scope->IsMethod() )
			{
				if( strcmp( "self", s ) == 0 )
					is_self = true;
				else
					num_args = scope->NumArgs() + 1;
			}
			else
				num_args = scope->NumArgs();

			if( !is_self && idx >= num_args && !CurrentScope()->HasLocalBeenGenerated( idx ) )
				idx = -1;
		}

		// no? look it up as a non-local
		if( idx == -1 )
		{
			// get the constant pool index for this identifier
			idx = GetConstant( Object( obj_symbol_name, s ) );
			if( idx < 0 )
				throw ICE( boost::format( "Cannot find constant '%1%'." ) % s );

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
}

// unary operators
void Compiler::NegateOp( pANTLR3_BASE_TREE node, int line )
{
	// test for simple case where node is a number
	unsigned int type = node->getType( node );
	bool is_negative_number = false;
	if( type == NUMBER )
	{
		// check for negate flag from pass one
		if( (long)(node->u) == 0x1 )
			is_negative_number = true;
	}

	// don't need to do anything for negative number constants
	if( !is_negative_number )
	{
		EmitLineNum( line );
		Emit( op_neg );
	}
}

void Compiler::NotOp( pANTLR3_BASE_TREE node, int line )
{
	// TODO: test for simple case where node is a boolean, string or number
	EmitLineNum( line );
	Emit( op_not );
}

// assignments and variable decls
void Compiler::LocalVar( char* n, int line )
{
	// get the index for the name
	int idx = CurrentScope()->ResolveLocalToIndex( n );
	if( idx == -1 )
		throw ICE( boost::format( "Cannot locate local symbol '%1%'." ) % n );
	// mark the local as having been defined in this scope
	CurrentScope()->SetLocalGenerated( idx );

	EmitLineNum( line );

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

void Compiler::ExternVar( char* n, bool is_assign, int line )
{
	// find the name in the constant pool
	int idx = GetConstant( Object( obj_symbol_name, n ) );
	// not found? error
	if( idx == -1 )
		throw ICE( boost::format( "Cannot find constant '%1%'." ) % n );

	// if this is an assignment, generate a store op
	if( is_assign )
	{
		EmitLineNum( line );
		Emit( op_storeconst, (dword)idx );
	}
}

void Compiler::Assign( pANTLR3_BASE_TREE lhs_node, bool parent_is_assign, int line )
{
	// the immediate lhs always determines whether this is a simple store (an
	// ID) or a table store (Key/DOT_OP)
	unsigned int type = lhs_node->getType( lhs_node );

	EmitLineNum( line );

	if( parent_is_assign )
		Emit( op_dup1 );

	// is the lhs node an identifier?
	if( type == ID )
	{
		// get the text
		char* lhs = (char*)lhs_node->getText( lhs_node )->chars;
		// is it a local?
		int idx = CurrentScope()->ResolveLocalToIndex( lhs );
		// has the local been generated yet?
		if( idx != -1 && !CurrentScope()->HasLocalBeenGenerated( idx ) )
			idx = -1;

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
			// if we're in a chained assignment, we dup'd the tos, we need to get
			// that item into it's proper place *before* we do the store
			if( parent_is_assign )
				Emit( op_rot3 );

			// simple index
			Emit( op_tbl_store );
		}
		else if( num_children == 3 )
		{
			// if we're in a chained assignment, we dup'd the tos, we need to get
			// that item into it's proper place *before* we do the store
			if( parent_is_assign )
				Emit( op_rot4 );

			// 2 index slice
			Emit( op_storeslice2 );
		}
		else if( num_children == 4 )
		{
			// if we're in a chained assignment, we dup'd the tos, we need to get
			// that item into it's proper place *before* we do the store
			if( parent_is_assign )
				Emit( op_rot, 5 );

			// 3 index slice
			Emit( op_storeslice3 );
		}
	}
}

// function call
void Compiler::CallOp( pANTLR3_BASE_TREE fcn, pANTLR3_BASE_TREE args, pANTLR3_BASE_TREE parent, int line )
{
	// how many args are being pushed?
	int num_children = args->getChildCount( args );

	EmitLineNum( line );

	// if we generated a method_load to match this call
	if( is_method )
	{
		Emit( op_call_method, (dword)num_children );
		// reset method_load flag
		is_method = false;
	}
	else
		Emit( op_call, (dword)num_children );

	// if the return value is unused, a pop instruction needs to be generated
	// (DOT_OP and Call are the only fcn nodes that pass a non-NULL 'parent' in)
	//
	// if the translation unit is the parent, we pass (void*)-1. kludgey, yes.
	if( parent == (pANTLR3_BASE_TREE)-1 )
		Emit( op_pop );
	else if( parent )
	{
		unsigned int fcn_type = fcn->getType( fcn );
		unsigned int type = parent->getType( parent );

		// examples:
		// print( foo ); => fcn_type = ID, type = Block	: POP
		// bar()(); => fcn_type = ID, type = Call		: NO POP
		// 			=> fcn_type = Call, type = Call		: POP
		// a.fcn()	=> fcn_type = DOT_OP, type = Call	: POP

		if( fcn_type == ID )
		{
			if( type != Call )
				Emit( op_pop );
		}
		else if( fcn_type == Call )
		{
			if( type == Call )
				Emit( op_pop );
		}
		else if( fcn_type == DOT_OP )
		{
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
}

// augmented assignment operators
void Compiler::AugmentedAssignOp(  pANTLR3_BASE_TREE lhs_node, Opcode op, int line )
{
	// the immediate lhs always determines whether this is a simple store (an
	// ID) or a table store (Key/DOT_OP)
	unsigned int type = lhs_node->getType( lhs_node );

	EmitLineNum( line );

	// is the lhs node an identifier?
	if( type == ID )
	{
		// get the text
		char* lhs = (char*)lhs_node->getText( lhs_node )->chars;

		// is it a local?
		int idx = CurrentScope()->ResolveLocalToIndex( lhs );
		// has the local been generated yet?
		if( idx != -1 && !CurrentScope()->HasLocalBeenGenerated( idx ) )
			idx = -1;

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

// Key ('[]') op
void Compiler::KeyOp( bool is_lhs_of_assign, int num_children, pANTLR3_BASE_TREE parent )
{
	// do nothing for left-hand side of assign,
	// assignment op will take care of generating the tbl_store
	if( is_lhs_of_assign )
		return;

	// index or slice?

	if( num_children == 1 )
	{
		// (Key ops never generate method_load ops)
		Emit( op_tbl_load );
	}
	else if( num_children == 2 )
	{
		// 2 arg slice
		Emit( op_loadslice2 );
	}
	else if( num_children == 3 )
	{
		// 3 arg slice
		Emit( op_loadslice3 );
	}
}

// '$' op in a index or slice
void Compiler::EndOp()
{
	Emit( op_push_null );
}

// Dot ('.') op
void Compiler::DotOp( bool is_lhs_of_assign, pANTLR3_BASE_TREE rhs, pANTLR3_BASE_TREE parent, int line )
{
	// do nothing for left-hand side of assign,
	// assignment op will take care of generating the tbl_store
	if( is_lhs_of_assign )
		return;

	EmitLineNum( line );

	// if the parent is a call op to what might be a method, 
	// generate a method_load op
	unsigned int type = 0;
	if( parent )
		type = parent->getType( parent );
	if( type == Call )
	{
		// if the rhs is an identifier we can try to tell more
		unsigned int rhs_type = rhs->getType( rhs );
		if( rhs_type == ID )
		{
			Emit( op_method_load );
		}
		else
			Emit( op_method_load );
		// mark as method for the following call op generation
		is_method = true;
	}
	// otherwise a tbl_load op
	else
		Emit( op_tbl_load );

}

void Compiler::ReturnOp( int line, bool no_val /*= false*/ )
{
	EmitLineNum( line );

	// no return value in the code? force a 'return null;'
	if( no_val )
	{
		// if we're inside a 'for' loop we need to emit a pop instruction too, 
		// as for loops have a loop collection var on the stack
		if( InForLoop() )
			Emit( op_pop );
		Emit( op_push_null );
	}
	// code is returning a value
	else
	{
		// if we're inside a 'for' loop we need to emit a pop instruction too, 
		// as for loops have a loop collection var on the stack
		if( InForLoop() )
		{
			// the code to get the return value on the stack is already generated,
			// so we need to swap the tos and tos1 and then pop the loop
			// collection var
			Emit( op_swap );
			Emit( op_pop );
		}
	}

	dword scopecount = fcn_scope_stack.back();
	Emit( op_return, scopecount );
}

void Compiler::ContinueOp( int line )
{
	dword scopecount = loop_scope_stack.back();
	dword address = (dword)labelstack.back();
	EmitLineNum( line );
	Emit( op_exit_loop, address, scopecount );
}

void Compiler::BreakOp( int line )
{
	dword scopecount = loop_scope_stack.back();

	// save the location for back-patching
	// (to be double-safe, check to ensure we're in a loop...)
	if( loop_break_locations.size() == 0 )
		throw ICE( "Invalid break statement: Not inside a loop.." );

	EmitLineNum( line );
	Emit( op_exit_loop, (dword)-1, scopecount );
	loop_break_locations.back()->push_back( (dword)(is->Length() - (2 * sizeof(dword))) );
}


void Compiler::ImportOp( pANTLR3_BASE_TREE node, int line )
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
	int idx = GetConstant( Object( obj_symbol_name, (char*)modname.c_str() ) );
	// not found? error
	if( idx == -1 )
		throw ICE( boost::format( "Symbol '%1%' not found for 'import' instruction." ) % modname );
	EmitLineNum( line );
	Emit( op_import, (dword)idx );
}

// 'new'
void Compiler::NewOp( int line )
{
	// no-op
}

// vector and map creation ops
void Compiler::VecOp( pANTLR3_BASE_TREE node, int line )
{
	int num_children = node->getChildCount( node );
	EmitLineNum( line );
	Emit( op_new_vec, (dword)num_children );
}

void Compiler::MapOp( pANTLR3_BASE_TREE node, int line )
{
	int num_children = node->getChildCount( node );
	EmitLineNum( line );
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

void Compiler::CleanupEndLoop()
{
	loop_scope_stack.pop_back();

	// back-patch any 'break' statements
	// pop the loop from the loop stack
	vector<dword>* breaks = loop_break_locations.back();
	loop_break_locations.pop_back();
	for( vector<dword>::iterator it = breaks->begin(); it != breaks->end(); ++it )
	{
		// back-patch the instruction at the break to point to the current
		// location in the instruction stream (end of this loop)
		is->Set( *it, (dword)is->Length() );
	}
	delete breaks;
}

void Compiler::WhileOpEnd()
{
	// emit the jump-to-start
	Emit( op_jmp, (dword)-1 );
	AddPatchLoc();
	BackpatchToLastLabel();
			
	// back-patch the conditional jump
	BackpatchToCur();

	CleanupEndLoop();
}

void Compiler::InOp( char* key, char* val, pANTLR3_BASE_TREE container, int line )
{
	// if 'val' is NULL this is a vector/single-loop-var iteration

	// start a new count of scopes inside this loop
	loop_scope_stack.push_back( 0 );

	// start a new loop context for back-patching 'break' statements
	loop_break_locations.push_back( new vector<dword>() );

	// is the key a local?
	int key_idx = CurrentScope()->ResolveLocalToIndex( key );

	// is the val a local?
	int val_idx = -1;
	if( val )
		val_idx = CurrentScope()->ResolveLocalToIndex( val );

	// not found? error out, for loop vars are always locals
	if( key_idx == -1 || (val && val_idx == -1) )
	{
		throw ICE( boost::format( "For loop variable '%1%' not found in the local symbols." ) % key );
	}

	EmitLineNum( line );

	// the vector to iterate is on top of the stack, dup it for the calls to
	// 'rewind' and 'next'
	Emit( op_dup1 );
	
	// generate the call to 'rewind'
	int rewind_idx = GetConstant( Object( obj_symbol_name, const_cast<char*>("rewind") ) );
	// not found? error
	if( rewind_idx == -1 )
		throw ICE( "Non-local symbol 'rewind' not found in Compiler::InOp()." );
	Emit( op_pushconst, (dword)rewind_idx );
	Emit( op_method_load );
	Emit( op_call_method, 0 );
	Emit( op_pop );	// pop the unused return value from 'rewind'

	// vector to iterate is now on top of the stack again

	// mark the label for the loop beginning
	AddLabel();

	// generate the for_iter instruction
	if( !val )
		Emit( op_for_iter, (dword)-1 );
	else
		Emit( op_for_iter_pair, (dword)-1 );

	// mark it for back-patching
	AddPatchLoc();

	// the val & key are now on the stack, store them into the local loop vars
	// val:
	// index under 10: use the short instructions
	if( val )
	{
		// mark the local as having been defined in this scope
		CurrentScope()->SetLocalGenerated( val_idx );

		if( val_idx < 10 )
		{
			switch( val_idx )
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
			Emit( op_def_local, (dword)val_idx );
	}

	// key:
	// index under 10: use the short instructions

	// mark the local as having been defined in this scope
	CurrentScope()->SetLocalGenerated( key_idx );

	if( key_idx < 10 )
	{
		switch( key_idx )
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
		Emit( op_def_local, (dword)key_idx );

	// loop body here
}

// vectors
void Compiler::InOp( char* key, pANTLR3_BASE_TREE container, int line )
{
	InOp( key, NULL, container, line );
}

void Compiler::ForOpEnd()
{
	// emit the jump-to-start
	Emit( op_jmp, (dword)-1 );
	AddPatchLoc();
	BackpatchToLastLabel();

	// backpatch the for_iter instruction with this (loop complete) location
	BackpatchToCur();

	// generate break jumps to here (*prior* to the pop which removes the excess
	// item from the stack!!)
	CleanupEndLoop();

	// pop the remaining iterable item off the stack
	Emit( op_pop );
}


} // namespace deva_compile
