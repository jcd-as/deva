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
#include "string_builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"
#include "api.h"

#include <algorithm>
#include <sstream>

using namespace std;


namespace deva
{

/////////////////////////////////////////////////////////////////////////////
// executive/VM functions and globals
/////////////////////////////////////////////////////////////////////////////

// global executor object
Executor* ex = NULL;
// static member initialization
// singleton support:
bool Executor::instantiated = false;

Executor::Executor() : ip( NULL ), debug( false ), trace( false ), is_error( false )
{
	if( instantiated )
		throw ICE( "Executor is a singleton object, it cannot be instantiated twice." );
	else
		instantiated = true;

	// set-up the constant pool
	constants.reserve( 256 );

	// add names that always exist
	AddConstant( Object( true ) );
	AddConstant( Object( false ) );
	AddConstant( Object( obj_null ) );
	AddConstant( Object( obj_symbol_name, copystr( "__name__" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "__class__" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "__bases__" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "__module__" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "new" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "delete" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "self" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "rewind" ) ) );
	AddConstant( Object( obj_symbol_name, copystr( "next" ) ) );
}

Executor::~Executor()
{
	// free the constants' string data
	for( int i = 0; i < constants.size(); i++ )
	{
		ObjectType type = constants.at( i ).type;
		if( type == obj_string || type == obj_symbol_name ) delete [] constants.at( i ).s;
	}
	// free the function objects
	for( multimap<string, Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		string s = i->first;
		Object* f = i->second;
		if( i->second->type == obj_function )
			delete i->second->f;
		delete i->second;
	}
	// free the code blocks
	for( vector<const Code*>::iterator i = code_blocks.begin(); i != code_blocks.end(); ++i )
	{
		delete *i;
	}
}

Object* Executor::FindFunction( string name, size_t offset )
{
	multimap<string, Object*>::iterator i = functions.find( name );
	for( ; i != functions.end(); ++i )
	{
		if( i->second->type == obj_function )
		{
			if( (size_t)i->second->f->addr == offset )
				return i->second;
		}
		else if( i->second->type == obj_native_function )
		{
			if( (size_t)i->second->nf.p == offset )
				return i->second;
		}
		else
			throw ICE( "Non-function found in Executor's function table." );
	}
	return NULL;
}

// return a resolved symbol - if sym is a symbol name, find the symbol,
// otherwise return sym unmodified
Object Executor::ResolveSymbol( Object sym )
{
	if( sym.type != obj_symbol_name )
		return sym;

	Object* obj = scopes->FindSymbol( sym.s );

	if( !obj )
	{
		// builtin?
		NativeFunction nf = GetBuiltin( sym.s );
		if( nf.p )
			return Object( nf );
		// string builtin?
		nf = GetStringBuiltin( sym.s );
		if( nf.p )
			return Object( nf );
		// vector builtin?
		nf = GetVectorBuiltin( sym.s );
		if( nf.p )
			return Object( nf );
		// map builtin?
		nf = GetMapBuiltin( sym.s );
		if( nf.p )
			return Object( nf );
		// module name?
		if( module_names.count( sym.s ) != 0 )
			return sym;

		throw RuntimeException( boost::format( "Undefined symbol '%1%'." ) % sym.s );
	}

	return *obj;
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
	// get the 'new' method (constructor) of this class and call it
	Map::iterator it = o.m->find( Object( obj_symbol_name, "new" ) );
	if( it != o.m->end() )
	{
		// push the new instance onto the stack
		stack.push_back( instance );
		IncRef( instance );

		if( it->second.type != obj_function )
			throw RuntimeException( "'new' method of instance object is not a function." );
		ExecuteFunction( it->second.f, num_args, true );
		// pop the (null) return value
		stack.pop_back();

		// if a constructor was actually called, the return op will dec
		// ref the 'self' arg, so we don't need to decref the instance
	}
}

// recursively call destructors on an object and its base classes
void Executor::CallDestructors( Object o )
{
	// call the destructor for this (most derived) class
	// get the 'delete' method (destructor) of this class and call it
	Map::iterator it = o.m->find( Object( obj_symbol_name, "delete" ) );
	if( it != o.m->end() )
	{
		// push the instance onto the stack
		stack.push_back( o );
		IncRef( o );

		if( it->second.type != obj_function )
			throw RuntimeException( "'delete' method of instance object is not a function." );
		ExecuteFunction( it->second.f, 0, true, true );
		// pop the (null) return value
		stack.pop_back();

		// if a constructor was actually called, the return op will dec
		// ref the 'self' arg, so we don't need to decref the instance
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

void Executor::Execute( const Code* const code )
{
	// NOTE: all actions that create a new C string (char*) as a local need to
	// add it to that frame's strings collection (i.e. call
	// CurrentFrame()->AddString())

//	Opcode op = op_nop;

//	end = code->code + code->len;
//	bp = code->code;
//	ip = bp;

	scopes = new ScopeTable();

	// a starting ('global') frame
	Object *main = FindFunction( string( "@main" ), 0 );
//	Frame* frame = new Frame( NULL, scopes, bp, bp, 0, main->f );
	Frame* frame = new Frame( NULL, scopes, code->code, code->code, 0, main->f );
	PushFrame( frame );

	// make sure the global scope is always around
	scopes->PushScope( new Scope() );

	if( trace )
		cout << "Execution trace:" << endl;

	// execute until end
	ExecuteCode( code );
//	while( ip < end && op < op_halt )
//	{
//		op = ExecuteInstruction();
//	}

	// pop the 'global' scope
	scopes->PopScope();
	// free the scope table
	delete scopes;
	PopFrame();
}

// TODO: implement
void Executor::ExecuteCode( const Code* const code )
{
	// add this code block to our collection
	code_blocks.push_back( code );

	Opcode op = op_nop;

	// set the ip, bp and end 
	end = code->code + code->len;
	bp = code->code;
	ip = bp;

	// execute until end
	while( ip < end && op < op_halt )
	{
		op = ExecuteInstruction();
	}
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

// helper for slicing vectors in steps (see vector_builtins.cpp for fcn def)
static size_t s_step = 1;
static size_t s_i = 0;
bool if_step_ex( Object )
{
	if( s_i++ % s_step == 0 )
		return false;
	else
		return true;
}

Opcode Executor::ExecuteInstruction()
{
	dword arg, arg2, arg3;
	Object o, lhs, rhs, *plhs;

	// decode opcode
	Opcode op = (Opcode)*ip;

	if( trace )
	{
		PrintOpcode( op, bp, ip );
		// print the top five stack items
		DumpStackTop();
	}

	ip++;
	switch( op )
	{
	case op_nop:
		break;
	case op_pop:
		{
			Object tmp = ResolveSymbol( stack.back() );
			DecRef( tmp );
		}
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
		arg = *((dword*)ip);
		ip += sizeof( dword );
		stack.push_back( CurrentFrame()->GetLocal( arg ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal0:
		stack.push_back( CurrentFrame()->GetLocal( 0 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal1:
		stack.push_back( CurrentFrame()->GetLocal( 1 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal2:
		stack.push_back( CurrentFrame()->GetLocal( 2 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal3:
		stack.push_back( CurrentFrame()->GetLocal( 3 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal4:
		stack.push_back( CurrentFrame()->GetLocal( 4 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal5:
		stack.push_back( CurrentFrame()->GetLocal( 5 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal6:
		stack.push_back( CurrentFrame()->GetLocal( 6 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal7:
		stack.push_back( CurrentFrame()->GetLocal( 7 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal8:
		stack.push_back( CurrentFrame()->GetLocal( 8 ) );
		IncRef( stack.back() );
		break;
	case op_pushlocal9:
		stack.push_back( CurrentFrame()->GetLocal( 9 ) );
		IncRef( stack.back() );
		break;
	case op_pushconst:
		// 1 arg: index to constant
		arg = *((dword*)ip);
		{
			// TODO: Resolve the constant sym *here* and remove all the calls to
			// ResolveSymbol when an obj is popped off the stack ???
			Object tmp = GetConstant( arg );
			Object tmp2 = ResolveSymbol( tmp );
			IncRef( tmp2 );
			stack.push_back( tmp );
		}
		ip += sizeof( dword );
		break;
	case op_storeconst:
		// 1 arg
		arg = *((dword*)ip);
		// look-up the constant
		o = GetConstant( arg );
		// find the variable
		plhs = FindSymbol( o.s );
		if( !plhs )
			throw RuntimeException( boost::format( "Symbol '%1%' not found." ) % o.s );
		rhs = stack.back();
		stack.pop_back();
		DecRef( *plhs );
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
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( arg, rhs );
		ip += sizeof( dword );
		break;
	case op_storelocal0:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 0, rhs );
		break;
	case op_storelocal1:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 1, rhs );
		break;
	case op_storelocal2:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 2, rhs );
		break;
	case op_storelocal3:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 3, rhs );
		break;
	case op_storelocal4:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 4, rhs );
		break;
	case op_storelocal5:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 5, rhs );
		break;
	case op_storelocal6:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 6, rhs );
		break;
	case op_storelocal7:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 7, rhs );
		break;
	case op_storelocal8:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 8, rhs );
		break;
	case op_storelocal9:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 9, rhs );
		break;
	case op_def_local:
		{
		// 1 arg
		arg = *((dword*)ip);
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( arg, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( arg ), CurrentFrame()->GetLocalRef( arg ) );
		ip += sizeof( dword );
		}
		break;
	case op_def_local0:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 0, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 0 ), CurrentFrame()->GetLocalRef( 0 ) );
		break;
	case op_def_local1:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 1, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 1 ), CurrentFrame()->GetLocalRef( 1 ) );
		break;
	case op_def_local2:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 2, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 2 ), CurrentFrame()->GetLocalRef( 2 ) );
		break;
	case op_def_local3:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 3, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 3 ), CurrentFrame()->GetLocalRef( 3 ) );
		break;
	case op_def_local4:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 4, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 4 ), CurrentFrame()->GetLocalRef( 4 ) );
		break;
	case op_def_local5:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 5, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 5 ), CurrentFrame()->GetLocalRef( 5 ) );
		break;
	case op_def_local6:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 6, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 6 ), CurrentFrame()->GetLocalRef( 6 ) );
		break;
	case op_def_local7:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 7, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 7 ), CurrentFrame()->GetLocalRef( 7 ) );
		break;
	case op_def_local8:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 8, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 8 ), CurrentFrame()->GetLocalRef( 8 ) );
		break;
	case op_def_local9:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 9, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 9 ), CurrentFrame()->GetLocalRef( 9 ) );
		break;
	case op_def_function:
		{
		// 2 args: constant index of fcn name, address of fcn
		arg2 = *((dword*)ip);
		ip += sizeof( dword );
		arg3 = *((dword*)ip);
		ip += sizeof( dword );
		// find the constant
		Object fcnname = GetConstant( arg2 );
		if( fcnname.type != obj_string && fcnname.type != obj_symbol_name )
			throw ICE( "def_function instruction called with an object that is not a function name." );
		string name( fcnname.s );
		// find the function
		Object* objf = FindFunction( name, (size_t)arg3 );
		if( !objf )
			throw RuntimeException( boost::format( "Function '%1%' not found." ) % name );
		// is there a local of this name that we need to override?
		Object* local_obj = CurrentScope()->FindSymbol( fcnname.s );
		if( local_obj )
		{
			// get the index of the local
			int local_idx = CurrentScope()->FindSymbolIndex( local_obj, CurrentFrame() );
			// set the local in the current frame
			if( local_idx != -1 )
				CurrentFrame()->SetLocal( local_idx, *objf );
			// define the local in the current scope
			// (this frame cannot be native fcn, obviously)
			CurrentScope()->AddSymbol( string( fcnname.s ), local_obj );
		}
		// add the function to the local scope's fcn collection
		CurrentScope()->AddFunction( name, objf );
		}
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
			rhs = ResolveSymbol( rhs );
			stack.pop_back();
			lhs = stack.back();
			lhs = ResolveSymbol( lhs );
			stack.pop_back();
			m.m->insert( pair<Object, Object>( lhs, rhs ) );
		}
		IncRef( m );
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
			o = ResolveSymbol( o );
			stack.pop_back();
			v.v->operator[]( arg-i-1 ) = o;
		}
		IncRef( v );
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
		// walk in right-to-left (as seen in code) order
		for( int i = arg; i > 0; i-- )
		{
			Object base = stack.back();
			stack.pop_back();
			if( base.type != obj_class )
				throw ICE( "Base class expected. Bad code gen? Corrupt stack?" );
			// don't dec ref the base class here, because we're adding it to our
			// bases vector, which is another ref on it

			// merge the base class into the new class
			for( Map::iterator i = base.m->begin(); i != base.m->end(); ++i )
			{
				if( i->second.type == obj_function )
				{
					if( i->first.type != obj_symbol_name )
						throw RuntimeException( boost::format( "Invalid method in base class '%1%'" ) % base );
					// don't copy 'new' or 'delete'
					if( strcmp( "new", i->first.s ) == 0 || strcmp( "delete", i->first.s ) == 0 )
						continue;
					// override any existing method
					pair<Object, Object> pr = make_pair( i->first, i->second );
					Map::iterator fi = m.m->find( i->first );
					if( fi != m.m->end() )
					{
						// exists, we need to replace it
						m.m->erase( fi );
						m.m->insert( pair<Object, Object>( i->first, i->second ) );
					}
					else
						m.m->insert( pair<Object, Object>( i->first, i->second ) );
				}
			}

			// add to the list of parents
			v->push_back( base );
		}
		Object basesObj( v );
		IncRef( basesObj );
		Object _bases = GetConstant( FindConstant( Object( obj_symbol_name, "__bases__" ) ) );
		m.m->insert( pair<Object, Object>( _bases, basesObj ) );

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
		IncRef( m );
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
		o = ResolveSymbol( o );
		stack.pop_back();
		if( !o.CoerceToBool() )
			ip = (byte*)(bp + arg);
		else
			ip += sizeof( dword );
		break;
	case op_eq:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		if( lhs.type != rhs.type )
			throw RuntimeException( "Equality operator used on operands of different types." );
		switch( lhs.type )
		{
		case obj_null: stack.push_back( Object( rhs.type == obj_null ) ); break;
		case obj_boolean: stack.push_back( Object( lhs.b == rhs.b ) ); break;
		case obj_number: stack.push_back( Object( lhs.d == rhs.d ) ); break;
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		stack.push_back( Object( lhs.CoerceToBool() || rhs.CoerceToBool() ) );
		break;
	case op_and:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		stack.push_back( Object( lhs.CoerceToBool() && rhs.CoerceToBool() ) );
		break;
	case op_neg:
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();
		if( o.type != obj_number )
			throw RuntimeException( "Negate operator can only be used on numeric objects." );
		stack.push_back( Object( -o.d ) );
		break;
	case op_not:
		o = stack.back();
		o = ResolveSymbol( o );
		DecRef( o );
		stack.pop_back();
		stack.push_back( Object( !o.CoerceToBool() ) );
		break;
	case op_add:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
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
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();
		if( lhs.type != obj_number )
			throw RuntimeException( "Left-hand side of subtraction operator must be a number." );
		if( rhs.type != obj_number )
			throw RuntimeException( "Right-hand side of subtraction operator must be a number." );
		stack.push_back( Object( lhs.d - rhs.d ) );
		break;
	case op_mul:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();
		if( lhs.type != obj_number )
			throw RuntimeException( "Left-hand side of multiplication operator must be a number." );
		if( rhs.type != obj_number )
			throw RuntimeException( "Right-hand side of multiplication operator must be a number." );
		stack.push_back( Object( lhs.d * rhs.d ) );
		break;
	case op_div:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
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
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();
		if( lhs.type != obj_number )
			throw RuntimeException( "Left-hand side of modulus operator must be a number." );
		if( rhs.type != obj_number )
			throw RuntimeException( "Right-hand side of modulus operator must be a number." );
		if( rhs.d == 0.0 )
			throw RuntimeException( "Division by zero fault." );
		// error if arguments aren't integral numbers...
		if( !is_integral( lhs.d ) || !is_integral( rhs.d ) )
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
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
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
		rhs = ResolveSymbol( rhs );
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
		rhs = ResolveSymbol( rhs );
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
		rhs = ResolveSymbol( rhs );
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
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		if( plhs->type != obj_number )
			throw RuntimeException( "Left-hand side of modulus assignment operator must be a number." );
		if( rhs.type != obj_number && rhs.type != obj_string )
			throw RuntimeException( "Right-hand side of modulus assignment operator must be a number." );
		if( rhs.d == 0.0 )
			throw RuntimeException( "Divide-by-zero error." );
		// error if arguments aren't integral numbers...
		if( !is_integral( lhs.d ) || !is_integral( rhs.d ) )
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
		rhs = ResolveSymbol( rhs );
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
		rhs = ResolveSymbol( rhs );
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
		rhs = ResolveSymbol( rhs );
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
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		if( lhs.type != obj_number )
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
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		if( lhs.type != obj_number )
			throw RuntimeException( "Left-hand side of modulus assignment operator must be a number." );
		if( rhs.type != obj_number )
			throw RuntimeException( "Right-hand side of modulus assignment operator must be a number." );
		if( rhs.d == 0.0 )
			throw RuntimeException( "Divide-by-zero error." );
		// error if arguments aren't integral numbers...
		if( !is_integral( lhs.d ) || !is_integral( rhs.d ) )
			throw RuntimeException( "Operands in modulus operator must be integral numbers." );
		CurrentFrame()->SetLocal( arg, Object( (double)((int)lhs.d % (int)rhs.d) ) );
		ip += sizeof( dword );
		break;
	case op_call: // call function with <Op0> args on on stack, fcn after args
	case op_call_method: // call function with <Op0> args on on stack, fcn after args
		{
		// 1 arg: number of args passed
		arg = *((dword*)ip);
		ip += sizeof( dword );
		// get the fcn
		o = stack.back();
		Object tmp = ResolveSymbol( o );
		DecRef( tmp );
		stack.pop_back();
		// TODO: if addr relative to another code block (module)??
		//
		Object callable;
		// is it a symbol name that we need to look-up?
		if( o.type == obj_symbol_name )
		{
			Object* callablePtr = scopes->FindSymbol( o.s );
			if( !callablePtr )
			{
				// builtin?
				NativeFunction nf = GetBuiltin( o.s );
				if( nf.p )
					callable = Object( nf );
				else
					throw RuntimeException( boost::format( "Invalid function or callable object name '%1%'." ) % o.s );
			}
			else
				callable = *callablePtr;
		}
		else
			callable = o;

		// if it's a fcn, build a frame for it
		if( callable.type == obj_function )
		{
			if( op == op_call_method && !callable.f->IsMethod() )
			{
				// if the number of arguments including 'self' is correct,
				// continue ahead
				if( callable.f->num_args == arg )
					ExecuteFunction( callable.f, arg, false );
				// if there are one too many args, pop 'self', it was pushed
				// because we couldn't tell if this was a fcn or method at
				// compile time
				else if( callable.f->num_args == arg-1 )
				{
					if( stack.size() > 0 )
						stack.pop_back();
					else
						throw RuntimeException( "Call to a method expected but call to a non-method found." );

					ExecuteFunction( callable.f, arg, false );
				}
				else
					throw RuntimeException( "Call to a method expected but call to a non-method found." );
			}
			else
				ExecuteFunction( callable.f, arg, op == op_call_method );
		}
		// is it a native fcn?
		else if( callable.type == obj_native_function )
		{
			ExecuteFunction( callable.nf, arg, op == op_call_method );
		}
		// is it a class object (i.e. a constructor call)
		else if( callable.type == obj_class )
		{
			// - create a copy of the class object
			Map* inst = CreateMap( *callable.m );

			// - add the __class__ member to it
			Object _class = GetConstant( FindConstant( Object( obj_symbol_name, "__class__" ) ) );
			Map::iterator i = callable.m->find( Object( obj_symbol_name, "__name__" ) );
			if( i == callable.m->end() )
				throw ICE( "Unable to find '__name__' member in class object." );
			inst->insert( pair<Object, Object>( _class, i->second ) );

			// create our instance
			Object instance;
			instance.MakeInstance( inst );

			// inc ref it before we do anything with it
			IncRef( instance );
			// and its children, which we just copied
			IncRefChildren( instance );

			// recursively call the constructors on this object and its base classes
			CallConstructors( callable, instance, arg );

			stack.push_back( instance );
		}
		else
			throw RuntimeException( boost::format( "Object '%1%' is not a function." ) % callable );
		}
		break;
	case op_return:
		// TODO: better way: add a Frame::MoveString() method that moves the
		// string ownership from one scope to another
		//
		// if a string is being returned, it needs to be copied to the calling
		// frame! (string mem in a frame is freed on frame exit)
		if( stack.back().type == obj_string )
		{
			Object o = stack.back();
			stack.pop_back();
			const char* str = CurrentFrame()->GetParent()->AddString( string( o.s ) );
			stack.push_back( Object( str ) );
		}

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
			IncRef( stack.back() );
			// call 'next'
			ExecuteFunction( nf, 0, true );
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
				IncRef( stack.back() );
				ExecuteFunction( it->second.f, 0, true );
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
				IncRef( stack.back() );
				// call 'next'
				ExecuteFunction( nf, 0, true );
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
				IncRef( stack.back() );
				stack.push_back( ov.v->operator[]( 1 ) );
				IncRef( stack.back() );
			}
			// a vector will just be the value we want
			else
			{
				stack.push_back( o.v->operator[]( 1 ) );
				IncRef( stack.back() );
			}
		}
		DecRef( o );
		}
		break;
	case op_tbl_load:// tos = tos1[tos]
		rhs = stack.back();
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) && lhs.type != obj_string )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// string:
		if( lhs.type == obj_string )
		{
			// TODO: handle string built-in methods
			//
			// validate the indexer type
			if( rhs.type != obj_number || !is_integral( rhs.d ) )
				throw RuntimeException( "Argument to string indexer must be an integral number." );
			// validate the bounds
			if( rhs.d > strlen( lhs.s ) )
				throw RuntimeException( boost::format( "Out-of-bounds in string index: '%1%' is greater than the length of '%2%'" ) % rhs.d % lhs.s );
			// create a new (single-character) string of the indexed character,
			char* c = new char[2];
			c[0] = lhs.s[(size_t)rhs.d];
			c[1] = '\0';
			// add it to the current scope
			CurrentFrame()->AddString( c );
			// return it on the stack
			stack.push_back( Object( c ) );
		}
		// vector:
		else if( IsVecType( lhs.type ) )
		{
			if( rhs.type == obj_string || rhs.type == obj_symbol_name )
			{
				// check for vector built-in method
				NativeFunction nf = GetVectorBuiltin( string( rhs.s ) );
				if( nf.p )
				{
					if( !nf.is_method )
						throw ICE( "Vector builtin not marked as a method." );
					stack.push_back( Object( nf ) );
				}
				else
					throw RuntimeException( "Invalid vector index or method." );
				DecRef( lhs );
				DecRef( rhs );
				break;
			}
			if( rhs.type != obj_number )
				throw RuntimeException( "Index to a vector must be a numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
				throw RuntimeException( "Index to a vector must be an integral value." );
			int idx = (int)rhs.d;
			// out-of-bounds check
			if( lhs.v->size() <= idx || idx < 0 )
				throw RuntimeException( "Out-of-bounds error indexing vector." );
			Object obj = lhs.v->operator[]( idx );
			IncRef( obj );
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
				// conversely, if this was a string, try looking for it as a
				// symbol name
				if( rhs.type == obj_symbol_name || rhs.type == obj_string )
				{
					// look for it in the map first...
					Map::iterator it = lhs.m->find( Object( rhs.s ) );
					if( it != lhs.m->end() )
					{
						Object obj = it->second;
						if( obj.type == obj_function || obj.type == obj_native_function )
						{
							if( obj.type == obj_function && !obj.f->IsMethod() )
								throw ICE( "class/instance method not marked as a method." );
							if( obj.type == obj_native_function && !obj.nf.is_method )
								throw ICE( "class/instance native method not marked as a method." );
						}
						// push the object
						stack.push_back( obj );
					}
					// try it as a symbol name
					it = lhs.m->find( Object( obj_symbol_name, rhs.s ) );
					if( it != lhs.m->end() )
					{
						Object obj = it->second;
						// push the object
						stack.push_back( obj );
					}
					// else try it as a built-in method
					else
					{
						// check for map built-in method
						NativeFunction nf = GetMapBuiltin( string( rhs.s ) );
						if( nf.p )
						{
							if( !nf.is_method )
								throw ICE( "Map builtin not marked as a method." );
							stack.push_back( Object( nf ) );
						}
						else
							throw RuntimeException( boost::format( "Invalid map key or method: '%1%'." ) % rhs.s );

					}
					DecRef( lhs );
					DecRef( rhs );
					break;
				}
				else 
					throw RuntimeException( "Invalid key value. Item not found." );
			}
			IncRef( i->second );
			stack.push_back( i->second );
		}
		DecRef( lhs );
		DecRef( rhs );
		break;
	case op_method_load:// tos = tos1[tos], but leaves tos1 ('self') on the stack
		{
		rhs = stack.back();
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) && lhs.type != obj_string )
			throw RuntimeException( boost::format( "'%1%' is not a type that has methods (string, vector, map, class or instance)." ) % lhs );

		// string:
		if( lhs.type == obj_string )
		{
			if( rhs.type != obj_string && rhs.type != obj_symbol_name )
				throw RuntimeException( boost::format( "Expected method name, found '%1%'." ) % rhs );

			// check for string built-in method
			NativeFunction nf = GetStringBuiltin( string( rhs.s ) );
			if( nf.p )
			{
				if( !nf.is_method )
					throw ICE( "String builtin not marked as a method." );
				stack.push_back( lhs );
				IncRef( lhs );
				stack.push_back( Object( nf ) );
			}
			else
				throw RuntimeException( "Invalid string method." );

			break;
		}
		// vector:
		else if( lhs.type == obj_vector )
		{
			if( rhs.type != obj_string && rhs.type != obj_symbol_name )
				throw RuntimeException( boost::format( "Expected method name, found '%1%'." ) % rhs );

			// check for vector built-in method
			NativeFunction nf = GetVectorBuiltin( string( rhs.s ) );
			if( nf.p )
			{
				if( !nf.is_method )
					throw ICE( "Vector builtin not marked as a method." );
				stack.push_back( lhs );
				IncRef( lhs );
				stack.push_back( Object( nf ) );
			}
			else
				throw RuntimeException( "Invalid vector method." );

			DecRef( lhs );
			break;
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
				// conversely, if it is a string, try looking for it as a symbol
				// name 
				if( rhs.type == obj_symbol_name )
				{
					// look for it in the map first...
					Map::iterator it = lhs.m->find( Object( rhs.s ) );
					// try it as a symbol name
					if( it == lhs.m->end() )
						it = lhs.m->find( Object( obj_symbol_name, rhs.s ) );
					if( it != lhs.m->end() )
					{
						Object obj = it->second;
						if( obj.type == obj_function || obj.type == obj_native_function )
						{
							if( obj.type == obj_function && !obj.f->IsMethod() )
								throw ICE( "class/instance method not marked as a method." );
							if( obj.type == obj_native_function && !obj.nf.is_method )
								throw ICE( "class/instance native method not marked as a method." );
							// push the class/instance ('this') for instances
							if( lhs.type == obj_instance )
							{
								IncRef( lhs );
								stack.push_back( lhs );
							}
						}
						// TODO: IncRef the object??
						IncRef( obj );
						// push the object
						stack.push_back( obj );
					}
					// try it as a built-in method
					else
					{
						// check for map built-in method
						NativeFunction nf = GetMapBuiltin( string( rhs.s ) );
						if( nf.p )
						{
							if( !nf.is_method )
								throw ICE( "Map builtin not marked as a method." );
							stack.push_back( lhs );
							IncRef( lhs );
							stack.push_back( Object( nf ) );
						}
						else
							throw RuntimeException( boost::format( "Invalid map key or method: '%1%'." ) % rhs.s );

					}
					DecRef( lhs );
					break;
				}
				else if( rhs.type == obj_string )
				{
					// check for class/instance method
					if( lhs.type == obj_class || lhs.type == obj_instance )
					{
						// find the object by name in the map
						Map::iterator i = lhs.m->find( Object( rhs.s ) );
						// not found as a string, try as a symbol name 
						// (for methods, which won't be entered as strings)
						if( i == lhs.m->end() )
							i = lhs.m->find( Object( obj_symbol_name, rhs.s ) );
						if( i != lhs.m->end() )
						{
							Object obj = i->second;
							if( obj.type != obj_function )
								throw RuntimeException( boost::format( "Invalid method: '%1%'." ) % rhs.s );
							// push the class/instance ('this')
							if( lhs.type == obj_instance || lhs.type == obj_class )
							{
								IncRef( lhs );
								stack.push_back( lhs );
							}
							// push the function
							stack.push_back( obj );
							DecRef( lhs );
							break;
						}
						else
							throw RuntimeException( boost::format( "Invalid map key or method: '%1%'." ) % rhs.s );
					}
					else
					{
						// check for map built-in method
						NativeFunction nf = GetMapBuiltin( string( rhs.s ) );
						if( nf.p )
						{
							if( !nf.is_method )
								throw ICE( "Map builtin not marked as a method." );
							stack.push_back( lhs );
							IncRef( lhs );
							stack.push_back( Object( nf ) );
						}
						else
							throw RuntimeException( boost::format( "Invalid map key or method: '%1%'." ) % rhs.s );
					}
					DecRef( lhs );
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
					{
						stack.push_back( lhs );
						IncRef( lhs );
					}
				}
			}
			// push the object
			IncRef( i->second );
			stack.push_back( i->second );
		}

		DecRef( lhs );
		DecRef( rhs );
		}
		break;
	case op_loadslice2:// tos = tos2[tos1:tos]
		{
		Object idx2 = stack.back();
		DecRef( idx2 );
		idx2 = ResolveSymbol( idx2 );
		stack.pop_back();
		Object idx1 = stack.back();
		DecRef( idx1 );
		idx1 = ResolveSymbol( idx1 );
		stack.pop_back();
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();

		if( !is_integral( idx1.type ) && idx1.type != obj_null )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );
		if( !is_integral( idx2.type ) && idx2.type != obj_null )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );

		// string
		if( o.type == obj_string )
		{
			int start, end;
			size_t sz = strlen( o.s );
			if( sz == 0 )
			{
				start = 0;
				end = 0;
			}
			else
			{
				start = idx1.type == obj_null ? sz : (int)idx1.d;
				end = idx2.type == obj_null ? sz : (int)idx2.d;
			}

			// handle negative values
			if( start < 0 )
				start = sz + start;
			if( end < 0 )
				end = sz + end;

			// check the indices & step value and convert to sane values, if
			// necessary
			if( start > sz )
				start = sz;
			if( start < 0 )
				start = 0;
			if( end > sz )
				end = sz;
			if( end < 0 )
				end = 0;
			if( end < start )
				end = start;

			// slice the string
			// (strings are immutable. create a copy and add it to the calling frame)
			string s( o.s );
			string r = s.substr( start, end - start );
			const char* ret = CurrentFrame()->AddString( r );
			stack.push_back( Object( ret ) );
		}
		else if( o.type == obj_vector )
		{
			int start, end;
			size_t sz = o.v->size();
			if( sz == 0 )
			{
				start = 0;
				end = 0;
			}
			else
			{
				start = idx1.type == obj_null ? sz : (int)idx1.d;
				end = idx2.type == obj_null ? sz : (int)idx2.d;
			}

			// handle negative values
			if( start < 0 )
				start = sz + start;
			if( end < 0 )
				end = sz + end;

			// check the indices & step value and convert to sane values, if
			// necessary
			if( start > sz )
				start = sz;
			if( start < 0 )
				start = 0;
			if( end > sz )
				end = sz;
			if( end < 0 )
				end = 0;
			if( end < start )
				end = start;

			// slice the vector
			Object ret;
			// create a new vector object that is a copy of the 'sub-vector' we're
			// looking for
			Vector* v = CreateVector( *(o.v), start, end );
			ret = Object( v );

			IncRef( ret );
			stack.push_back( ret );
		}
		else
			throw RuntimeException( boost::format( "Invalid slice: '%1%' is not a vector or string." ) % o );

		DecRef( o );
		}
		break;
	case op_loadslice3:// tos = tos3[tos2:tos1:tos]
		{
		Object idx3 = stack.back();
		DecRef( idx3 );
		idx3 = ResolveSymbol( idx3 );
		stack.pop_back();
		Object idx2 = stack.back();
		DecRef( idx2 );
		idx2 = ResolveSymbol( idx2 );
		stack.pop_back();
		Object idx1 = stack.back();
		DecRef( idx1 );
		idx1 = ResolveSymbol( idx1 );
		stack.pop_back();
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();

		if( !is_integral( idx1.type ) && idx1.type != obj_null )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );
		if( !is_integral( idx2.type ) && idx2.type != obj_null )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );
		if( !is_integral( idx2.type ) )
			throw RuntimeException( "'step' value in slice must be an integral number." );

		int step = (int)idx3.d;

		// string
		if( o.type == obj_string )
		{
			int start, end;
			size_t sz = strlen( o.s );
			if( sz == 0 )
			{
				start = 0;
				end = 0;
			}
			else
			{
				start = idx1.type == obj_null ? sz : (int)idx1.d;
				end = idx2.type == obj_null ? sz : (int)idx2.d;
			}

			// handle negative values
			if( start < 0 )
				start = sz + start;
			if( end < 0 )
				end = sz + end;

			// check the indices & step value and convert to sane values, if
			// necessary
			if( start > sz )
				start = sz;
			if( start < 0 )
				start = 0;
			if( end > sz )
				end = sz;
			if( end < 0 )
				end = 0;
			if( end < start )
				end = start;
			if( step < 1 )
				throw RuntimeException( "Invalid 'step' argument in slice: 'step' is less than one." );

			// slice the string
			// (strings are immutable. create a copy and add it to the calling frame)
			string s( o.s );
			if( step == 1 )
			{
				string r = s.substr( start, end - start );
				const char* ret = CurrentFrame()->AddString( r );
				stack.push_back( Object( ret ) );
			}
			// otherwise the string class doesn't help us, have to do it manually
			else
			{
				// first get the substring from start to end positions
				string r = s.substr( start, end - start );
				// TODO: call 'reserve' on the string to reduce allocations?
				// then walk it grabbing every 'nth' character
				string slice;
				for( int i = 0; i < r.length(); i += step )
				{
					slice += r[i];
				}
				const char* ret = CurrentFrame()->AddString( slice );
				stack.push_back( Object( ret ) );
			}
		}
		else if( o.type == obj_vector )
		{
			int start, end;
			size_t sz = o.v->size();
			if( sz == 0 )
			{
				start = 0;
				end = 0;
			}
			else
			{
				start = idx1.type == obj_null ? sz : (int)idx1.d;
				end = idx2.type == obj_null ? sz : (int)idx2.d;
			}

			// handle negative values
			if( start < 0 )
				start = sz + start;
			if( end < 0 )
				end = sz + end;

			// check the indices & step value and convert to sane values, if
			// necessary
			if( start > sz )
				start = sz;
			if( start < 0 )
				start = 0;
			if( end > sz )
				end = sz;
			if( end < 0 )
				end = 0;
			if( end < start )
				end = start;
			if( step < 1 )
				throw RuntimeException( "Invalid 'step' argument in slice: 'step' is less than one." );

			// slice the vector
			Object ret;
			// if 'step' is '1' (the default)
			if( step == 1 )
			{
				// create a new vector object that is a copy of the 'sub-vector' we're
				// looking for
				Vector* v = CreateVector( *(o.v), start, end );
				ret = Object( v );
			}
			// otherwise the vector class doesn't help us, have to do it manually
			else
			{
				Vector* v = CreateVector();
				s_i = 0;
				s_step = step;
				remove_copy_if( o.v->begin() + start, o.v->begin() + end, back_inserter( *v ), if_step_ex );
				ret = Object( v );
			}

			IncRef( ret );
			stack.push_back( ret );
		}
		else
			throw RuntimeException( boost::format( "Invalid slice: '%1%' is not a vector or string." ) % o );

		DecRef( o );
		}
		break;
	case op_tbl_store:// tos2[tos1] = tos
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) && lhs.type != obj_string )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// string:
		if( lhs.type == obj_string )
		{
			// TODO: handle string built-in methods
			//
			// validate the indexer type
			if( rhs.type != obj_number || !is_integral( rhs.d ) )
				throw RuntimeException( "Argument to string indexer must be an integral number." );
			// validate the bounds
			if( rhs.d > strlen( lhs.s ) )
				throw RuntimeException( boost::format( "Out-of-bounds in string index: '%1%' is greater than the length of '%2%'" ) % rhs.d % lhs.s );

			// strings are immutable, so we need to create a new string with the 
			// modified contents and add it to the current scope's string collection
			char* s = copystr( lhs.s );
			size_t idx = (size_t)rhs.d;
			s[idx] = lhs.s[idx];
			// add it to the current scope
			CurrentFrame()->AddString( s );
			// return it on the stack
			stack.push_back( Object( s ) );
		}
		// vector:
		if( IsVecType( lhs.type ) )
		{
			if( rhs.type != obj_number )
				throw RuntimeException( "Vectors can only be indexed with numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
				throw RuntimeException( "Index to a vector must be an integral value." );
			int idx = (int)rhs.d;
			// out-of-bounds check
			if( lhs.v->size() <= idx || idx < 0 )
				throw RuntimeException( "Out-of-bounds error indexing vector." );
			// dec ref the current tos2[tos1], as we're assigning into it
			DecRef( lhs.v->operator[]( idx ) );
			// set the new value
			lhs.v->operator[]( idx ) = o;
		}
		// map/class/instance:
		else
		{
			// TODO: deva1 seemed to allow only numbers, strings and UDTs as
			// keys... ???
			// dec ref the current tos2[tos1], as we're assigning into it
			DecRef( lhs.m->operator[]( rhs ) );
			// set the new value
			lhs.m->operator[]( rhs ) = o;
		}
		break;
	case op_storeslice2:
		{
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		Object idx2 = stack.back();
		idx2 = ResolveSymbol( idx2 );
		stack.pop_back();
		Object idx1 = stack.back();
		idx1 = ResolveSymbol( idx1 );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();

		if( !IsVecType( lhs.type ) )
			throw RuntimeException( boost::format( "Invalid object for slice assignment: '%1%' is not a vector." ) % lhs );
		if( !is_integral( idx1.type ) )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );
		if( !is_integral( idx2.type ) )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );

		size_t sz = lhs.v->size();
		if( sz != 0 )
		{
			int start = idx1.type == obj_null ? sz : (int)idx1.d;
			int end = idx2.type == obj_null ? sz : (int)idx2.d;

			// handle negative values
			if( start < 0 )
				start = sz + start;
			if( end < 0 )
				end = sz + end;

			// check the indices & step value and convert to sane values, if
			// necessary
			if( start > sz )
				start = sz;
			if( start < 0 )
				start = 0;
			if( end > sz )
				end = sz;
			if( end < 0 )
				end = 0;
			if( end < start )
				end = start;

			// first erase the destination range
			lhs.v->erase( lhs.v->begin() + start, lhs.v->begin() + end );
			// if the rhs is a vector, insert its contents
			if( rhs.type == obj_vector )
			{
				lhs.v->insert( lhs.v->begin() + start, rhs.v->begin(), rhs.v->end() );
				// IncRef each of the items being inserted
				for( Vector::iterator i = rhs.v->begin(); i != rhs.v->end(); ++i )
				{
					IncRef( *i );
				}
			}
			// otherwise insert whatever the object is
			else
			{
				lhs.v->insert( lhs.v->begin() + start, rhs );
				IncRef( rhs );
			}
		}
		DecRef( rhs );
		DecRef( lhs );
		}
		break;
	case op_storeslice3:
		{
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		Object idx3 = stack.back();
		idx3 = ResolveSymbol( idx3 );
		stack.pop_back();
		Object idx2 = stack.back();
		idx2 = ResolveSymbol( idx2 );
		stack.pop_back();
		Object idx1 = stack.back();
		idx1 = ResolveSymbol( idx1 );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		stack.pop_back();

		if( !IsVecType( lhs.type ) )
			throw RuntimeException( boost::format( "Invalid object for slice assignment: '%1%' is not a vector." ) % lhs );
		if( !is_integral( idx1.type ) && idx1.type != obj_null )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );
		if( !is_integral( idx2.type ) && idx2.type != obj_null )
			throw RuntimeException( "'start' index in slice must be an integral number or '$'." );

		size_t sz = lhs.v->size();

		int start = idx1.type == obj_null ? sz : (int)idx1.d;
		int end = idx2.type == obj_null ? sz : (int)idx2.d;
		int step = (int)idx3.d;

		// handle negative values
		if( start < 0 )
			start = sz + start;
		if( end < 0 )
			end = sz + end;

		// check the indices & step value and convert to sane values, if
		// necessary
		if( start > sz )
			start = sz;
		if( start < 0 )
			start = 0;
		if( end > sz )
			end = sz;
		if( end < 0 )
			end = 0;
		if( end < start )
			end = start;
		if( step < 1 )
			throw RuntimeException( "Invalid 'step' argument in slice: 'step' is less than one." );

		if( step == 1 )
		{
			// first erase the destination range
			lhs.v->erase( lhs.v->begin() + start, lhs.v->begin() + end );
			// if the rhs is a vector, insert its contents
			if( rhs.type == obj_vector )
			{
				lhs.v->insert( lhs.v->begin() + start, rhs.v->begin(), rhs.v->end() );
				// IncRef each of the items being inserted
				for( Vector::iterator i = rhs.v->begin(); i != rhs.v->end(); ++i )
				{
					IncRef( *i );
				}
			}
			// otherwise insert whatever the object is
			else
			{
				lhs.v->insert( lhs.v->begin() + start, rhs );
				IncRef( rhs );
			}
		}
		// other steps are separate deletions and insertions
		else
		{
			if( !IsVecType( rhs.type ) )
				throw RuntimeException( "Source in slice assignment must be a vector." );
			// ensure the destination and source lengths are identical
			size_t lhs_sz = (size_t)ceil(((double)end - (double)start)/(double)step);
			size_t rhs_sz = rhs.v->size();
			if( lhs_sz != rhs_sz )
				throw RuntimeException( "Source and destination in slice assignment must be the same size." );

			int j = 0;
			for( int i = 0; i < end - start; i++ )
			{
				if( i % step == 0 )
				{
					lhs.v->at( start + i ) = rhs.v->at( j );
					IncRef( rhs.v->at( j ) );
					j++;
				}
			}
		}
		DecRef( rhs );
		DecRef( lhs );
		}
		break;
	case op_add_tbl_store:	// tos2[tos1] += tos
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = ResolveSymbol( lhs );
		lhs = stack.back();
		DecRef( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// vector:
		if( IsVecType( lhs.type ) )
		{
			if( rhs.type != obj_number )
				throw RuntimeException( "Vectors can only be indexed with numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
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
		o = ResolveSymbol( o );
		stack.pop_back();
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// vector:
		if( IsVecType( lhs.type ) )
		{
			if( rhs.type != obj_number )
				throw RuntimeException( "Vectors can only be indexed with numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
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
		o = ResolveSymbol( o );
		stack.pop_back();
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// vector:
		if( IsVecType( lhs.type ) )
		{
			if( rhs.type != obj_number )
				throw RuntimeException( "Vectors can only be indexed with numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
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
		o = ResolveSymbol( o );
		stack.pop_back();
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// vector:
		if( IsVecType( lhs.type ) )
		{
			if( rhs.type != obj_number )
				throw RuntimeException( "Vectors can only be indexed with numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
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
		o = ResolveSymbol( o );
		stack.pop_back();
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		DecRef( rhs );
		stack.pop_back();
		lhs = stack.back();
		lhs = ResolveSymbol( lhs );
		DecRef( lhs );
		stack.pop_back();
		if( !IsRefType( lhs.type ) )
			throw RuntimeException( boost::format( "'%1%' is not a vector or map." ) % lhs );
		// vector:
		if( IsVecType( lhs.type ) )
		{
			if( rhs.type != obj_number )
				throw RuntimeException( "Vectors can only be indexed with numeric values." );
			// error if arguments aren't integral numbers...
			if( !is_integral( rhs.d ) )
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
			if( !is_integral( o.d ) || !is_integral( lhsob.d ) )
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
			if( !is_integral( o.d ) || !is_integral( lhsob.d ) )
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
		{
			stack.push_back( stack.back() );
			IncRef( stack.back() );
		}
		break;
	case op_dup1:
		stack.push_back( stack.back() );
		IncRef( stack.back() );
		break;
	case op_dup2:
		stack.push_back( stack.back() );
		IncRef( stack.back() );
		stack.push_back( stack.back() );
		IncRef( stack.back() );
		break;
	case op_dup3:
		stack.push_back( stack.back() );
		IncRef( stack.back() );
		stack.push_back( stack.back() );
		IncRef( stack.back() );
		stack.push_back( stack.back() );
		IncRef( stack.back() );
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
	case op_swap:	// tos = tos1; tos1 = tos
		// verify the stack
		{
		int stack_size = stack.size();
		if( stack_size < 2 )
			throw ICE( "Stack error: not enough elements on the stack for 'swap' instruction." );
		Object tmp = stack[stack_size-1];
		stack[stack_size-1] = stack[stack_size-2];
		stack[stack_size-2] = tmp;
		}
		break;
	case op_rot:
		// 1 arg: integer number for how far to rotate
		arg = *((dword*)ip);
		ip += sizeof( dword );
		// verify the stack is deep enough
		if( stack.size() < (int)arg+1 )
			throw ICE( "Stack error: not enough elements on the stack for 'rot' instruction." );
		// pop the tos
		o = stack.back();
		stack.pop_back();
		// insert it into place
		stack.insert( stack.end() - (int)arg, o );
		break;
	case op_rot2:
		// verify the stack is deep enough
		if( stack.size() < 3 )
			throw ICE( "Stack error: not enough elements on the stack for 'rot2' instruction." );
		// pop the tos
		o = stack.back();
		stack.pop_back();
		// insert it into the second-to-last place
		stack.insert( stack.end() - 2, o );
		break;
	case op_rot3:
		// verify the stack is deep enough
		if( stack.size() < 4 )
			throw ICE( "Stack error: not enough elements on the stack for 'rot3' instruction." );
		// pop the tos
		o = stack.back();
		stack.pop_back();
		// insert it into place
		stack.insert( stack.end() - 3, o );
		break;
	case op_rot4:
		// verify the stack is deep enough
		if( stack.size() < 5 )
			throw ICE( "Stack error: not enough elements on the stack for 'rot4' instruction." );
		// pop the tos
		o = stack.back();
		stack.pop_back();
		// insert it into place
		stack.insert( stack.end() - 4, o );
		break;
	case op_import:
		// 1 arg:
		arg = *((dword*)ip);
		ip += sizeof( dword );
		// look-up the constant
		o = GetConstant( arg );
		if( o.type != obj_symbol_name )
			throw ICE( "Invalid argument to 'import' instruction: not a symbol name." );
		ImportModule( o.s );
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

void Executor::ExecuteFunction( Function* f, int num_args, bool method_call_op, bool is_destructor /*= false*/ )
{
	// if this is a method there's an extra arg for 'this'
	int args_passed = num_args;
	if( f->IsMethod() )
	{
		// if this was called via op_call_method, 'self' was passed implicitly
		// and should not be included in the num_args counter
		if( method_call_op )
			num_args++;
		// (if this was *not* called via op_call_method, but *is* a method, then
		// num_args must contain 'self' explicitly, do *not* increment
		else
			args_passed--;
	}

	if( num_args > f->num_args )
		throw RuntimeException( boost::format( "Too many arguments passed to function '%1%'." ) % f->name );
	if( (f->num_args - num_args) > f->NumDefaultArgs() )
		throw RuntimeException( boost::format( "Not enough arguments passed to function '%1%'." ) % f->name );

	// create a frame for the fcn
	Frame* frame = new Frame( CurrentFrame(), scopes, ip, ip - sizeof(dword) - 1, num_args, f );
	Scope* scope = new Scope();

	// set the args for the frame

	// for op_call_method, 'self' is on top of stack
	if( method_call_op && f->IsMethod() )
	{
		// set local 0 to 'self'
		Object ob = stack.back();
		// ensure it is a class or an instance!
		if( ob.type != obj_class && ob.type != obj_instance )
			throw RuntimeException( boost::format( "Class or instance expected as 'self' argument to method, found '%1%'." ) % ob );
		frame->SetLocal( 0, ob );
		stack.pop_back();
	}
	for( int i = 0; i < args_passed; i++ )
	{
		Object ob = stack.back();
		frame->SetLocal( num_args-i-1, ob );
		stack.pop_back();
	}
	// but for op_call, 'self' is the last arg on the stack
	if( !method_call_op && f->IsMethod() )
	{
		// set local 0 to 'self'
		Object ob = stack.back();
		if( ob.type != obj_class && ob.type != obj_instance )
			throw RuntimeException( boost::format( "Class or instance expected as 'self' argument to method, found '%1%'." ) % ob );
		frame->SetLocal( 0, ob );
		stack.pop_back();
	}

	// default arg vals...
	int num_defaults = f->num_args - num_args;
	if( num_defaults != 0 )
	{
		int non_defaults = f->num_args - f->default_args.Size();
		for( int i = 0; i < num_defaults; i++ )
		{
			int idx = f->default_args.At( num_args + i - non_defaults );
			Object o = GetConstant( idx );
			frame->SetLocal( num_args+i, o );
		}
	}
	// push the frame onto the callstack
	PushFrame( frame );
	PushScope( scope );
	// jump to the function
	ip = (byte*)(bp + f->addr);
	int stack_size = stack.size();
	if( stack_size < 0 )
		throw ICE( "Stack underflow." );
	// clear the error state/object
	if( is_error )
		DecRef( error );
	is_error = false;
	// execute until it returns
	ExecuteToReturn( is_destructor );
	if( stack.size() != stack_size + 1 )
		throw ICE( boost::format( "Function '%1%' corrupted the stack." ) % f->name );
}

void Executor::ExecuteFunction( NativeFunction nf, int num_args, bool method_call_op )
{
	// if this is a method there's an extra arg for 'this'
	int args_passed = num_args;
	if( nf.is_method )
	{
		// if this was called via op_call_method, 'self' was passed implicitly
		// and should not be included in the num_args counter
		if( method_call_op )
			num_args++;
		// (if this was *not* called via op_call_method, but *is* a method, then
		// num_args must contain 'self' explicitly, do *not* increment
		else
			args_passed--;
	}

	// create a frame for the fcn
	Frame* frame = new Frame( CurrentFrame(), scopes, ip, ip - sizeof(dword) - 1, num_args, nf );
	Scope* scope = new Scope();
	// set the args for the frame

	// for op_call_method, 'self' is on top of stack
	if( method_call_op && nf.is_method )
	{
		// set local 0 to 'self'
		Object ob = stack.back();
		frame->SetLocal( 0, ob );
		stack.pop_back();
	}
	for( int i = 0; i < args_passed; i++ )
	{
		Object ob = stack.back();
		frame->SetLocal( num_args-i-1, ob );
		stack.pop_back();
	}
	// but for op_call, 'self' is the last arg on the stack
	if( !method_call_op && nf.is_method )
	{
		// set local 0 to 'self'
		Object ob = stack.back();
		frame->SetLocal( 0, ob );
		stack.pop_back();
	}

	// push the frame onto the callstack
	PushFrame( frame );
	PushScope( scope );
	// save the stack depth & check for underflow
	int stack_size = stack.size();
	if( stack_size < 0 )
		throw ICE( "Stack underflow." );
	// clear the error state/object
	if( is_error && nf.p != do_error && nf.p != do_seterror && nf.p != do_geterror )
	{
		is_error = false;
		DecRef( error );
	}
	// execute the function
	nf.p( frame );
	// check the stack
	if( stack.size() != stack_size + 1 )
		throw ICE( "Native function corrupted the stack." );
	PopScope();
	PopFrame();
}

void Executor::SetError( Object* err )
{
	if( is_error )
		DecRef( error );
	error = *err;
	is_error = true;
	IncRef( error );
}

bool Executor::Error()
{
	return is_error;
}

Object Executor::GetError()
{
	if( is_error )
		return error;
	else
		return Object( obj_null );
}

// helper function to locate a namespace by (module) name
// helper (functor) class
class equal_to_first
{
	string value;
public:
	equal_to_first( string val ) : value( val ) {}
	bool operator()(pair<string, void*> n ) { return n.first == value; }
};
// find a namespace
vector< pair<string, void*> >::iterator Executor::find_namespace( string mod )
{
	return find_if( namespaces.begin(), namespaces.end(), equal_to_first( mod ) );
}

// helper function for ImportModule
string Executor::find_module( string mod )
{
	// split the path given into it's / separated parts
	vector<string> path = split_path( mod );

	// first, look in the directory of the currently executing file
	string cf( current_file );
	string cur_file = get_file_part( cf );
	// get the cwd
	string curdir = get_cwd();
	// append the current file to the current directory (in case the current
	// file path contains directories, e.g. the compiler was called on the
	// file 'src/foo.dv')
	curdir += "/";
	curdir += cur_file;
	curdir = get_dir_part( curdir );
	// 'curdir' now contains the directory of the currently executing file
	// append the mod name to get the mod path
	string modpath( curdir );
	for( vector<string>::iterator it = path.begin(); it != path.end(); ++it )
	{
		modpath = join_paths( modpath, *it );
	}
	// check for .dv/.dvc files on disk
	struct stat statbuf;
	// if we can open the module file, return it
	string dv = modpath + ".dv";
	if( stat( dv.c_str(), &statbuf ) != -1 )
		return modpath;
	string dvc = modpath + ".dvc";
	if( stat( dvc.c_str(), &statbuf ) != -1 )
		return modpath;
	// otherwise check the paths in the DEVA env var
	else
	{
		// get the DEVA env var
		string devapath( getenv( "DEVA" ) );
		// split it into separate paths (on the ":" char in un*x)
		vector<string> paths;
		split_env_var_paths( devapath, paths );
		// for each of the paths, append the mod
		// and see if it exists
		for( vector<string>::iterator it = paths.begin(); it != paths.end(); ++it )
		{
			modpath = join_paths( *it, mod );
			// check for .dv/.dvc files on disk
			struct stat statbuf;
			// if we can't open the module file, error out
			string dv = modpath + ".dv";
			if( stat( dv.c_str(), &statbuf ) != -1 )
				return modpath;
			string dvc = modpath + ".dvc";
			if( stat( dvc.c_str(), &statbuf ) != -1 )
				return modpath;
		}
	}
	// not found, error
	throw RuntimeException( boost::format( "Unable to locate module '%1%' for import." ) % mod );
}

// helper fcn for parsing and compiling a module
const Code* const Executor::LoadModule( string fname )
{
	ParseReturnValue prv;
	PassOneReturnValue p1rv;

	// NOTE: compiler and semantics global objects should be free'd and NULL at
	// this point!

	// parse the file
	prv = Parse( fname.c_str() );

	if( prv.successful )
	{
		PassOneFlags p1f; // currently no pass one flags
		PassTwoFlags p2f;
		p2f.trace = trace;

		// PASS ONE: build the symbol table and check semantics
		p1rv = PassOne( prv, p1f );

		// PASS TWO: compile
		PassTwo( p1rv, p2f );
		Code* c = new Code( (byte*)compiler->is->Bytes(), compiler->is->Length() );

		// free parser, compiler memory
		FreeParseReturnValue( prv );
		FreePassOneReturnValue( p1rv );
		// free the semantics and compiler objects (symbol table et al) 
		delete compiler;
		delete semantics;

		// return the code
		return c;
	}
	else
		throw RuntimeException( boost::format( "Unable to load module '%1%'." ) % fname );
}

bool Executor::ImportModule( const char* module_name )
{
	// TODO: implement!
	//throw ICE( "ImportModule() not yet implemented!" );
	
	// check the list of builtin modules first
//	if( ImportBuiltinModule( mod ) )
//		return true;

	// prevent importing the same module more than once
	vector< pair<string, void*> >::iterator it;
	it = find_namespace( module_name );
	// if we found the namespace
	if( it != namespaces.end() )
		return true;

	// otherwise look for the .dv/.dvc file to import
	string path = find_module( module_name );

	// for now, just run the file by short name with ".dvc" extension (i.e. in
	// the current working directory)
	string dvfile( path + ".dv" );
	string dvcfile( path + ".dvc" );
	// save the ip, bp and end ptrs
	byte* orig_ip = ip;
	byte* orig_bp = bp;
	byte* orig_end = end;
	// save the current scope table
//	ScopeTable* orig_scopes = current_scopes;
	// create a new namespace and set it at the current scope
	// TODO: currently this only adds the "short" name of the module as a
	// namespace. should the full path be used somehow?? foo::bar? foo.bar?
	// foo-bar? foo/bar?
	string mod( module_name );
	mod = get_file_part( mod );
//	ScopeTable* st = new ScopeTable( this );
//	namespaces.push_back( pair<string, ScopeTable*>(mod, st) );
	namespaces.push_back( pair<string, void*>(mod, NULL) );
//	current_scopes = st;
//	// create a 'file/module' level scope for the namespace
//	current_scopes->Push();
//	// compile the file, if needed
//	CompileAndWriteFile( dvfile.c_str(), mod.c_str() );
//	// and then run the file
//	RunFile( dvcfile.c_str() );

	// load (compile) the module
	const Code* code = LoadModule( dvfile );
	if( !code )
		throw RuntimeException( boost::format( "Unable to load module '%1%'." ) % mod );
	// execute it
	ExecuteCode( code );
	
	// restore the ip, bp and end ptrs
	ip = orig_ip;
	bp = orig_bp;
	end = orig_end;

	// restore the current scope table
//	current_scopes = orig_scopes;

	return true;
}

// disassemble the instruction stream to stdout
void Executor::Decode( const Code* code)
{
	byte* b = code->code;
	byte* p = b;
	byte* end = code->code + code->len;
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
		arg = *((dword*)p);
		cout << "\t" << arg;
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
		arg = *((dword*)p);
		cout << "\t" << arg;
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
		arg = *((dword*)p);
		cout << "\t" << arg;
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
	case op_def_function:
		// 2 args: constant index of name, fcn address
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant( arg );
		cout << arg << " (" << o << ")";
		// address
		arg = *((dword*)( p + sizeof( dword ) ) );
		cout << ", " << arg;
		ret = sizeof( dword ) * 2;
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
	case op_call_method:
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
		arg2 = *((dword*)(p + sizeof( dword ) ) );
		ret = sizeof( dword ) * 2;
		cout << "\t" << arg << "\t" << arg2;
		break;
	case op_enter:
	case op_leave:
		cout << "\t" << " ";
		break;
	case op_for_iter:
	case op_for_iter_pair:
		// 1 arg: iterable object
		arg = *((dword*)p);
		cout << "\t" << arg;
		ret = sizeof( dword );
		break;
	case op_tbl_load:
	case op_method_load:
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
		break;
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

void Executor::DumpStackTop()
{
	// print the top five stack items
	cout << "\t\t[";
	int n = (stack.size() > 5 ? 5 : stack.size());
	for( int i = n-1; i >= 0; i-- )
	{
		cout << stack[stack.size()-(n-i)];
		if( i > 0 )
			cout << ", ";
	}
	cout << "]";
	if( stack.size() > 5 )
		cout << " +" << stack.size()-5 << " more";
	cout << endl;
}

void Executor::DumpTrace( ostream & os )
{
	// nothing to do?
	if( callstack.begin() == callstack.end() - 1 )
		return;

	os << "Traceback (most recent first):" << endl;
	string fcn, file;
	int line = -1;
	int depth = 1;
	Frame* f;
	for(vector<Frame*>::reverse_iterator i = callstack.rbegin(); i < callstack.rend() - 1; ++i )
	{
		f = *i;
		if( f->IsNative() )
		{
			ostringstream s;
			s << hex << f->GetNativeFunction().p;
			fcn = "[Native Function at " + s.str() + "]";

			file = "[Native Module]";
		}
		else
		{
			fcn = f->GetFunction()->name;
			file = f->GetFunction()->filename;
		}

		// pad for callstack depth
		for( int c = 0; c < depth; c++ )
			os << " ";

		// TODO: if there is debug info, include the line number, else don't
		// TODO: call site: if there is debug info, map the call-site
		// (frame->addr) to a line number. otherwise, subtract the bp (for this
		// module) from it to produce the code offset
		if( depth == 1 )
			os << "file: " << file << ", line: " << line << ", in " << fcn << endl;
		else
		{
			size_t call_site = GetOffsetForCallSite( f->GetCallSite() );
			os << "file: " << file << ", at: " << call_site << ", call to " << fcn << endl;
//		os << "  file: " << i->file << ", line: " << i->call_site << ", call to " << i->function << endl;
		}

		depth++;
	}
}

} // namespace deva
