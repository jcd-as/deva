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


/////////////////////////////////////////////////////////////////////////////
// compilation functions and globals
/////////////////////////////////////////////////////////////////////////////
Compiler* compiler = NULL;


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
		ex->AddConstant( *i );
	}

	// add our 'global' function, "@main"
	FunctionScope* scope = dynamic_cast<FunctionScope*>(sem->global_scope);
	DevaFunction f;
	f.name = string( "@main" );
	f.filename = string( current_file );
	f.first_line = 0;
	f.num_args = 0;
	f.num_locals = scope->NumLocals();
	f.addr = 0;
	// add the global names for the global scope
	for( int i = 0; i < scope->GetNames().Size(); i++ )
	{
		Symbol* s = scope->GetNames().At( i );
		if( s->IsLocal() || s->IsArg() || s->IsConst() )
			f.local_names.Add( s->Name() );
	}
	ex->functions.Add( f );

	// set-up the scope stack to match
	AddScope();
}

// disassemble the instruction stream to stdout
void Compiler::Decode()
{
	const byte* b = is->Bytes();
	const byte* p = b;
	size_t len = is->Length();
	dword arg;
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
			// ???
			break;
		case op_new_vec:
			// ???
			break;
		case op_new_class:
			// ???
			break;
		case op_new_instance:
			// ???
			break;
		case op_jmp:
			// 1 arg: size
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_jmpf:
			// 1 arg: size
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
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
		case op_call:
			break;
		case op_call_n:
			// 1 arg: fcn name
			arg = *((dword*)p);
			// look-up the constant
			o = ex->GetConstant(arg);
			cout << "\t\t" << arg << " (" << o << ")";
			p += sizeof( dword );
			break;
		case op_return:
		case op_break:
		case op_continue:
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
		case op_print:
		case op_printline:
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
	// track the current scope
	AddScope();
	// emit an enter instruction
	Emit( op_enter );
}
void Compiler::ExitBlock()
{
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

	FunctionScope* scope = dynamic_cast<FunctionScope*>(CurrentScope());

	// create a new DevaFunction object
	DevaFunction fcn;
	fcn.name = string( name );
	if( classname )
	{
		fcn.name += "@";
		fcn.name += classname;
	}
	fcn.filename = string( current_file );
	fcn.first_line = line;
	fcn.num_args = scope->NumArgs();
	fcn.num_locals = scope->NumLocals();
	// set the code address for this function
	fcn.addr = is->Length();

	// add the global names for this (function) scope and its local scope children
	for( int i = 0; i < scope->GetNames().Size(); i++ )
	{
		Symbol* s = scope->GetNames().At( i );
		if( s->IsLocal() || s->IsArg() || s->IsConst() )
			fcn.local_names.Add( s->Name() );
	}
	// add the default arg indices for this (function) scope
	for( int i = 0; i < scope->GetDefaultArgVals().Size(); i++ )
	{
		// find the index for this constant
		int idx = GetConstant( scope->GetDefaultArgVals().At( i ) );
		fcn.default_args.Add( idx );
	}

	// add to the list of fcn objects
	ex->AddFunction( fcn );
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
void Compiler::Number( double d )
{
	// get the constant pool index for this number
	int idx = GetConstant( DevaObject( d ) );
	if( idx < 0 )
		throw DevaICE( boost::format( "Cannot find constant '%1%'." ) % d );
	// emit op to push it onto the stack
	Emit( op_pushconst, (dword)idx );
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
		throw DevaICE( boost::format( "Cannot find constant '%1%'." ) % s );
	// emit op to push it onto the stack
	Emit( op_pushconst, (dword)idx );
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
	unsigned int type = fcn->getType( fcn );
	// is the node an identifier? must be the fcn name
	if( type == ID )
	{
		// find the name in the constant pool
		char* name = (char*)fcn->getText( fcn )->chars;
		int idx = GetConstant( DevaObject( name ) );
		// not found? error
		if( idx == -1 )
			throw DevaICE( boost::format( "Function '%1%' not found." ) % name );
		Emit( op_call_n, (dword)idx );
	}
	// otherwise it must be an object of some kind, we need to rotate the stack
	// and call it
	else
	{
		// TODO: if it is a method there will be an extra 'self' arg!

		// rotate by the number of args
		int num_children = args->getChildCount( args );
		if( num_children == 1 )
		{
			Emit( op_swap );
		}
		else if( num_children == 2 )
		{
			Emit( op_rot2 );
		}
		else if( num_children == 3 )
		{
			Emit( op_rot3 );
		}
		else if( num_children == 4 )
		{
			Emit( op_rot4 );
		}
		else if( num_children > 0 )
		{
			Emit( op_rot, (dword)num_children );
		}
		Emit( op_call );
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

	Emit( op_return );
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
	// generate a new_map op for the call to 'new' to use
	Emit( op_new_map );
}

