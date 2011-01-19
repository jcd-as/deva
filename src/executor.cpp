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
	for( map<string, Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		delete i->second->f;
		delete i->second;
	}
}

// recursively call constructors on an object and its base classes
// given a class object and the instance we're creating
// (only the first constructor call (most derived class) can pass arguments)
void Executor::CallConstructors( Object o, Object instance, int num_args /*= 0*/ )
{
	// get the base classes collection
	Map::iterator i = o.m->find( Object( obj_symbol_name, "__bases__" ) );
	if( i == o.m->end() )
		throw ICE( "Unable to find '__bases__' member in class object." );
	if( i->second.type != obj_vector )
		throw ICE( "Type of '__bases__' member is not a vector." );
	Vector* bases = i->second.v;
	// call each base class' constructor (in reverse order)
	for( Vector::reverse_iterator iv = bases->rbegin(); iv != bases->rend(); ++iv )
	{
		if( iv->type != obj_class )
			throw ICE( "Base class object in '__bases__' member is not a class." );

		// recur
		CallConstructors( *iv, instance, 0 );
	}
	// call the constructor for this (most derived) class
	// push the new instance onto the stack
	stack.push_back( instance );
	// get the 'new' method (constructor) of this class and call it
	Map::iterator it = o.m->find( Object( obj_symbol_name, "new" ) );
	if( it != o.m->end() )
	{
		if( it->second.type != obj_function )
			throw RuntimeException( "'new' method of instance object is not a function." );
		ExecuteFunction( it->second.f, num_args );
		// pop the (null) return value
		stack.pop_back();
	}
}

// recursively call destructors on an object and its base classes
void Executor::CallDestructors( Object o )
{
	// call the destructor for this (most derived) class
	// push the instance onto the stack
	stack.push_back( o );
	// get the 'delete' method (destructor) of this class and call it
	Map::iterator it = o.m->find( Object( obj_symbol_name, "delete" ) );
	if( it != o.m->end() )
	{
		if( it->second.type != obj_function )
			throw RuntimeException( "'delete' method of instance object is not a function." );
		ExecuteFunction( it->second.f, 0, true );
		// pop the (null) return value
		stack.pop_back();
	}

	// get the base classes collection
	Map::iterator i = o.m->find( Object( obj_symbol_name, "__bases__" ) );
	if( i == o.m->end() )
		throw ICE( "Unable to find '__bases__' member in class object." );
	if( i->second.type != obj_vector )
		throw ICE( "Type of '__bases__' member is not a vector." );
	Vector* bases = i->second.v;
	// call each base class' destructor
	for( Vector::iterator iv = bases->begin(); iv != bases->end(); ++iv )
	{
		if( iv->type != obj_class )
			throw ICE( "Base class object in '__bases__' member is not a class." );

		// recur
		CallDestructors( *iv );
	}
}

void Executor::ExecuteCode( const Code & code )
{
	// NOTE: all actions that create a new C string (char*) as a local need to
	// add it to that frame's strings collection (i.e. call
	// CurrentFrame()->AddString())

	Opcode op = op_nop;

	end = code.code + code.len;
	bp = code.code;
	ip = bp;

	// TODO: create the scopes here?
	scopes = new ScopeTable();

	// TODO: review. is this where this frame should be added?
	// (it needs the code location/address, for return values, so where else can
	// it go??)

	// a starting ('global') frame
	Object *main = FindFunction( string( "@main" ) );
	Frame* frame = new Frame( NULL, scopes, (dword)bp, 0, main->f );
	PushFrame( frame );

	// make sure the global scope is always around
	scopes->PushScope( new Scope() );

	if( trace )
		cout << "Execution trace:" << endl;

	// execute until end
	while( ip < end && op < op_halt )
	{
		op = ExecuteInstruction();
	}

	// TODO: delete scope table here?
	// free the scope table
	delete scopes;
	// TODO: review. is this where the 'global' frame should be deleted?
	PopFrame();
}

void Executor::ExecuteToReturn( bool is_destructor /*= false*/ )
{
	Opcode op = op_nop;

	// execute until return
	while( op != op_return )
	{
		op = ExecuteInstruction();
		// if end, error, unless this is a destructor
		if( !is_destructor && (ip >= end || op == op_halt ) )
			throw ICE( "End of code encounted before function returned." );
	}
}

Opcode Executor::ExecuteInstruction()
{
	dword arg, arg2;
	Object o, lhs, rhs, *plhs;
	double intpart; // for modf() calls

	// decode opcode
	Opcode op = (Opcode)*ip;

	if( trace )
	{
		PrintOpcode( op, bp, ip );
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
		// (pop is the only op that can follow a return op and *doesn't* IncRef
		// the returned value - in all other cases the IncRef call will reset
		// the flag)
		last_op_was_return = false;
		stack.pop_back();
		break;
	case op_push:
		// push an integer value directly
		// 1 arg
		arg = *((dword*)ip);
		stack.push_back( Object( (double)(int)arg ) );
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
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( arg ), CurrentFrame()->GetLocalRef( arg ) );
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
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 0 ), CurrentFrame()->GetLocalRef( 0 ) );
		break;
	case op_def_local1:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 1, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 1 ), CurrentFrame()->GetLocalRef( 1 ) );
		break;
	case op_def_local2:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 2, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 2 ), CurrentFrame()->GetLocalRef( 2 ) );
		break;
	case op_def_local3:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 3, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 3 ), CurrentFrame()->GetLocalRef( 3 ) );
		break;
	case op_def_local4:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 4, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 4 ), CurrentFrame()->GetLocalRef( 4 ) );
		break;
	case op_def_local5:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 5, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 5 ), CurrentFrame()->GetLocalRef( 5 ) );
		break;
	case op_def_local6:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 6, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 6 ), CurrentFrame()->GetLocalRef( 6 ) );
		break;
	case op_def_local7:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 7, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 7 ), CurrentFrame()->GetLocalRef( 7 ) );
		break;
	case op_def_local8:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 8, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 8 ), CurrentFrame()->GetLocalRef( 8 ) );
		break;
	case op_def_local9:
		rhs = stack.back();
		stack.pop_back();
		// set the local in the current frame
		IncRef( rhs );
		CurrentFrame()->SetLocal( 9, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 9 ), CurrentFrame()->GetLocalRef( 9 ) );
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
		// 1 arg: size = number of base classes
		arg = *((dword*)ip);
		ip += sizeof( dword );
		// create the map and turn it into a class
		Object m;
		m.MakeClass( CreateMap() );
		// set its name
		Object name = stack.back();
		stack.pop_back();
		Object _name = GetConstant( FindConstant( Object( obj_symbol_name, "__name__" ) ) );
		m.m->insert( pair<Object, Object>( _name, name ) );
		// build the bases list
		Vector* v = CreateVector();
		for( int i = 0; i < arg; i++ )
		{
			Object base = stack.back();
			stack.pop_back();
			if( base.type != obj_class )
				throw ICE( "Base class expected. Bad code gen? Corrupt stack?" );
			v->push_back( base );
		}
		Object _bases = GetConstant( FindConstant( Object( obj_symbol_name, "__bases__" ) ) );
		m.m->insert( pair<Object, Object>( _bases, Object( v ) ) );
		// add all of the methods for this class
		for( map<string,Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
		{
			Function* f = i->second->f;
			if( f->classname == name.s )
			{
				int idx = FindConstant( Object( obj_symbol_name, f->name.c_str() ) );
				if( idx == -1 )
					throw ICE( "Function name not found in constants pool." );
				Object fcnname = GetConstant( idx );
				m.m->insert( pair<Object, Object>( fcnname, Object( f ) ) );
			}
		}
		// push the new class object onto the stack (it will be consumed by a
		// following 'def_local' instruction)
		stack.push_back( m );
		}
		break;
	case op_jmp:
		// 1 arg: size
		arg = *((dword*)ip);
		ip = (byte*)(bp + arg);
		break;
	case op_jmpf:
		// 1 arg: size
		arg = *((dword*)ip);
		o = stack.back();
		stack.pop_back();
		if( !o.CoerceToBool() )
			ip = (byte*)(bp + arg);
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
		// TODO: if addr relative to another code block (module)??
		//
		// if it's a fcn, build a frame for it
		if( o.type == obj_function )
		{
			ExecuteFunction( o.f, arg );
		}
		// is it a native fcn?
		else if( o.type == obj_native_function )
		{
			ExecuteFunction( o.nf, arg );
		}
		// is it a class object (i.e. a constructor call)
		else if( o.type == obj_class )
		{
			// - create a copy of the class object
			Map* inst = CreateMap( *o.m );

			// - add the __class__ member to it
			Object _class = GetConstant( FindConstant( Object( obj_symbol_name, "__class__" ) ) );
			Map::iterator i = o.m->find( Object( obj_symbol_name, "__name__" ) );
			if( i == o.m->end() )
				throw ICE( "Unable to find '__name__' member in class object." );
			inst->insert( pair<Object, Object>( _class, i->second ) );

			// create our instance
			Object instance;
			instance.MakeInstance( inst );

			// recursively call the constructors on this object and its base classes
			CallConstructors( o, instance, arg );
			// ensure we IncRef the upcoming store op...
			last_op_was_return = false;

			stack.push_back( instance );
		}
		// is it a const we need to look up?
		else if( o.type == obj_symbol_name )
		{
			// find the function
			Object* f = FindFunction( o.s );
			if( f && f->f )
			{
				ExecuteFunction( f->f, arg );
			}
			else
			{
				NativeFunction nf = GetBuiltin( o.s );
				if( nf.p )
				{
					ExecuteFunction( nf, arg );
				}
				// TODO: can this ever happen?
				// class name?
//				else
//				{
//					Object* _class = scopes->FindSymbol( o.s );
//					if( _class && _class->type == obj_class )
//					{
//						// get the 'new' method of this class and call it
//					}
//					else
//						throw RuntimeException( boost::format( "Invalid function name '%1%'." ) % o.s );
//				}
			}
		}
		else
			throw RuntimeException( boost::format( "Object '%1%' is not a function." ) % o );
		break;
	case op_return:
		// TODO: better way??
		// if a string is being returned, it needs to be copied to the calling
		// frame! (string mem in a frame is freed on frame exit)
		if( stack.back().type == obj_string )
		{
			Object o = stack.back();
			stack.pop_back();
			const char* str = CurrentFrame()->GetParent()->AddString( string( o.s ) );
			stack.push_back( Object( str ) );
		}
		// inc ref the object being returned so that it leaving the fcn
		// scope won't delete it...
		IncRef( stack.back() );
		// set the var indicating the last op was a return so that the next op
		// won't inc ref again
		last_op_was_return = true;
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
		// TODO: how???
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
		ip = (byte*)(bp + arg);
		break;
	case op_enter:
		PushScope( new Scope() );
		break;
	case op_leave:
		PopScope();
		break;
	case op_for_iter:
	case op_for_iter_pair:
		{
		// 1 arg: size/address to jump to if done looping
		arg = *((dword*)ip);
		ip += sizeof( dword );
		// the iterable object is on the top of the stack, get it but don't
		// pop it - we'll need it for the loop
		lhs = stack.back();
		if( !IsRefType( lhs.type ) )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// call 'next':
		if( IsVecType( lhs.type ) )
		{
			// get vector 'next' method
			NativeFunction nf = GetVectorBuiltin( string( "next" ) );
			if( !nf.p )
				throw ICE( "Vector builtin 'next' not found." );
			if( !nf.is_method )
				throw ICE( "Vector builtin not marked as a method." );
			// dup the TOS (vector)
			stack.push_back( stack.back() );
			// call 'next'
			ExecuteFunction( nf, 0 );
		}
		else if( IsMapType( lhs.type ) )
		{
			// handle classes and instances
			if( lhs.type == obj_class || lhs.type == obj_instance )
			{
				// get the iteration fcns & ensure lhs is an iterable type
				Map::iterator it = lhs.m->find( Object( obj_symbol_name, "next" ) );
				if( it == lhs.m->end() || it->second.type != obj_function )
					throw RuntimeException( "Class used in 'for' loop does not support iteration: missing 'next' method." );
				// dup the TOS (class/instance)
				stack.push_back( stack.back() );
				ExecuteFunction( it->second.f, 0 );
			}
			// handle maps
			else
			{
				// get map 'next' method
				NativeFunction nf = GetMapBuiltin( string( "next" ) );
				if( !nf.p )
					throw ICE( "Map builtin 'next' not found." );
				if( !nf.is_method )
					throw ICE( "Map builtin not marked as a method." );
				// dup the TOS (map)
				stack.push_back( stack.back() );
				// call 'next'
				ExecuteFunction( nf, 0 );
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
			ip = (byte*)(bp + arg);
		// otherwise push the item(s) onto the stack
		else
		{
			// a map/two var loop will have returned the second item as a vector
			// (key/value pair)
			if( op == op_for_iter_pair )
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
				// if this was a symbol name, try looking for it as a string,
				// since 'a.b;' is syntactic sugar for 'a["b"];'
				if( rhs.type == obj_symbol_name )
				{
					Map::iterator it = lhs.m->find( Object( rhs.s ) );
					if( it != lhs.m->end() )
					{
						Object obj = it->second;
						if( obj.type == obj_function || obj.type == obj_native_function )
						{
							// push the class/instance ('this') for instances
							if( lhs.type == obj_instance )
								stack.push_back( lhs );
						}
						// push the object
						stack.push_back( obj );
						break;
					}
				}
				if( rhs.type == obj_string || rhs.type == obj_symbol_name )
				{
					// TODO: should builtins be looked for _last_, so they can
					// be overridden by user methods??
					//
					// check for map built-in method
					NativeFunction nf = GetMapBuiltin( string( rhs.s ) );
					if( nf.p )
					{
						if( !nf.is_method )
							throw ICE( "Map builtin not marked as a method." );
						stack.push_back( lhs );
						stack.push_back( Object( nf ) );
					}
					// check for class/instance method
					else if( lhs.type == obj_class || lhs.type == obj_instance )
					{
						// find the object by name in the map
						Map::iterator i = lhs.m->find( Object( rhs.s ) );
						if( i != lhs.m->end() )
						{
							Object obj = i->second;
							if( obj.type != obj_function )
								throw RuntimeException( boost::format( "Invalid method: '%1%'." ) % rhs.s );
							// push the class/instance ('this')
							if( lhs.type == obj_instance || lhs.type == obj_class )
								stack.push_back( lhs );
							// push the function
							stack.push_back( obj );
							break;
						}
						else
							throw RuntimeException( boost::format( "Invalid map key or method: '%1%'." ) % rhs.s );
					}
					else
						throw RuntimeException( boost::format( "Invalid map key or method: '%1%'." ) % rhs.s );
					break;
				}
				else 
					throw RuntimeException( "Invalid key value. Item not found." );
			}
			else
			{
				// class/instance method object?
				if( i->second.type == obj_function )
				{
					// if this is an instance or class, push 'self'
					if( lhs.type == obj_instance || lhs.type == obj_class )
						stack.push_back( lhs );
				}
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
	case op_add_tbl_store:	// tos2[tos1] += tos
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

			Object lhsob = lhs.v->operator[]( idx );
			if( lhsob.type != obj_number && lhsob.type != obj_string )
				throw RuntimeException( "left-hand side of '+=' operator must be a number or a string." );
			if( o.type != obj_number && o.type != obj_string )
				throw RuntimeException( "right-hand side of '+=' operator must be a number or a string." );
			if( o.type != lhsob.type )
				throw RuntimeException( "left-hand and right-hand sides of '+=' operator must be the same type." );
			if( o.type == obj_number )
			{
				double d = lhsob.d + o.d;
				lhs.v->operator[]( idx ) = Object( d );
			}
			else
			{
				size_t len = strlen( lhsob.s ) + strlen( o.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, lhsob.s );
				strcat( ret, o.s );
				CurrentFrame()->AddString( ret );
				lhs.v->operator[]( idx ) = Object( ret );
			}
		}
		// map/class/instance:
		else
		{
			// TODO: deva1 seemed to allow only numbers, strings and UDTs as
			// keys... ???
			Map::iterator it = lhs.m->find( rhs );
			if( it == lhs.m->end() )
			{
				if( rhs.type == obj_symbol_name )
				{
					// rhs is a symbol, convert it to a string
					rhs = Object( rhs.s );
					it = lhs.m->find( Object( rhs.s ) );
				}
				if( it == lhs.m->end() )
					throw RuntimeException( boost::format( "Invalid index into map: '%1%'." ) % rhs );
			}
			Object lhsob = it->second;
			if( lhsob.type != obj_number && lhsob.type != obj_string )
				throw RuntimeException( "left-hand side of '+=' operator must be a number or a string." );
			if( o.type != obj_number && o.type != obj_string )
				throw RuntimeException( "right-hand side of '+=' operator must be a number or a string." );
			if( o.type != lhsob.type )
				throw RuntimeException( "left-hand and right-hand sides of '+=' operator must be the same type." );
			if( o.type == obj_number )
			{
				double d = lhsob.d + o.d;
				lhs.m->operator[]( rhs ) = Object( d );
			}
			else
			{
				size_t len = strlen( lhsob.s ) + strlen( o.s ) + 1;
				char* ret = new char[len];
				memset( ret, 0, len );
				strcpy( ret, lhsob.s );
				strcat( ret, o.s );
				CurrentFrame()->AddString( ret );
				lhs.m->operator[]( rhs ) = Object( ret );
			}
		}
		break;
	case op_sub_tbl_store:	// tos2[tos1] -= tos
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

			Object lhsob = lhs.v->operator[]( idx );
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '-=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '-=' operator must be a number." );

			double d = lhsob.d - o.d;
			lhs.v->operator[]( idx ) = Object( d );
		}
		// map/class/instance:
		else
		{
			// TODO: deva1 seemed to allow only numbers, strings and UDTs as
			// keys... ???
			Map::iterator it = lhs.m->find( rhs );
			if( it == lhs.m->end() )
			{
				if( rhs.type == obj_symbol_name )
				{
					// rhs is a symbol, convert it to a string
					rhs = Object( rhs.s );
					it = lhs.m->find( Object( rhs.s ) );
				}
				if( it == lhs.m->end() )
					throw RuntimeException( boost::format( "Invalid index into map: '%1%'." ) % rhs );
			}
			Object lhsob = it->second;
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '-=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '-=' operator must be a number." );

			double d = lhsob.d - o.d;
			lhs.m->operator[]( rhs ) = Object( d );
		}
		break;
	case op_mul_tbl_store:	// tos2[tos1] *= tos
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

			Object lhsob = lhs.v->operator[]( idx );
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '*=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '*=' operator must be a number." );

			double d = lhsob.d * o.d;
			lhs.v->operator[]( idx ) = Object( d );
		}
		// map/class/instance:
		else
		{
			// TODO: deva1 seemed to allow only numbers, strings and UDTs as
			// keys... ???
			Map::iterator it = lhs.m->find( rhs );
			if( it == lhs.m->end() )
			{
				if( rhs.type == obj_symbol_name )
				{
					// rhs is a symbol, convert it to a string
					rhs = Object( rhs.s );
					it = lhs.m->find( Object( rhs.s ) );
				}
				if( it == lhs.m->end() )
					throw RuntimeException( boost::format( "Invalid index into map: '%1%'." ) % rhs );
			}
			Object lhsob = it->second;
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '*=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '*=' operator must be a number." );

			double d = lhsob.d * o.d;
			lhs.m->operator[]( rhs ) = Object( d );
		}
		break;
	case op_div_tbl_store:	// tos2[tos1] /= tos
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

			Object lhsob = lhs.v->operator[]( idx );
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '/=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '/=' operator must be a number." );

			double d = lhsob.d / o.d;
			lhs.v->operator[]( idx ) = Object( d );
		}
		// map/class/instance:
		else
		{
			// TODO: deva1 seemed to allow only numbers, strings and UDTs as
			// keys... ???
			Map::iterator it = lhs.m->find( rhs );
			if( it == lhs.m->end() )
			{
				if( rhs.type == obj_symbol_name )
				{
					// rhs is a symbol, convert it to a string
					rhs = Object( rhs.s );
					it = lhs.m->find( Object( rhs.s ) );
				}
				if( it == lhs.m->end() )
					throw RuntimeException( boost::format( "Invalid index into map: '%1%'." ) % rhs );
			}
			Object lhsob = it->second;
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '/=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '/=' operator must be a number." );

			double d = lhsob.d / o.d;
			lhs.m->operator[]( rhs ) = Object( d );
		}
		break;
	case op_mod_tbl_store:	// tos2[tos1] %= tos
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

			Object lhsob = lhs.v->operator[]( idx );
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '%=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '%=' operator must be a number." );

			// ensure integral arguments
			if( modf( o.d, &intpart ) != 0.0 || modf( lhsob.d, &intpart ) != 0.0 )
				throw RuntimeException( "arguments to '%=' must be integral values." );

			double d = (int)lhsob.d % (int)o.d;
			lhs.v->operator[]( idx ) = Object( d );
		}
		// map/class/instance:
		else
		{
			// TODO: deva1 seemed to allow only numbers, strings and UDTs as
			// keys... ???
			Map::iterator it = lhs.m->find( rhs );
			if( it == lhs.m->end() )
			{
				if( rhs.type == obj_symbol_name )
				{
					// rhs is a symbol, convert it to a string
					rhs = Object( rhs.s );
					it = lhs.m->find( Object( rhs.s ) );
				}
				if( it == lhs.m->end() )
					throw RuntimeException( boost::format( "Invalid index into map: '%1%'." ) % rhs );
			}
			Object lhsob = it->second;
			if( lhsob.type != obj_number )
				throw RuntimeException( "left-hand side of '%=' operator must be a number." );
			if( o.type != obj_number )
				throw RuntimeException( "right-hand side of '%=' operator must be a number." );

			// ensure integral arguments
			double intpart;
			if( modf( o.d, &intpart ) != 0.0 || modf( lhsob.d, &intpart ) != 0.0 )
				throw RuntimeException( "arguments to '%=' must be integral values." );

			double d = (int)lhsob.d % (int)o.d;
			lhs.m->operator[]( rhs ) = Object( d );
		}
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
	return op;
}

void Executor::ExecuteFunction( Function* f, int num_args, bool is_destructor /*= false*/ )
{
	// if this is a method there's an extra arg for 'this'
	if( f->IsMethod() )
		num_args++;

	if( num_args > f->num_args )
		throw RuntimeException( boost::format( "Too many arguments passed to function '%1%'." ) % f->name );
	if( (f->num_args - num_args) > f->NumDefaultArgs() )
		throw RuntimeException( boost::format( "Not enough arguments passed to function '%1%'." ) % f->name );

	// create a frame for the fcn
	Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, num_args, f );
	Scope* scope = new Scope();
	// set the args for the frame
	for( int i = 0; i < num_args; i++ )
	{
		Object ob = stack.back();
		frame->SetLocal( num_args-i-1, ob );
		stack.pop_back();
	}
	// default arg vals...
	int num_defaults = f->num_args - num_args;
	if( num_defaults != 0 )
	{
		int non_defaults = f->num_args - f->default_args.Size();
		for( int i = 0; i < num_defaults; i++ )
		{
			int idx = f->default_args.At( num_args+i-non_defaults );
			Object o = GetConstant( num_args );
			frame->SetLocal( num_args+i, o );
		}
	}
	// push the frame onto the callstack
	PushFrame( frame );
	PushScope( scope );
	// jump to the function
	ip = (byte*)(bp + f->addr);
	// execute until it returns
	ExecuteToReturn( is_destructor );
}

void Executor::ExecuteFunction( NativeFunction nf, int num_args )
{
	// if this is a method there's an extra arg for 'this'
	if( nf.is_method )
		num_args++;
	// create a frame for the fcn
	Frame* frame = new Frame( CurrentFrame(), scopes, (dword)ip, num_args, nf );
	Scope* scope = new Scope();
	// set the args for the frame
	for( int i = 0; i < num_args; i++ )
	{
		Object ob = stack.back();
		frame->SetLocal( num_args-i-1, ob );
		stack.pop_back();
	}
	// push the frame onto the callstack
	PushFrame( frame );
	PushScope( scope );
	nf.p( frame );
	PopScope();
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
	for( map<string,Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		Function* f = i->second->f;
		cout << "function: " << f->name << ", from file: " << f->filename << ", line: " << f->first_line;
		cout << endl;
		cout << f->num_args << " arg(s), default value indices: ";
		for( int j = 0; j < f->default_args.Size(); j++ )
			cout << f->default_args.At( j ) << " ";
		cout << endl << f->num_locals << " local(s): ";
		for( int j = 0; j < f->local_names.size(); j++ )
			cout << f->local_names.operator[]( j ) << " ";
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
