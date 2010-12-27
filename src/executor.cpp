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

// executor.cpp
// executive/VM global object/functions for the deva language
// created by jcs, december 16, 2010 


#include "executor.h"
#include "error.h"

#include <algorithm>

using namespace std;


namespace deva
{

/////////////////////////////////////////////////////////////////////////////
// executive/VM functions and globals
/////////////////////////////////////////////////////////////////////////////

// global executor object
Executor* ex;

// TODO: move this to a builtins h/cpp file:
void print( Frame* frame )
{
	// number of args passed
	int args_passed = frame->NumArgsPassed();

	const char *separator;
	if( args_passed == 1 )
		separator = "\n";
	else if( args_passed == 2 )
	{
		// get a string arg off the stack
		DevaObject sep = frame->GetLocal( 1 );
		// TODO: safe to use this string? who owns the memory?
		separator = sep.s;
	}
	else
		throw DevaRuntimeException( "Too many arguments passed to print() builtin." );

	// get the string arg from the frame
	DevaObject o = frame->GetLocal( 0 );

	// print the string + separator
	cout << o << separator;

	// push null onto the stack (retval)
	ex->PushStack( DevaObject( obj_null ) );
}

Executor::Executor()
{
	// set-up the constant and function object pools
	constants.Reserve( 256 );

	// TODO: are these needed?? we have opcodes to push these constants
	// directly...
	// add the "perpetual constants":
	constants.Add( DevaObject( true ) );
	constants.Add( DevaObject( false ) );
	constants.Add( DevaObject( obj_null ) );

	// make sure the global scope is always around
	scopes.PushScope( new Scope() );

	// TODO: move this outside of here
	AddBuiltin( "print", print );
}

Executor::~Executor()
{
	// free the constants' string data
	for( int i = 0; i < constants.Size(); i++ )
	{
		if( constants.At( i ).type == obj_string ) delete [] constants.At( i ).s;
	}
	// free the function objects
	for( map<string, DevaFunction*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		delete i->second;
	}
}

void Executor::AddBuiltin( const char* name, NativeFunction fcn )
{
	DevaObject* o = new DevaObject( fcn );
	GlobalScope()->AddObject( name, o );
	builtins.insert( pair<string,NativeFunction>( string( name ), fcn ) );
}

void Executor::ExecuteCode( const Code & code )
{
	// NOTE: all actions that create a new C string (char*) as a local need to
	// add it to that frame's strings collection (i.e. call
	// CurrentFrame()->AddString())

	byte *base = code.code;
	byte *ip = code.code;
	byte *end = code.code + code.len;
	dword arg, arg2;
	Opcode op = op_nop;
	DevaObject o, lhs, rhs, *plhs;

	// TODO: review. is this where this frame should be added?
	// where does it get deleted?
	// a starting ('global') frame
	DevaFunction *main = FindFunction( string( "@main" ) );
	Frame* frame = new Frame( (dword)base, 0, main );
	PushFrame( frame );

	while( ip < end && op < op_halt )
	{
		// decode opcode
		op = (Opcode)*ip;
		ip++;
		switch( op )
		{
		case op_nop:
			break;
		case op_pop:
			stack.pop_back();
			break;
		case op_push:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			stack.push_back( GetConstant(arg) );
			ip += sizeof( dword );
			break;
		case op_push_true:
			stack.push_back( DevaObject( true ) );
			break;
		case op_push_false:
			stack.push_back( DevaObject( false ) );
			break;
		case op_push_null:
			stack.push_back( DevaObject( obj_null ) );
			break;
		case op_push_zero:
			stack.push_back( DevaObject( 0.0 ) );
			break;
		case op_push_one:
			stack.push_back( DevaObject( 1.0 ) );
			break;
		case op_push0:
			stack.push_back( GetConstant( 0 ) );
			break;
		case op_push1:
			stack.push_back( GetConstant( 1 ) );
			break;
		case op_push2:
			stack.push_back( GetConstant( 2 ) );
			break;
		case op_push3:
			stack.push_back( GetConstant( 3 ) );
			break;
		case op_pushlocal:
			// 1 arg
			ip += sizeof( dword );
			break;
		case op_pushlocal0:
			stack.push_back( CurrentFrame()->GetLocal( 0 ) );
			break;
		case op_pushlocal1:
			stack.push_back( CurrentFrame()->GetLocal( 1 ) );
			break;
		case op_pushlocal2:
			stack.push_back( CurrentFrame()->GetLocal( 2 ) );
			break;
		case op_pushlocal3:
			stack.push_back( CurrentFrame()->GetLocal( 3 ) );
			break;
		case op_pushlocal4:
			stack.push_back( CurrentFrame()->GetLocal( 4 ) );
			break;
		case op_pushlocal5:
			stack.push_back( CurrentFrame()->GetLocal( 5 ) );
			break;
		case op_pushlocal6:
			stack.push_back( CurrentFrame()->GetLocal( 6 ) );
			break;
		case op_pushlocal7:
			stack.push_back( CurrentFrame()->GetLocal( 7 ) );
			break;
		case op_pushlocal8:
			stack.push_back( CurrentFrame()->GetLocal( 8 ) );
			break;
		case op_pushlocal9:
			stack.push_back( CurrentFrame()->GetLocal( 9 ) );
			break;
		case op_pushconst:
			// 1 arg: index to constant
			arg = *((dword*)ip);
			stack.push_back( GetConstant( arg ) );
			ip += sizeof( dword );
			break;
		case op_store:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			// TODO: free lhs before assigning to it??
			*plhs = rhs;
			ip += sizeof( dword );
			break;
		case op_store_true:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant(arg);
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			// TODO: free lhs before assigning to it??
			*plhs = DevaObject( true );
			ip += sizeof( dword );
			break;
		case op_store_false:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant(arg);
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			// TODO: free lhs before assigning to it??
			*plhs = DevaObject( true );
			ip += sizeof( dword );
			break;
		case op_store_null:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant(arg);
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			// TODO: free lhs before assigning to it??
			*plhs = DevaObject( obj_null );
			ip += sizeof( dword );
			break;
		case op_storelocal:
			// 1 arg
			arg = *((dword*)ip);
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( arg, rhs );
			ip += sizeof( dword );
			break;
		case op_storelocal0:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 0, rhs );
			break;
		case op_storelocal1:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 1, rhs );
			break;
		case op_storelocal2:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 2, rhs );
			break;
		case op_storelocal3:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 3, rhs );
			break;
		case op_storelocal4:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 4, rhs );
			break;
		case op_storelocal5:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 5, rhs );
			break;
		case op_storelocal6:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 6, rhs );
			break;
		case op_storelocal7:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 7, rhs );
			break;
		case op_storelocal8:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 8, rhs );
			break;
		case op_storelocal9:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			CurrentFrame()->SetLocal( 9, rhs );
			break;
			break;
		case op_new_map:
			// TODO:
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			break;
		case op_new_vec:
			// TODO:
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			break;
		case op_new_class:
			// TODO:
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			break;
		case op_new_instance:
			// TODO:
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			break;
		case op_jmp:
			// 1 arg: size
			arg = *((dword*)ip);
			ip = (byte*)(base + arg);
			break;
		case op_jmpf:
			// 1 arg: size
			arg = *((dword*)ip);
			o = stack.back();
			stack.pop_back();
			if( !o.CoerceToBool() )
				ip = (byte*)(base + arg);
			else
				ip += sizeof( dword );
			break;
		case op_eq:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Equality operator used on operands of different types." );
			switch( lhs.type )
			{
			case obj_null: stack.push_back( DevaObject( rhs.type == obj_null ) ); break;
			case obj_boolean: stack.push_back( DevaObject( lhs.b == rhs.b ) ); break;
			case obj_number: stack.push_back( DevaObject( lhs.b == rhs.b ) ); break;
			case obj_string: stack.push_back( DevaObject( strcmp( lhs.s, rhs.s ) == 0 ) ); break;
			case obj_vector: stack.push_back( DevaObject( lhs.v == rhs.v ) ); break;
			case obj_map:
			case obj_class:
			case obj_instance:
				stack.push_back( DevaObject( lhs.m == rhs.m ) ); break;
			case obj_function: stack.push_back( DevaObject( lhs.f == rhs.f ) ); break;
			case obj_native_function: stack.push_back( DevaObject( lhs.nf == rhs.nf ) ); break;
			case obj_native_obj: stack.push_back( DevaObject( lhs.no == rhs.no ) ); break;
			case obj_size: stack.push_back( DevaObject( lhs.sz == rhs.sz ) ); break;
			}
			break;
		case op_neq:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Inequality operator used on operands of different types." );
			switch( lhs.type )
			{
			case obj_null: stack.push_back( DevaObject( rhs.type != obj_null ) ); break;
			case obj_boolean: stack.push_back( DevaObject( lhs.b != rhs.b ) ); break;
			case obj_number: stack.push_back( DevaObject( lhs.d != rhs.d ) ); break;
			case obj_string: stack.push_back( DevaObject( strcmp( lhs.s, rhs.s ) != 0 ) ); break;
			case obj_vector: stack.push_back( DevaObject( lhs.v != rhs.v ) ); break;
			case obj_map:
			case obj_class:
			case obj_instance:
				stack.push_back( DevaObject( lhs.m != rhs.m ) ); break;
			case obj_function: stack.push_back( DevaObject( lhs.f != rhs.f ) ); break;
			case obj_native_function: stack.push_back( DevaObject( lhs.nf != rhs.nf ) ); break;
			case obj_native_obj: stack.push_back( DevaObject( lhs.no != rhs.no ) ); break;
			case obj_size: stack.push_back( DevaObject( lhs.sz != rhs.sz ) ); break;
			}
			break;
		case op_lt:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Less-than operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( DevaObject( lhs.d < rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( DevaObject( strcmp( lhs.s, rhs.s ) < 0 ) );
			else
				throw DevaRuntimeException( "Operands to less-than operator must be numbers or strings." );
			break;
		case op_lte:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Less-than-or-equals operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( DevaObject( lhs.d <= rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( DevaObject( strcmp( lhs.s, rhs.s ) <= 0 ) );
			else
				throw DevaRuntimeException( "Operands to less-than-or-equals operator must be numbers or strings." );
			break;
		case op_gt:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Greater-than operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( DevaObject( lhs.d > rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( DevaObject( strcmp( lhs.s, rhs.s ) > 0 ) );
			else
				throw DevaRuntimeException( "Operands to greater-than operator must be numbers or strings." );
			break;
		case op_gte:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Greater-than-or-equals operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( DevaObject( lhs.d >= rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( DevaObject( strcmp( lhs.s, rhs.s ) >= 0 ) );
			else
				throw DevaRuntimeException( "Operands to greater-than-or-equals operator must be numbers or strings." );
			break;
		case op_or:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			stack.push_back( DevaObject( lhs.CoerceToBool() || rhs.CoerceToBool() ) );
			break;
		case op_and:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			stack.push_back( DevaObject( lhs.CoerceToBool() && rhs.CoerceToBool() ) );
			break;
		case op_neg:
			o = stack.back();
			stack.pop_back();
			if( o.type != obj_number )
				throw DevaRuntimeException( "Negate operator can only be used on numeric objects." );
			stack.push_back( DevaObject( -o.d ) );
			break;
		case op_not:
			o = stack.back();
			stack.pop_back();
			stack.push_back( DevaObject( !o.CoerceToBool() ) );
			break;
		case op_add:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number && lhs.type != obj_string )
				throw DevaRuntimeException( "Left-hand side of addition operator must be a number or a string." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of addition operator must be a number or a string." );
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Addition operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( DevaObject( lhs.d + rhs.d ) );
			else if( lhs.type == obj_string )
			{
				size_t len = strlen( lhs.s ) + strlen( rhs.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, lhs.s );
				strcat( ret, rhs.s );
				CurrentFrame()->AddString( ret );
				stack.push_back( DevaObject( ret ) ); 
			}
			break;
		case op_sub:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of subtraction operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of subtraction operator must be a number." );
			stack.push_back( DevaObject( lhs.d - rhs.d ) );
			break;
		case op_mul:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of multiplication operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of multiplication operator must be a number." );
			stack.push_back( DevaObject( lhs.d * rhs.d ) );
			break;
		case op_div:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of division operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of division operator must be a number." );
			if( rhs.d == 0.0 )
				throw DevaRuntimeException( "Division by zero fault." );
			stack.push_back( DevaObject( lhs.d / rhs.d ) );
			break;
		case op_mod:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of modulus operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of modulus operator must be a number." );
			if( rhs.d == 0.0 )
				throw DevaRuntimeException( "Division by zero fault." );
			// TODO: error if arguments aren't integral numbers...
			stack.push_back( DevaObject( (double)((int)lhs.d % (int)rhs.d) ) );
			break;
		case op_add_assign: // add <Op0> and tos and store back into <Op0>
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// TODO: this needs to look for locals first!!
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number && plhs->type != obj_string )
				throw DevaRuntimeException( "Left-hand side of addition assignment operator must be a number or a string." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of addition assignment operator must be a number or a string." );
			if( plhs->type != rhs.type )
				throw DevaRuntimeException( "Addition assignment operator used on operands of different types." );
			if( plhs->type == obj_number )
				*plhs = DevaObject( plhs->d + rhs.d );
			else if( plhs->type == obj_string )
			{
				size_t len = strlen( plhs->s ) + strlen( rhs.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, plhs->s );
				strcat( ret, rhs.s );
				CurrentFrame()->AddString( ret );
				*plhs = DevaObject( ret );
			}
			ip += sizeof( dword );
			break;
		case op_sub_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// TODO: this needs to look for locals first!!
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw DevaRuntimeException( "Left-hand side of subtraction assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of subtraction assignment operator must be a number." );
			*plhs = DevaObject( plhs->d - rhs.d );
			ip += sizeof( dword );
			break;
		case op_mul_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// TODO: this needs to look for locals first!!
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw DevaRuntimeException( "Left-hand side of multiplication assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of multiplication assignment operator must be a number." );
			*plhs = DevaObject( plhs->d * rhs.d );
			ip += sizeof( dword );
			break;
		case op_div_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// TODO: this needs to look for locals first!!
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw DevaRuntimeException( "Left-hand side of division assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of division assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw DevaRuntimeException( "Divide-by-zero error." );
			*plhs = DevaObject( plhs->d / rhs.d );
			ip += sizeof( dword );
			break;
		case op_mod_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// TODO: this needs to look for locals first!!
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw DevaRuntimeException( "Left-hand side of modulus assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of modulus assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw DevaRuntimeException( "Divide-by-zero error." );
			// TODO: error if arguments aren't integral numbers...
			*plhs = DevaObject( (double)((int)plhs->d / (int)rhs.d) );
			ip += sizeof( dword );
			break;
		case op_add_assign_local:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the local
			lhs = CurrentFrame()->GetLocal( arg );
			rhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number && lhs.type != obj_string )
				throw DevaRuntimeException( "Left-hand side of addition assignment operator must be a number or a string." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw DevaRuntimeException( "Right-hand side of addition assignment operator must be a number or a string." );
			if( lhs.type != rhs.type )
				throw DevaRuntimeException( "Addition assignment operator used on operands of different types." );
			if( lhs.type == obj_number )
				CurrentFrame()->SetLocal( arg, DevaObject( lhs.d + rhs.d ) );
			else if( lhs.type == obj_string )
			{
				size_t len = strlen( lhs.s ) + strlen( rhs.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, lhs.s );
				strcat( ret, rhs.s );
				CurrentFrame()->AddString( ret );
				CurrentFrame()->SetLocal( arg, DevaObject( ret ) );
			}
			ip += sizeof( dword );
			break;
		case op_sub_assign_local:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the local
			lhs = CurrentFrame()->GetLocal( arg );
			// find the variable
			rhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of subtraction assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of subtraction assignment operator must be a number." );
			CurrentFrame()->SetLocal( arg, DevaObject( lhs.d - rhs.d ) );
			ip += sizeof( dword );
			break;
		case op_mul_assign_local:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the local
			lhs = CurrentFrame()->GetLocal( arg );
			rhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of multiplication assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of multiplication assignment operator must be a number." );
			CurrentFrame()->SetLocal( arg, DevaObject( lhs.d * rhs.d ) );
			ip += sizeof( dword );
			break;
		case op_div_assign_local:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the local
			lhs = CurrentFrame()->GetLocal( arg );
			if( lhs )
				throw DevaRuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw DevaRuntimeException( "Left-hand side of division assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of division assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw DevaRuntimeException( "Divide-by-zero error." );
			CurrentFrame()->SetLocal( arg, DevaObject( lhs.d / rhs.d ) );
			ip += sizeof( dword );
			break;
		case op_mod_assign_local:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the local
			lhs = CurrentFrame()->GetLocal( arg );
			rhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw DevaRuntimeException( "Left-hand side of modulus assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw DevaRuntimeException( "Right-hand side of modulus assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw DevaRuntimeException( "Divide-by-zero error." );
			// TODO: error if arguments aren't integral numbers...
			CurrentFrame()->SetLocal( arg, DevaObject( (double)((int)lhs.d % (int)rhs.d) ) );
			ip += sizeof( dword );
			break;
		case op_call: // call function with <Op0> args on on stack, fcn after args
			// 1 arg: number of args passed
			arg = *((dword*)ip);
			ip += sizeof( dword );
			o = *(stack.end() - arg - 1);
			// if it's a fcn, build a frame for it
			if( o.type == obj_function )
			{
				// create a frame for the fcn
				Frame* frame = new Frame( (dword)ip, (int)arg, o.f );
				// set the args for the frame
				for( int i = 0; i < arg; i++ )
				{
					DevaObject ob = stack.back();
					frame->SetLocal( arg-i-1, ob );
					stack.pop_back();
				}
				// TODO: this doesn't work when the # of default arg vals != #
				// of args
				// default arg vals...
				int num_defaults = o.f->num_args - arg;
				if( num_defaults != 0 )
				{
					for( int i = 0; i < num_defaults; i++ )
					{
						int idx = o.f->default_args.At( arg+i );
						DevaObject o = GetConstant( arg );
						frame->SetLocal( arg+i, o );
					}
				}
				// push the frame onto the callstack
				PushFrame( frame );
				// jump to the function
				// TODO: if addr relative to another code block (module)??
				ip = (byte*)(base + o.f->addr);
			}
			// is it a native fcn?
			else if( o.type == obj_native_function )
			{
				// create a frame for the fcn
				Frame* frame = new Frame( (dword)ip, (int)arg, o.f );
				// set the args for the frame
				for( int i = 0; i < arg; i++ )
				{
					DevaObject ob = stack.back();
					frame->SetLocal( arg-i-1, ob );
					stack.pop_back();
				}
				// push the frame onto the callstack
				PushFrame( frame );
				o.nf( frame );
				PopFrame();
			}
			// is it a const we need to look up?
			else if( o.type == obj_string )
			{
				// find the function
				DevaFunction* f = FindFunction( o.s );
				if( f )
				{
					// create a frame for the fcn
					Frame* frame = new Frame( (dword)ip, arg, f );
					// set the args for the frame
					for( int i = 0; i < arg; i++ )
					{
						DevaObject ob = stack.back();
						frame->SetLocal( arg-i-1, ob );
						stack.pop_back();
					}
					// default arg vals...
					int num_defaults = f->num_args - arg;
					if( num_defaults != 0 )
					{
						for( int i = 0; i < num_defaults; i++ )
						{
							int idx = f->default_args.At( arg+i );
							DevaObject ob = GetConstant( idx );
							frame->SetLocal( arg+i, ob );
						}
					}
					// push the frame onto the callstack
					PushFrame( frame );
					// jump to the function
					// TODO: if addr relative to another code block (module)??
					ip = (byte*)(base + f->addr);
				}
				else
				{
					NativeFunction nf = FindNativeFunction( o.s );
					if( nf )
					{
						// create a frame for the fcn
						Frame* frame = new Frame( (dword)ip, arg, nf );
						// set the args for the frame
						for( int i = 0; i < arg; i++ )
						{
							DevaObject ob = stack.back();
							frame->SetLocal( arg-i-1, ob );
							stack.pop_back();
						}
						// push the frame onto the callstack
						PushFrame( frame );
						nf( frame );
						PopFrame();
					}
					else
						throw DevaRuntimeException( "Invalid function name" );
				}
			}
			else
				throw DevaRuntimeException( "Object is not a function." );
			break;
		case op_return:

			// 1 arg: number of scopes to leave
			arg = *((dword*)ip);
			// leave the scopes
			for( int i = 0; i < arg; i++ )
				PopScope();
			// jump to the new frame's address
			ip = (byte*)CurrentFrame()->GetReturnAddress();
			// pop the current frame
			PopFrame();
			break;
		case op_exit_loop:
			// 2 args: jump target address, number of scopes to leave
			arg = *((dword*)ip);
			ip += sizeof( dword );
			arg2 = *((dword*)ip);
			ip += sizeof( dword );
			// leave the scopes
			for( int i = 0; i < arg2; i++ )
				PopScope();
			// jump out of loop
			ip = (byte*)(base + arg);
			break;
		case op_enter:
			PushScope( new Scope() );
			break;
		case op_leave:
			PopScope();
			break;
		case op_for_iter:
			// TODO:
			// 1 arg: iterable object
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant(arg);
			ip += sizeof( dword );
			break;
		case op_tbl_load:// tos = tos1[tos]
			// TODO:
		case op_loadslice2:
			// TODO:
		case op_loadslice3:
			// TODO:
		case op_tbl_store:// tos2[tos1] = tos
			// TODO:
		case op_storeslice2:
			// TODO:
		case op_storeslice3:
			// TODO:
		case op_add_tbl_store:
			// TODO:
		case op_sub_tbl_store:
			// TODO:
		case op_mul_tbl_store:
			// TODO:
		case op_div_tbl_store:
			// TODO:
		case op_mod_tbl_store:
			// TODO:
			break;
		case op_dup:
			stack.push_back( stack.back() );
			break;
		case op_dup1:
			stack.push_back( stack[stack.size()-2] );
			break;
		case op_dup2:
			stack.push_back( stack[stack.size()-3] );
			break;
		case op_dup3:
			stack.push_back( stack[stack.size()-3] );
			break;
		case op_dup_top_n:
			// TODO:
		case op_dup_top1:
			// TODO:
		case op_dup_top2:
			// TODO:
		case op_dup_top3:
			// TODO:
		case op_swap:
			// TODO:
			break;
		case op_rot:
			// TODO:
			// 1 arg:
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant(arg);
			ip += sizeof( dword );
			break;
		case op_rot2:
			// TODO:
		case op_rot3:
			// TODO:
		case op_rot4:
			// TODO:
		case op_import:
			// TODO:
			// 1 arg:
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant(arg);
			ip += sizeof( dword );
			break;
		case op_halt:
			break;
		case op_illegal:
		default:
			throw DevaICE( "Illegal instruction" );
			break;
		}
	}
	// TODO: review. is this where this frame should be added?
	PopFrame();
}

} // namespace deva
