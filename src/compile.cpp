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

#include "semantics.h"
#include "compile.h"

#include <iostream>


/////////////////////////////////////////////////////////////////////////////
// compilation functions and globals
/////////////////////////////////////////////////////////////////////////////
Compiler* compiler = NULL;


Compiler::Compiler( Executor* ex ) : 
	max_scope_idx( 0 ),
	num_locals( 0 ),
	fcn_nesting( 0 ),
	in_class( false )
{
	// create the instruction stream
	is = new InstructionStream();

	// copy the global names and consts from the Semantics pass/object to the executor
	// (locals will be added as the compiler gets to each fcn declaration, see DefineFun)
	// constants
	for( set<DevaObject>::iterator i = semantics->constants.begin(); i != semantics->constants.end(); ++i )
	{
		ex->AddConstant( *i );
	}
	// global names
	for( set<string>::iterator i = semantics->names.begin(); i != semantics->names.end(); ++i )
	{
		ex->AddName( *i );
	}

	// add our 'global' function, "@main"
	FunctionScope* scope = dynamic_cast<FunctionScope*>(semantics->global_scope);
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
		if( s->IsLocal() )
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
		case op_push:
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
			{
			// 1 arg: index to constant
			arg = *((dword*)p);
			p += sizeof( dword );
			// look-up the constant
			DevaObject o = ex->GetConstant(arg);
			if( o.type == obj_number )
				cout << "\t\t" << arg << " (" << o.d << ")";
			else if( o.type == obj_string )
				cout << "\t\t" << arg << " (" << o.s << ")";
			else
				throw DevaICE( "Invalid type in constant pool." );
			}
			break;
		case op_store:
			// 1 arg
			p += sizeof( dword );
			break;
		case op_store_true:
			// 1 arg
			p += sizeof( dword );
			break;
		case op_store_false:
			// 1 arg
			p += sizeof( dword );
			break;
		case op_store_null:
			// 1 arg
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
			p += sizeof( dword );
			break;
		case op_jmpf:
			// 1 arg: size
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
			// 1 arg: fcn name
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
			p += sizeof( dword );
			break;
		case op_tbl_load:
		case op_slice2:
		case op_slice3:
		case op_tbl_load_local:
		case op_slice2local:
		case op_slice3local:
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

// find/add constants to the constant data
int Compiler::GetConstant( double d )
{
	int idx = ex->FindConstant( DevaObject( d ) );
	return idx;
}

int Compiler::GetConstant( char* s )
{
	int idx = ex->FindConstant( DevaObject( s ) );
	return idx;
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
void Compiler::DefineFun( char* name, int line )
{
	FunctionScope* scope = dynamic_cast<FunctionScope*>(CurrentScope());

	// create a new DevaFunction object
	DevaFunction fcn;
	fcn.name = string( name );
	fcn.filename = string( current_file );
	fcn.first_line = line;
	fcn.num_args = scope->NumArgs();
	fcn.num_locals = scope->NumLocals();
	// set the code address for this function
	fcn.addr = is->Length();

	// add the global names for this (function) scope and its children
	for( int i = 0; i < scope->GetNames().Size(); i++ )
	{
		Symbol* s = scope->GetNames().At( i );
		if( s->IsLocal() )
			fcn.local_names.Add( s->Name() );
	}

	// add to the list of fcn objects
	ex->AddFunction( fcn );
	// and add the name to the list of constants
	ex->AddConstant( name );
}

// define a class
void Compiler::DefineClass( char* name, int line )
{
	// TODO: implement
	// add the name to the list of constants
	ex->AddConstant( name );
}

// constants
void Compiler::Number( double d )
{
	// get the constant pool index for this number
	int idx = GetConstant( d );
	if( idx < 0 )
		throw DevaICE( "Cannot create or find constant." );
	// emit op to push it onto the stack
	Emit( op_pushconst, (dword)idx );
}

void Compiler::String( char* s )
{
	// get the constant pool index for this number
	int idx = GetConstant( s );
	if( idx < 0 )
		throw DevaICE( "Cannot create or find constant." );
	// emit op to push it onto the stack
	Emit( op_pushconst, (dword)idx );
}

// assignments and variable decls
void Compiler::LocalVar( char* n )
{
	// get the index for the name
	int idx = CurrentScope()->ResolveLocalToIndex( n );
	if( idx == -1 )
		throw DevaICE( boost::format( "Cannot locate local symbol '%1%'." ) % n );
	// TODO: add it to the current function object ??
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


void Compiler::Assign( char* n )
{
	// is it a local?
	int idx = CurrentScope()->ResolveLocalToIndex( n );

	// TODO: no? look it up in the global names
	if( idx == -1 )
	{
	}
	// not found? add it to the global names
}

// set a default argument value
void Compiler::DefaultArgVal( pANTLR3_BASE_TREE node, bool negate /*= false*/ )
{
	// TODO: implement
	// needs to store the default value with the function object
}

void Compiler::DefaultArgId( pANTLR3_BASE_TREE node, bool negate /*= false*/ )
{
	// TODO: implement
	// needs to store the default value with the function object
}

