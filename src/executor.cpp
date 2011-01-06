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
#include "builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"

#include <cmath>
#include <algorithm>

using namespace std;


namespace deva
{

/////////////////////////////////////////////////////////////////////////////
// executive/VM functions and globals
/////////////////////////////////////////////////////////////////////////////

// global executor object
Executor* ex;

Executor::Executor() : ip( NULL ), debug( false ), trace( false )
{
	// set-up the constant and function object pools
	constants.Reserve( 256 );
}

Executor::~Executor()
{
	// free the constants' string data
	for( int i = 0; i < constants.Size(); i++ )
	{
		ObjectType type = constants.At( i ).type;
		if( type == obj_string || type == obj_symbol_name ) delete [] constants.At( i ).s;
	}
	// free the function objects
	for( map<string, Function*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		delete i->second;
	}
}

void Executor::AddBuiltins()
{
	for( int i = 0; i < num_of_builtins; i++ )
	{
		NativeFunction nf;
		nf.p = builtin_fcns[i];
		nf.is_method = false;
		AddBuiltin( builtin_names[i], nf );
	}
}

void Executor::AddBuiltin( const string name, NativeFunction fcn )
{
	builtins.insert( pair<string,NativeFunction>( name, fcn ) );
}

NativeFunction Executor::FindBuiltin( string name )
{
	map<string, NativeFunction>::iterator i = builtins.find( name );
	if( i == builtins.end() )
	{
		NativeFunction nf;
		nf.p = NULL;
		nf.is_method = false;
		return nf;
	}
	else
		return i->second;
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
	Object o, lhs, rhs, *plhs;
	double intpart; // for modf() calls

	// TODO: create the scopes here?
	scopes = new ScopeTable();

	// TODO: review. is this where this frame should be added?
	// (it needs the code location/address, for return values, so where else can
	// it go??)

	// a starting ('global') frame
	Function *main = FindFunction( string( "@main" ) );
	Frame* frame = new Frame( NULL, scopes, (dword)base, 0, main );
	PushFrame( frame );

	// make sure the global scope is always around
	scopes->PushScope( new Scope() );

	// add the builtin functions
	AddBuiltins();

	if( trace )
		cout << "Execution trace:" << endl;

	while( ip < end && op < op_halt )
	{
		// decode opcode
		op = (Opcode)*ip;

		if( trace )
		{
			PrintOpcode( op, base, ip );
			// print the top five stack items
			cout << "\t\t[";
			int n = (stack.size() > 5 ? 5 : stack.size());
			for( int i = n-1; i >= 0; i-- )
			{
				cout << stack[stack.size()-(n-i)];
				if( i > 0 )
					cout << ", ";
			}
			cout << "]" << endl;
		}

		ip++;
		switch( op )
		{
		case op_nop:
			break;
		case op_pop:
			stack.pop_back();
			break;
		case op_push:
			// push an integer value directly
			// 1 arg
			arg = *((dword*)ip);
			stack.push_back( Object( (double)arg ) );
			ip += sizeof( dword );
			break;
		case op_push_true:
			stack.push_back( Object( true ) );
			break;
		case op_push_false:
			stack.push_back( Object( false ) );
			break;
		case op_push_null:
			stack.push_back( Object( obj_null ) );
			break;
		case op_push_zero:
			stack.push_back( Object( 0.0 ) );
			break;
		case op_push_one:
			stack.push_back( Object( 1.0 ) );
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
		case op_storeconst:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = CurrentScope()->FindSymbol( o.s );
			if( !plhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			DecRef( *plhs );
			IncRef( rhs );
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
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			DecRef( *plhs );
			*plhs = Object( true );
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
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			DecRef( *plhs );
			*plhs = Object( true );
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
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			DecRef( *plhs );
			*plhs = Object( obj_null );
			ip += sizeof( dword );
			break;
		case op_storelocal:
			// 1 arg
			arg = *((dword*)ip);
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( arg ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( arg, rhs );
			ip += sizeof( dword );
			break;
		case op_storelocal0:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 0 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 0, rhs );
			break;
		case op_storelocal1:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 1 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 1, rhs );
			break;
		case op_storelocal2:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 2 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 2, rhs );
			break;
		case op_storelocal3:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 3 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 3, rhs );
			break;
		case op_storelocal4:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 4 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 4, rhs );
			break;
		case op_storelocal5:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 5 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 5, rhs );
			break;
		case op_storelocal6:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 6 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 6, rhs );
			break;
		case op_storelocal7:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 7 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 7, rhs );
			break;
		case op_storelocal8:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 8 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 8, rhs );
			break;
		case op_storelocal9:
			rhs = stack.back();
			stack.pop_back();
			DecRef( *CurrentFrame()->GetLocalRef( 9 ) );
			IncRef( rhs );
			// set the local in the current frame
			CurrentFrame()->SetLocal( 9, rhs );
			break;
		case op_def_local:
			{
			// 1 arg
			arg = *((dword*)ip);
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( arg, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( arg ), CurrentFrame()->GetLocalRef( arg ) );
			ip += sizeof( dword );
			}
			break;
		case op_def_local0:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 0, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 0 ), CurrentFrame()->GetLocalRef( 0 ) );
			break;
		case op_def_local1:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 1, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 1 ), CurrentFrame()->GetLocalRef( 1 ) );
			break;
		case op_def_local2:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 2, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 2 ), CurrentFrame()->GetLocalRef( 2 ) );
			break;
		case op_def_local3:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 3, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 3 ), CurrentFrame()->GetLocalRef( 3 ) );
			break;
		case op_def_local4:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 4, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 4 ), CurrentFrame()->GetLocalRef( 4 ) );
			break;
		case op_def_local5:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 5, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 5 ), CurrentFrame()->GetLocalRef( 5 ) );
			break;
		case op_def_local6:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 6, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 6 ), CurrentFrame()->GetLocalRef( 6 ) );
			break;
		case op_def_local7:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 7, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 7 ), CurrentFrame()->GetLocalRef( 7 ) );
			break;
		case op_def_local8:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 8, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 8 ), CurrentFrame()->GetLocalRef( 8 ) );
			break;
		case op_def_local9:
			rhs = stack.back();
			stack.pop_back();
			// set the local in the current frame
			IncRef( rhs );
			CurrentFrame()->SetLocal( 9, rhs );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.At( 9 ), CurrentFrame()->GetLocalRef( 9 ) );
			break;
		case op_new_map:
			{
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			// create the map
			Object m = Object( CreateMap() );
			// populate it with 'arg' pairs off the stack
			for( int i = 0; i < arg; i++ )
			{
				rhs = stack.back();
				stack.pop_back();
				lhs = stack.back();
				stack.pop_back();
				m.m->insert( pair<Object, Object>( lhs, rhs ) );
			}
			stack.push_back( m );
			}
			break;
		case op_new_vec:
			{
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			// create the new vector with 'arg' empty slots
			Object v = Object( CreateVector( (int)arg ) );
			// populate it with 'arg' items off the stack
			for( int i = 0; i < arg; i++ )
			{
				o = stack.back();
				stack.pop_back();
				v.v->operator[]( arg-i-1 ) = o;
			}
			stack.push_back( v );
			}
			break;
		case op_new_class:
			{
			// 1 arg: size
			arg = *((dword*)ip);
			ip += sizeof( dword );
			// create the map and turn it into a class
			Object m;
			m.MakeClass( CreateMap() );
			// TODO: 
			// - call constructor function?
			// populate it with 'arg' pairs off the stack
			for( int i = 0; i < arg; i++ )
			{
				rhs = stack.back();
				stack.pop_back();
				lhs = stack.back();
				stack.pop_back();
				m.m->insert( pair<Object, Object>( lhs, rhs ) );
			}
			stack.push_back( m );
			}
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
				throw RuntimeException( "Equality operator used on operands of different types." );
			switch( lhs.type )
			{
			case obj_null: stack.push_back( Object( rhs.type == obj_null ) ); break;
			case obj_boolean: stack.push_back( Object( lhs.b == rhs.b ) ); break;
			case obj_number: stack.push_back( Object( lhs.b == rhs.b ) ); break;
			case obj_string: stack.push_back( Object( strcmp( lhs.s, rhs.s ) == 0 ) ); break;
			case obj_vector: stack.push_back( Object( lhs.v == rhs.v ) ); break;
			case obj_map:
			case obj_class:
			case obj_instance:
				stack.push_back( Object( lhs.m == rhs.m ) ); break;
			case obj_function: stack.push_back( Object( lhs.f == rhs.f ) ); break;
			case obj_native_function: stack.push_back( Object( lhs.nf.p == rhs.nf.p ) ); break;
			case obj_native_obj: stack.push_back( Object( lhs.no == rhs.no ) ); break;
			case obj_size: stack.push_back( Object( lhs.sz == rhs.sz ) ); break;
			}
			break;
		case op_neq:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw RuntimeException( "Inequality operator used on operands of different types." );
			switch( lhs.type )
			{
			case obj_null: stack.push_back( Object( rhs.type != obj_null ) ); break;
			case obj_boolean: stack.push_back( Object( lhs.b != rhs.b ) ); break;
			case obj_number: stack.push_back( Object( lhs.d != rhs.d ) ); break;
			case obj_string: stack.push_back( Object( strcmp( lhs.s, rhs.s ) != 0 ) ); break;
			case obj_vector: stack.push_back( Object( lhs.v != rhs.v ) ); break;
			case obj_map:
			case obj_class:
			case obj_instance:
				stack.push_back( Object( lhs.m != rhs.m ) ); break;
			case obj_function: stack.push_back( Object( lhs.f != rhs.f ) ); break;
			case obj_native_function: stack.push_back( Object( lhs.nf.p != rhs.nf.p ) ); break;
			case obj_native_obj: stack.push_back( Object( lhs.no != rhs.no ) ); break;
			case obj_size: stack.push_back( Object( lhs.sz != rhs.sz ) ); break;
			}
			break;
		case op_lt:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw RuntimeException( "Less-than operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( Object( lhs.d < rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( Object( strcmp( lhs.s, rhs.s ) < 0 ) );
			else
				throw RuntimeException( "Operands to less-than operator must be numbers or strings." );
			break;
		case op_lte:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw RuntimeException( "Less-than-or-equals operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( Object( lhs.d <= rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( Object( strcmp( lhs.s, rhs.s ) <= 0 ) );
			else
				throw RuntimeException( "Operands to less-than-or-equals operator must be numbers or strings." );
			break;
		case op_gt:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw RuntimeException( "Greater-than operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( Object( lhs.d > rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( Object( strcmp( lhs.s, rhs.s ) > 0 ) );
			else
				throw RuntimeException( "Operands to greater-than operator must be numbers or strings." );
			break;
		case op_gte:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != rhs.type )
				throw RuntimeException( "Greater-than-or-equals operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( Object( lhs.d >= rhs.d ) );
			else if( lhs.type == obj_string )
				stack.push_back( Object( strcmp( lhs.s, rhs.s ) >= 0 ) );
			else
				throw RuntimeException( "Operands to greater-than-or-equals operator must be numbers or strings." );
			break;
		case op_or:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			stack.push_back( Object( lhs.CoerceToBool() || rhs.CoerceToBool() ) );
			break;
		case op_and:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			stack.push_back( Object( lhs.CoerceToBool() && rhs.CoerceToBool() ) );
			break;
		case op_neg:
			o = stack.back();
			stack.pop_back();
			if( o.type != obj_number )
				throw RuntimeException( "Negate operator can only be used on numeric objects." );
			stack.push_back( Object( -o.d ) );
			break;
		case op_not:
			o = stack.back();
			stack.pop_back();
			stack.push_back( Object( !o.CoerceToBool() ) );
			break;
		case op_add:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number && lhs.type != obj_string )
				throw RuntimeException( "Left-hand side of addition operator must be a number or a string." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of addition operator must be a number or a string." );
			if( lhs.type != rhs.type )
				throw RuntimeException( "Addition operator used on operands of different types." );
			if( lhs.type == obj_number )
				stack.push_back( Object( lhs.d + rhs.d ) );
			else if( lhs.type == obj_string )
			{
				size_t len = strlen( lhs.s ) + strlen( rhs.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, lhs.s );
				strcat( ret, rhs.s );
				CurrentFrame()->AddString( ret );
				stack.push_back( Object( ret ) ); 
			}
			break;
		case op_sub:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw RuntimeException( "Left-hand side of subtraction operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of subtraction operator must be a number." );
			stack.push_back( Object( lhs.d - rhs.d ) );
			break;
		case op_mul:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw RuntimeException( "Left-hand side of multiplication operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of multiplication operator must be a number." );
			stack.push_back( Object( lhs.d * rhs.d ) );
			break;
		case op_div:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw RuntimeException( "Left-hand side of division operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of division operator must be a number." );
			if( rhs.d == 0.0 )
				throw RuntimeException( "Division by zero fault." );
			stack.push_back( Object( lhs.d / rhs.d ) );
			break;
		case op_mod:
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( lhs.type != obj_number )
				throw RuntimeException( "Left-hand side of modulus operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of modulus operator must be a number." );
			if( rhs.d == 0.0 )
				throw RuntimeException( "Division by zero fault." );
			// error if arguments aren't integral numbers...
			if( modf( lhs.d, &intpart ) != 0.0 || modf( rhs.d, &intpart ) != 0.0 )
				throw RuntimeException( "Operands in modulus operator must be integral numbers." );
			stack.push_back( Object( (double)((int)lhs.d % (int)rhs.d) ) );
			break;
		case op_add_assign: // add <Op0> and tos and store back into <Op0>
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = scopes->FindSymbol( o.s );
			if( !plhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number && plhs->type != obj_string )
				throw RuntimeException( "Left-hand side of addition assignment operator must be a number or a string." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of addition assignment operator must be a number or a string." );
			if( plhs->type != rhs.type )
				throw RuntimeException( "Addition assignment operator used on operands of different types." );
			if( plhs->type == obj_number )
				*plhs = Object( plhs->d + rhs.d );
			else if( plhs->type == obj_string )
			{
				size_t len = strlen( plhs->s ) + strlen( rhs.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, plhs->s );
				strcat( ret, rhs.s );
				CurrentFrame()->AddString( ret );
				*plhs = Object( ret );
			}
			ip += sizeof( dword );
			break;
		case op_sub_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = scopes->FindSymbol( o.s );
			if( !plhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw RuntimeException( "Left-hand side of subtraction assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of subtraction assignment operator must be a number." );
			*plhs = Object( plhs->d - rhs.d );
			ip += sizeof( dword );
			break;
		case op_mul_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = scopes->FindSymbol( o.s );
			if( !plhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw RuntimeException( "Left-hand side of multiplication assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of multiplication assignment operator must be a number." );
			*plhs = Object( plhs->d * rhs.d );
			ip += sizeof( dword );
			break;
		case op_div_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = scopes->FindSymbol( o.s );
			if( !plhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw RuntimeException( "Left-hand side of division assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of division assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw RuntimeException( "Divide-by-zero error." );
			*plhs = Object( plhs->d / rhs.d );
			ip += sizeof( dword );
			break;
		case op_mod_assign:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the constant
			o = GetConstant( arg );
			// find the variable
			plhs = scopes->FindSymbol( o.s );
			if( !plhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw RuntimeException( "Left-hand side of modulus assignment operator must be a number." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of modulus assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw RuntimeException( "Divide-by-zero error." );
			// error if arguments aren't integral numbers...
			if( modf( lhs.d, &intpart ) != 0.0 || modf( rhs.d, &intpart ) != 0.0 )
				throw RuntimeException( "Operands in modulus operator must be integral numbers." );
			*plhs = Object( (double)((int)plhs->d / (int)rhs.d) );
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
				throw RuntimeException( "Left-hand side of addition assignment operator must be a number or a string." );
			if( rhs.type != obj_number && rhs.type != obj_string )
				throw RuntimeException( "Right-hand side of addition assignment operator must be a number or a string." );
			if( lhs.type != rhs.type )
				throw RuntimeException( "Addition assignment operator used on operands of different types." );
			if( lhs.type == obj_number )
				CurrentFrame()->SetLocal( arg, Object( lhs.d + rhs.d ) );
			else if( lhs.type == obj_string )
			{
				size_t len = strlen( lhs.s ) + strlen( rhs.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, lhs.s );
				strcat( ret, rhs.s );
				CurrentFrame()->AddString( ret );
				CurrentFrame()->SetLocal( arg, Object( ret ) );
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
				throw RuntimeException( "Left-hand side of subtraction assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of subtraction assignment operator must be a number." );
			CurrentFrame()->SetLocal( arg, Object( lhs.d - rhs.d ) );
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
				throw RuntimeException( "Left-hand side of multiplication assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of multiplication assignment operator must be a number." );
			CurrentFrame()->SetLocal( arg, Object( lhs.d * rhs.d ) );
			ip += sizeof( dword );
			break;
		case op_div_assign_local:
			// 1 arg
			arg = *((dword*)ip);
			// look-up the local
			lhs = CurrentFrame()->GetLocal( arg );
			if( lhs )
				throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
			rhs = stack.back();
			stack.pop_back();
			if( plhs->type != obj_number )
				throw RuntimeException( "Left-hand side of division assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of division assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw RuntimeException( "Divide-by-zero error." );
			CurrentFrame()->SetLocal( arg, Object( lhs.d / rhs.d ) );
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
				throw RuntimeException( "Left-hand side of modulus assignment operator must be a number." );
			if( rhs.type != obj_number )
				throw RuntimeException( "Right-hand side of modulus assignment operator must be a number." );
			if( rhs.d == 0.0 )
				throw RuntimeException( "Divide-by-zero error." );
			// error if arguments aren't integral numbers...
			if( modf( lhs.d, &intpart ) != 0.0 || modf( rhs.d, &intpart ) != 0.0 )
				throw RuntimeException( "Operands in modulus operator must be integral numbers." );
			CurrentFrame()->SetLocal( arg, Object( (double)((int)lhs.d % (int)rhs.d) ) );
			ip += sizeof( dword );
			break;
		case op_call: // call function with <Op0> args on on stack, fcn after args
			// 1 arg: number of args passed
			arg = *((dword*)ip);
			ip += sizeof( dword );
			// get the fcn
			o = stack.back();
			stack.pop_back();
			// if it's a fcn, build a frame for it
			if( o.type == obj_function )
			{
				// if this is a method there's an extra arg for 'this'
				if( o.f->is_method )
					arg++;
				// create a frame for the fcn
				Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, (int)arg, o.f );
				Scope* scope = new Scope();
				// set the args for the frame
				for( int i = 0; i < arg; i++ )
				{
					Object ob = stack.back();
					frame->SetLocal( arg-i-1, ob );
					stack.pop_back();
				}
				// default arg vals...
				int num_defaults = o.f->num_args - arg;
				if( num_defaults != 0 )
				{
					int non_defaults = o.f->num_args - o.f->default_args.Size();
					for( int i = 0; i < num_defaults; i++ )
					{
						int idx = o.f->default_args.At( arg+i-non_defaults );
						Object o = GetConstant( arg );
						frame->SetLocal( arg+i, o );
					}
				}
				// push the frame onto the callstack
				PushFrame( frame );
				PushScope( scope );
				// jump to the function
				// TODO: if addr relative to another code block (module)??
				ip = (byte*)(base + o.f->addr);
			}
			// is it a native fcn?
			else if( o.type == obj_native_function )
			{
				// if this is a method there's an extra arg for 'this'
				if( o.nf.is_method )
					arg++;
				// create a frame for the fcn
				Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, (int)arg, o.nf );
				Scope* scope = new Scope();
				// set the args for the frame
				for( int i = 0; i < arg; i++ )
				{
					Object ob = stack.back();
					frame->SetLocal( arg-i-1, ob );
					stack.pop_back();
				}
				// push the frame onto the callstack
				PushFrame( frame );
				PushScope( scope );
				o.nf.p( frame );
				PopScope();
				PopFrame();
			}
			// is it a const we need to look up?
			else if( o.type == obj_symbol_name )
			{
				// TODO: need to look up class names too, as this could be a
				// constructor ('new') call
				// find the function
				Function* f = FindFunction( o.s );
				if( f )
				{
					// if this is a method there's an extra arg for 'this'
					if( f->is_method )
						arg++;
					// create a frame for the fcn
					Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, arg, f );
					Scope* scope = new Scope();
					// set the args for the frame
					for( int i = 0; i < arg; i++ )
					{
						Object ob = stack.back();
						frame->SetLocal( arg-i-1, ob );
						stack.pop_back();
					}
					// default arg vals...
					int num_defaults = f->num_args - arg;
					if( num_defaults != 0 )
					{
						int non_defaults = f->num_args - f->default_args.Size();
						for( int i = 0; i < num_defaults; i++ )
						{
							int idx = f->default_args.At( arg+i-non_defaults );
							Object ob = GetConstant( idx );
							frame->SetLocal( arg+i, ob );
						}
					}
					// push the frame onto the callstack
					PushFrame( frame );
					PushScope( scope );
					// jump to the function
					// TODO: if addr relative to another code block (module)??
					ip = (byte*)(base + f->addr);
				}
				else
				{
					NativeFunction nf = FindBuiltin( o.s );
					if( nf.p )
					{
						// if this is a method there's an extra arg for 'this'
						if( nf.is_method )
							arg++;
						// create a frame for the fcn
						Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, arg, nf );
						Scope* scope = new Scope();
						// set the args for the frame
						for( int i = 0; i < arg; i++ )
						{
							Object ob = stack.back();
							frame->SetLocal( arg-i-1, ob );
							stack.pop_back();
						}
						// push the frame onto the callstack
						PushFrame( frame );
						PushScope( scope );
						nf.p( frame );
						PopScope();
						PopFrame();
					}
					else
						throw RuntimeException( boost::format( "Invalid function name '%1%'." ) % o.s );
				}
			}
			else
				throw RuntimeException( boost::format( "Object '%1%' is not a function." ) % o );
			break;
		case op_return:
			// 1 arg: number of scopes to leave
			arg = *((dword*)ip);
			// leave the scopes
			for( int i = 0; i < arg; i++ )
				PopScope();
			// jump to the new frame's address
			ip = (byte*)CurrentFrame()->GetReturnAddress();
			// pop the argument scope...
			PopScope();
			// ... and the current frame
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
			{
			bool is_map = false;
			// 1 arg: size/address to jump to if done looping
			arg = *((dword*)ip);
			ip += sizeof( dword );
			// the iterable object is on the top of the stack, get it but don't
			// pop it - we'll need it for the loop
			lhs = stack.back();
			if( !IsRefType( lhs.type ) )
				throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
			// TODO: ensure lhs is an iterable type if it's a class/instance
			// call 'next':
			if( IsVecType( lhs.type ) )
			{
				// get vector 'next' method
				NativeFunction nf = GetVectorBuiltin( string( "next" ) );
				if( !nf.p )
					throw ICE( "Vector builtin 'next' not found." );
				if( !nf.is_method )
					throw ICE( "Vector builtin not marked as a method." );
				// call 'next'
				// this is a method, so there's an extra arg for 'this'
				int args = 1;
				// create a frame for the fcn
				Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, 1, nf );
				Scope* scope = new Scope();
				// set the arg for the frame
				// store op, needs IncRef()
//				IncRef( lhs );
				frame->SetLocal( 0, lhs );
				// define the local in the current scope
				// (don't know what the arg names are for native fcns)
//				scope->AddSymbol( string( "arg0" ), frame->GetLocalRef( 0 ) );
				// push the frame onto the callstack
				PushFrame( frame );
				PushScope( scope );
				// call 'next' vector builtin
				nf.p( frame );
				PopScope();
				PopFrame();
			}
			else if( IsMapType( lhs.type ) )
			{
				is_map = true;
				if( lhs.type == obj_class || lhs.type == obj_instance )
				{
					// TODO: handle classes and instances
//					Object next( "next" );
					// do a tbl_load to get the fcn object for 'next' on the stack
				}
				// TODO: handle maps
				else
				{
					// get map 'next' method
					NativeFunction nf = GetMapBuiltin( string( "next" ) );
					if( !nf.p )
						throw ICE( "Map builtin 'next' not found." );
					if( !nf.is_method )
						throw ICE( "Map builtin not marked as a method." );
					// call 'next'
					// this is a method, so there's an extra arg for 'this'
					int args = 1;
					// create a frame for the fcn
					Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, 1, nf );
					Scope* scope = new Scope();
					// set the arg for the frame
					// store op, needs IncRef()
//					IncRef( lhs );
					frame->SetLocal( 0, lhs );
					// define the local in the current scope
					// (don't know what the arg names are for native fcns)
//					scope->AddSymbol( string( "arg0" ), frame->GetLocalRef( 0 ) );
					// push the frame onto the callstack
					PushFrame( frame );
					PushScope( scope );
					// call 'next' map builtin
					nf.p( frame );
					PopScope();
					PopFrame();
				}
			}
			// 'next' has put a two-item vector on the stack, with a bool
			// indicating if there are more items and the item(s) if there are or null
			// if there aren't
			// get the vector off the stack
			o = stack.back();
			stack.pop_back();
			if( o.type != obj_vector )
				throw ICE( "Non-vector returned from 'next' in op_for_iter." );
			// check the first item
			Object cont = o.v->operator[]( 0 );
			if( cont.type != obj_boolean )
				throw ICE( "Non-boolean returned as first item from 'next' in op_for_iter." );
			// if 'false', we're done, jump to end (stored in 'arg')
			if( !cont.b )
				ip = (byte*)(base + arg);
			// otherwise push the item(s) onto the stack
			else
			{
				// a map will have returned the second item as a vector
				// (key/value pair)
				if( is_map )
				{
					Object ov = o.v->operator[]( 1 );
					if( ov.type != obj_vector )
						throw RuntimeException( "map 'next' builtin method did not return a vector with a vector key/value pair as its second item." );
					stack.push_back( ov.v->operator[]( 0 ) );
					stack.push_back( ov.v->operator[]( 1 ) );
				}
				// a vector will just be the value we want
				else
					stack.push_back( o.v->operator[]( 1 ) );
			}
			}
			break;
		case op_tbl_load:// tos = tos1[tos]
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( !IsRefType( lhs.type ) )
				throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
			// vector:
			if( IsVecType( lhs.type ) )
			{
				if( rhs.type == obj_string || rhs.type == obj_symbol_name )
				{
					// check for vector built-in method
					NativeFunction nf = GetVectorBuiltin( string( rhs.s ) );
					if( nf.p )
					{
						if( !nf.is_method )
							throw ICE( "Vector builtin not marked as a method." );
						stack.push_back( lhs );
						stack.push_back( Object( nf ) );
					}
					else
						throw RuntimeException( "Invalid vector index or method." );
					break;
				}
				if( rhs.type != obj_number )
					throw RuntimeException( "Index to a vector must be a numeric values." );
				// error if arguments aren't integral numbers...
				double intpart;
				if( modf( rhs.d, &intpart ) != 0.0 )
					throw RuntimeException( "Index to a vector must be an integral value." );
				int idx = (int)rhs.d;
				// out-of-bounds check
				if( lhs.v->size() <= idx || idx < 0 )
					throw RuntimeException( "Out-of-bounds error indexing vector." );
				Object obj = lhs.v->operator[]( idx );
				stack.push_back( obj );
			}
			// map/class/instance:
			else
			{
				// find the rhs (key in the lhs (map)
				Map::iterator i = lhs.m->find( rhs );
				if( i == lhs.m->end() )
				{
					if( rhs.type == obj_string || rhs.type == obj_symbol_name )
					{
						// check for map built-in method
						NativeFunction nf = GetMapBuiltin( string( rhs.s ) );
						if( nf.p )
						{
							if( !nf.is_method )
								throw ICE( "Map builtin not marked as a method." );
							stack.push_back( lhs );
							stack.push_back( Object( nf ) );
						}
						else
							throw RuntimeException( "Invalid vector index or method." );
						break;
					}
					// TODO: check for class/instance method
					else 
						throw RuntimeException( "Invalid key value. Item not found." );
				}
				stack.push_back( i->second );
			}
			break;
		case op_loadslice2:
			// TODO:
		case op_loadslice3:
			// TODO:
		case op_tbl_store:// tos2[tos1] = tos
			o = stack.back();
			stack.pop_back();
			rhs = stack.back();
			stack.pop_back();
			lhs = stack.back();
			stack.pop_back();
			if( !IsRefType( lhs.type ) )
				throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
			// vector:
			if( IsVecType( lhs.type ) )
			{
				if( rhs.type != obj_number )
					throw RuntimeException( "Vectors can only be indexed with numeric values." );
				// error if arguments aren't integral numbers...
				double intpart;
				if( modf( rhs.d, &intpart ) != 0.0 )
					throw RuntimeException( "Index to a vector must be an integral value." );
				int idx = (int)rhs.d;
				// out-of-bounds check
				if( lhs.v->size() <= idx || idx < 0 )
					throw RuntimeException( "Out-of-bounds error indexing vector." );
				lhs.v->operator[]( idx ) = o;
				// IncRef stored item
				IncRef( o );
			}
			// map/class/instance:
			else
			{
				// TODO: deva1 seemed to allow only numbers, strings and UDTs as
				// keys... ???
				lhs.m->operator[]( rhs ) = o;
				// IncRef stored item
				IncRef( o );
			}
			break;
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
			// 1 arg:
			arg = *((dword*)ip);
			ip += sizeof( dword );
			for( int i = 0; i < arg; i++ )
				stack.push_back( stack.back() );
			break;
		case op_dup1:
			stack.push_back( stack.back() );
			break;
		case op_dup2:
			stack.push_back( stack.back() );
			stack.push_back( stack.back() );
			break;
		case op_dup3:
			stack.push_back( stack.back() );
			stack.push_back( stack.back() );
			stack.push_back( stack.back() );
			break;
		case op_dup_top_n:
//			stack.push_back( stack[stack.size()-2] );
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
			throw ICE( "Illegal instruction" );
			break;
		}
	}
	// TODO: delete scope table here?
	// free the scope table
	delete scopes;
	// TODO: review. is this where the 'global' frame should be deleted?
	PopFrame();
}

// disassemble the instruction stream to stdout
void Executor::Decode( const Code & code)
{
	byte* b = code.code;
	byte* p = b;
	byte* end = code.code + code.len;
	Opcode op = op_nop;

	cout << "Instructions:" << endl;
	while( p < end && op < op_halt )
	{
		// decode opcode
		op = (Opcode)*p;
		p += PrintOpcode( op, b, p );
		cout << endl;
	}
}

int Executor::PrintOpcode( Opcode op, const byte* b, byte* p )
{
	dword arg, arg2;
	Object o;
	int ret = 0;

	cout.width( 4 );
	cout << (int)(p - b) << ": " <<  opcodeNames[op] << "\t";
	p++;
	switch( op )
	{
	case op_nop:
	case op_pop:
		cout << "\t" << " ";
		break;
	case op_push:
		// 1 arg
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
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
		cout << "\t" << " ";
		break;
	case op_pushlocal:
		// 1 arg
		ret = sizeof( dword );
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
		cout << "\t" << " ";
		break;
	case op_pushconst:
		// 1 arg: index to constant
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_storeconst:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_store_true:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_store_false:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_store_null:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_storelocal:
		// 1 arg
		ret = sizeof( dword );
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
		cout << "\t" << " ";
		break;
	case op_def_local:
		// 1 arg
		ret = sizeof( dword );
		break;
	case op_def_local0:
	case op_def_local1:
	case op_def_local2:
	case op_def_local3:
	case op_def_local4:
	case op_def_local5:
	case op_def_local6:
	case op_def_local7:
	case op_def_local8:
	case op_def_local9:
		cout << "\t" << " ";
		break;
	case op_new_map:
		// 1 arg: size
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_new_vec:
		// 1 arg: size
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_new_class:
		// 1 arg: size
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_new_instance:
		// 1 arg: size
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_jmp:
		// 1 arg: size
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_jmpf:
		// 1 arg: size
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
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
		cout << "\t" << " ";
		break;
	case op_add_assign:
	case op_sub_assign:
	case op_mul_assign:
	case op_div_assign:
	case op_mod_assign:
		// 1 arg: lhs
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_add_assign_local:
	case op_sub_assign_local:
	case op_mul_assign_local:
	case op_div_assign_local:
	case op_mod_assign_local:
		// 1 arg: lhs
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_call:
		// 1 arg: number of args passed
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_return:
		// 1 arg: number of scopes to leave
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_exit_loop:
		// 2 args: jump target address, number of scopes to leave
		arg = *((dword*)p);
		ret = sizeof( dword );
		arg2 = *((dword*)p);
		ret = sizeof( dword );
		cout << "\t" << arg << "\t" << arg2;
		break;
	case op_enter:
	case op_leave:
		cout << "\t" << " ";
		break;
	case op_for_iter:
		// 1 arg: iterable object
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
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
		break;
	case op_dup:
		// 1 arg:
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_dup1:
	case op_dup2:
	case op_dup3:
	case op_dup_top_n:
	case op_dup_top1:
	case op_dup_top2:
	case op_dup_top3:
	case op_swap:
		cout << "\t" << " ";
		break;
	case op_rot:
		// 1 arg:
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_rot2:
	case op_rot3:
	case op_rot4:
	case op_import:
		// 1 arg:
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant(arg);
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_halt:
		cout << "\t" << " ";
		break;
	case op_illegal:
	default:
		cout << "Error: Invalid instruction.";
		break;
	}
//	cout << endl;
	return ret+1;
}

void Executor::DumpFunctions()
{
	cout << "Function objects:" << endl;
	for( map<string,Function*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		Function* f = i->second;
		cout << "function: " << f->name << ", from file: " << f->filename << ", line: " << f->first_line;
		cout << endl;
		cout << f->num_args << " arg(s), default value indices: ";
		for( int j = 0; j < f->default_args.Size(); j++ )
			cout << f->default_args.At( j ) << " ";
		cout << endl << f->num_locals << " local(s): ";
		for( int j = 0; j < f->local_names.Size(); j++ )
			cout << f->local_names.At( j ) << " ";
		cout << endl << "code address: " << f->addr << endl;
	}
}
void Executor::DumpConstantPool()
{
	cout << "Constant data pool:" << endl;
	for( int i = 0.; i < NumConstants(); i++ )
	{
		Object o = GetConstant( i );
		if( o.type == obj_string || o.type == obj_symbol_name )
			cout << o.s << endl;
		else if( o.type == obj_number )
			cout << o.d << endl;
		else if( o.type == obj_boolean )
			cout << (o.b ? "<boolean-true>" : "<boolean-false>") << endl;
		else if( o.type == obj_null )
			cout << "<null-value>" << endl;
	}
}

} // namespace deva
