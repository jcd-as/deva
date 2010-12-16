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


// disassemble the instruction stream to stdout
void Compiler::Decode()
{
	const byte* b = is->Bytes();
	const byte* p = b;
	size_t len = is->Length();
	while( (p - b + 1) <= len )
	{
		// TODO: implement
//		DecodeOpcode( p );
	}
}

// add constants to the constant data
void Compiler::AddConstant( double d )
{
	// TODO: implement
}

void Compiler::AddConstant( char* s )
{
	// TODO: implement
}

// find index of constant
int Compiler::FindConstant( double d )
{
	// TODO: implement
}

int Compiler::FindConstant( char* s )
{
	// TODO: implement
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
	DevaFunction* fcn = new DevaFunction();
	fcn->name = string( name );
	fcn->filename = string( current_file );
	fcn->firstLine = line;
	fcn->numArgs = scope->NumArgs();
	fcn->numLocals = scope->NumLocals();
	// set the code address for this function
	fcn->addr = is->Length();

	// add all the external, undeclared and function call symbols for this scope and its children
	for( map<string, Symbol*>::iterator i = scope->GetNames().begin(); i != scope->GetNames().end(); ++i )
	{
		if( i->second->IsExtern() || i->second->IsUndeclared() || i->second->Type() == sym_function )
			fcn->names.insert( i->first );
	}

	// add to the list of fcn objects
	functions.push_back( fcn );
}

// define a class
void Compiler::DefineClass( char* name, int line )
{
	// TODO: implement
}


