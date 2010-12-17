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
#include "executor.h"

#include <iostream>


/////////////////////////////////////////////////////////////////////////////
// compilation functions and globals
/////////////////////////////////////////////////////////////////////////////
Compiler* compiler = NULL;


// disassemble the instruction stream to stdout
void Compiler::Decode()
{
	const byte* b = is->Bytes();
	const byte* p = b;
	size_t len = is->Length();
	dword arg;
	Opcode op;

	cout << "Instructions:" << endl;
	while( (p - b + 1) < len )
	{
		// decode opcode
		op = (Opcode)*p;
		cout << opcodeNames[op] << "\t";
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
	int idx = ex->AddConstant( DevaObject( d ) );
	return idx;
}

int Compiler::GetConstant( char* s )
{
	int idx = ex->AddConstant( DevaObject( s ) );
	return idx;
}

// block
void Compiler::EnterBlock()
{
	// track the current scope
	current_scope_idx++;
	// emit an enter instruction
	Emit( op_enter );
}
void Compiler::ExitBlock()
{
	// track the current scope
	current_scope_idx--;
	// emit a leave instruction
	Emit( op_leave );
}

// define a function
void Compiler::DefineFun( char* name, int line )
{
	FunctionScope* scope = dynamic_cast<FunctionScope*>(semantics->scopes[current_scope_idx]);

	// create a new DevaFunction object
	DevaFunction fcn;
	fcn.name = string( name );
	fcn.filename = string( current_file );
	fcn.firstLine = line;
	fcn.numArgs = scope->NumArgs();
	fcn.numLocals = scope->NumLocals();
	// set the code address for this function
	fcn.addr = is->Length();

	// add all the external, undeclared and function call symbols for this scope and its children
	for( map<string, Symbol*>::iterator i = scope->GetNames().begin(); i != scope->GetNames().end(); ++i )
	{
		if( i->second->IsExtern() || i->second->IsUndeclared() || i->second->Type() == sym_function )
			fcn.names.insert( i->first );
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
	// create an index for this local
	int idx = num_locals++;
	// TODO: map the name to the local
	//
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


