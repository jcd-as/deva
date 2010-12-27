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

#include <iostream>

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
	in_class( false )
{
	// create the instruction stream
	is = new InstructionStream();

	// all global names and constants MUST be added here, as the compiler needs
	// to generate and use indices into their collections, and adding items to
	// the sorted collections will invalidate the indices already used

	// copy the global names and consts from the Semantics pass/object to the executor
	// (locals will be added as the compiler gets to each fcn declaration, see DefineFun)
	// constants
	for( set<DevaObject>::iterator i = sem->constants.begin(); i != sem->constants.end(); ++i )
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
			if( !ex->AddConstant( DevaObject( s ) ) )
				delete [] s;
		}
		else
			ex->AddConstant( *i );
	}

	// add our 'global' function, "@main"
	FunctionScope* scope = dynamic_cast<FunctionScope*>(sem->global_scope);
	DevaFunction* f = new DevaFunction();
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

// disassemble the instruction stream to stdout
void Compiler::Decode()
{
	const byte* b = is->Bytes();
	const byte* p = b;
	size_t len = is->Length();
	dword arg, arg2;
	Opcode op;
	DevaObject o;

	cout << "Instructions:" << endl;
	while( (p - b) < len )
	{
		// decode opcode
		op = (Opcode)*p;
		cout << (int)(p - b) << ": " <<  opcodeNames[op] << "\t";
		p++;
		switch( op )
		{
		case op_nop:
		case op_pop:
			break;
		case op_push:
			// 1 arg
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_push_true:
		case op_push_false:
		case op_push_null:
		case op_push_zero:
		case op_push_one:
		case op_push0:
		case op_push1:
		case op_push2:
		case op_push3:
			break;
		case op_pushlocal:
			// 1 arg
			p += sizeof( dword );
			break;
		case op_pushlocal0:
		case op_pushlocal1:
		case op_pushlocal2:
		case op_pushlocal3:
		case op_pushlocal4:
		case op_pushlocal5:
		case op_pushlocal6:
		case op_pushlocal7:
		case op_pushlocal8:
		case op_pushlocal9:
			break;
		case op_pushconst:
			// 1 arg: index to constant
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_store:
			// 1 arg
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_store_true:
			// 1 arg
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_store_false:
			// 1 arg
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_store_null:
			// 1 arg
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_storelocal:
			// 1 arg
			p += sizeof( dword );
			break;
		case op_storelocal0:
		case op_storelocal1:
		case op_storelocal2:
		case op_storelocal3:
		case op_storelocal4:
		case op_storelocal5:
		case op_storelocal6:
		case op_storelocal7:
		case op_storelocal8:
		case op_storelocal9:
			break;
		case op_new_map:
			// 1 arg: size
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_new_vec:
			// 1 arg: size
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_new_class:
			// 1 arg: size
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_new_instance:
			// 1 arg: size
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_jmp:
			// 1 arg: size
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_jmpf:
			// 1 arg: size
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_eq:
		case op_neq:
		case op_lt:
		case op_lte:
		case op_gt:
		case op_gte:
		case op_or:
		case op_and:
		case op_neg:
		case op_not:
		case op_add:
		case op_sub:
		case op_mul:
		case op_div:
		case op_mod:
			break;
		case op_add_assign:
		case op_sub_assign:
		case op_mul_assign:
		case op_div_assign:
		case op_mod_assign:
			// 1 arg: lhs
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_add_assign_local:
		case op_sub_assign_local:
		case op_mul_assign_local:
		case op_div_assign_local:
		case op_mod_assign_local:
			// 1 arg: lhs
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_call:
			// 1 arg: number of args passed
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_return:
			// 1 arg: number of scopes to leave
			arg = *((dword*)p);
			cout << "\t\t" << arg;
			p += sizeof( dword );
			break;
		case op_exit_loop:
			// 2 args: jump target address, number of scopes to leave
			arg = *((dword*)p);
			p += sizeof( dword );
			arg2 = *((dword*)p);
			p += sizeof( dword );
			cout << "\t\t" << arg << "\t" << arg2;
			break;
		case op_enter:
		case op_leave:
			break;
		case op_for_iter:
			// 1 arg: iterable object
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_tbl_load:
		case op_loadslice2:
		case op_loadslice3:
		case op_tbl_store:
		case op_storeslice2:
		case op_storeslice3:
		case op_add_tbl_store:
		case op_sub_tbl_store:
		case op_mul_tbl_store:
		case op_div_tbl_store:
		case op_mod_tbl_store:
		case op_dup:
		case op_dup1:
		case op_dup2:
		case op_dup3:
		case op_dup_top_n:
		case op_dup_top1:
		case op_dup_top2:
		case op_dup_top3:
		case op_swap:
			break;
		case op_rot:
			// 1 arg:
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_rot2:
		case op_rot3:
		case op_rot4:
		case op_import:
			// 1 arg:
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_halt:
			break;
		case op_illegal:
		default:
			cout << "Error: Invalid instruction.";
			break;
		}
		cout << endl;
	}
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

	// create a new DevaFunction object
	DevaFunction* fcn = new DevaFunction();
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
		DevaObject obj = scope->GetDefaultArgVals().At( i );
		if( obj.type == obj_string )
		{
			// strip the string of quotes and unescape it
			string str( obj.s );
			str = unescape( strip_quotes( str ) );
			char* s = new char[str.length()+1];
			strcpy( s, str.c_str() );
			idx = GetConstant( DevaObject( s ) );
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
	// generate a 'return' statement, in case there isn't one that will be hit
	// (check the immediately preceding instruction so we at least don't
	// generate two returns in a row...)
	if( is->Length() <= 5 || (Opcode)*(is->Current()-5) != op_return )
		Emit( op_return, 0 );

	// back-patch the jump over fcn body
	BackpatchToCur();
}

// define a class
void Compiler::DefineClass( char* name, int line )
{
	// TODO: 
	// - create the map for the class
	// - add the __name__, __class__, __module__ and __bases__ members
	// 
	// - if new and delete methods aren't defined for this class, we need to add
	// them and generate code for them (that calls the base-class new/delete
	// methods)
}

// constants
void Compiler::Number( pANTLR3_BASE_TREE node )
{
	double d = atof( (char*)node->getText(node)->chars );

	// negate numbers that should be negative
	unsigned int type = node->getType( node );
	if( type == NUMBER )
	{
		// check for negate flag from pass one
		if( (int)(node->u) == 0x1 )
			d *= -1.0;
	}
	else
		throw DevaICE( "Expecting a number type." );

	if( d == 0.0 )
	{
		Emit( op_push_zero );
	}
	else if( d == 1.0 )
	{
		Emit( op_push_one );
	}
	else
	{
		// get the constant pool index for this number
		int idx = GetConstant( DevaObject( d ) );
		if( idx < 0 )
			throw DevaICE( boost::format( "Cannot find constant '%1%'." ) % d );
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
	int idx = GetConstant( DevaObject( s ) );
	if( idx < 0 )
	{
		delete s;
		throw DevaICE( boost::format( "Cannot find constant '%1%'." ) % s );
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
		idx = GetConstant( DevaObject( s ) );
		if( idx < 0 )
			throw DevaICE( boost::format( "Cannot find constant '%1%'." ) % s );

		Emit( op_push, (dword)idx );
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
		throw DevaICE( boost::format( "Cannot locate local symbol '%1%'." ) % n );
	// emit a 'op_storelocalN' op
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

void Compiler::ExternVar( char* n, bool is_assign )
{
	// find the name in the constant pool
	int idx = GetConstant( DevaObject( n ) );
	// not found? error
	if( idx == -1 )
		throw DevaICE( boost::format( "Cannot find constant '%1%'." ) % n );

	// if this is an assignment, generate a store op
	if( is_assign )
		Emit( op_store, (dword)idx );
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
			idx = GetConstant( DevaObject( lhs ) );
			// not found? error
			if( idx == -1 )
				throw DevaICE( boost::format( "Non-local symbol '%1%' not found." ) % lhs );

			Emit( op_store, (dword)idx );
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
void Compiler::CallOp( pANTLR3_BASE_TREE fcn, pANTLR3_BASE_TREE args )
{
	// how many args are being pushed?
	int num_children = args->getChildCount( args );
	// emit the call
	Emit( op_call, (dword)num_children );
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
			int idx = GetConstant( DevaObject( lhs ) );
			// not found? error
			if( idx == -1 )
				throw DevaICE( boost::format( "Non-local symbol '%1%' not found." ) % lhs );

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
				throw DevaICE( "Unknown augmented assign operator." );
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
				throw DevaICE( "Unknown augmented assign operator." );
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
			throw DevaICE( "Unknown augmented assign operator." );
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
		throw DevaICE( "Invalid break statement: Not inside a loop.." );

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
	int idx = GetConstant( DevaObject( (char*)modname.c_str() ) );
	// not found? error
	if( idx == -1 )
		throw DevaICE( boost::format( "Symbol '%1%' not found." ) % modname );
	Emit( op_import, (dword)idx );
}

// 'new'
void Compiler::NewOp()
{
	// generate a new_class op for the call to 'new' to use
	Emit( op_new_class );
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

} // namespace deva_compile
