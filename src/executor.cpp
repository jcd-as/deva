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
#include "fileformat.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <sstream>

using namespace std;


namespace deva
{


// symbols that always exist
const Object constant_symbols[] = 
{
	Object( true ),
	Object( false ),
	Object( obj_null ),
	Object( obj_symbol_name, copystr( "" ) ),			// empty string for 'main' module name
	Object( obj_symbol_name, copystr( "__name__" ) ),
	Object( obj_symbol_name, copystr( "__class__" ) ),
	Object( obj_symbol_name, copystr( "__bases__" ) ),
	Object( obj_symbol_name, copystr( "__module__" ) ),
	Object( obj_symbol_name, copystr( "new" ) ),
	Object( obj_symbol_name, copystr( "delete" ) ),
	Object( obj_symbol_name, copystr( "self" ) ),
	Object( obj_symbol_name, copystr( "rewind" ) ),
	Object( obj_symbol_name, copystr( "next" ) )
};
const int num_of_constant_symbols = sizeof( constant_symbols ) / sizeof( Object );

/////////////////////////////////////////////////////////////////////////////
// executive/VM functions and globals
/////////////////////////////////////////////////////////////////////////////

// global executor object
Executor* ex = NULL;
// static member initialization
// singleton support:
bool Executor::instantiated = false;

Executor::Executor() : 
	cur_code( NULL ),
	ip( NULL ), 
	bp( NULL ), 
	end( NULL ), 
	scopes( NULL ),
	is_error( false ),
	debug( false ), 
	trace( false )
{
	if( instantiated )
		throw ICE( "Executor is a singleton object, it cannot be instantiated twice." );
	else
		instantiated = true;

	// set-up the constant pool
	constants.reserve( 256 );

	// add names that always exist
	for( int i = 0; i < num_of_constant_symbols; i++ )
	{
		AddGlobalConstant( constant_symbols[i] );
	}
	// add the builtins, string builtins, vector builtins and map builtins to the constant pool
	// builtins
	for( int i = 0; i < num_of_builtins; i++ )
	{
		char* s = copystr( builtin_names[i].c_str() );
		if( !AddGlobalConstant( Object( obj_symbol_name, s ) ) )
			delete [] s;
	}
	// string builtins
	for( int i = 0; i < num_of_string_builtins; i++ )
	{
		char* s = copystr( string_builtin_names[i].c_str() );
		if( !AddGlobalConstant( Object( obj_symbol_name, s ) ) )
			delete [] s;
	}
	// vector builtins
	for( int i = 0; i < num_of_vector_builtins; i++ )
	{
		char* s = copystr( vector_builtin_names[i].c_str() );
		if( !AddGlobalConstant( Object( obj_symbol_name, s ) ) )
			delete [] s;
	}
	// map builtins
	for( int i = 0; i < num_of_map_builtins; i++ )
	{
		char* s = copystr( map_builtin_names[i].c_str() );
		if( !AddGlobalConstant( Object( obj_symbol_name, s ) ) )
			delete [] s;
	}
}

// WARNING! do not delete anything here which might DecRef Objects and thus execute deva code!
// all code must be finished executing and all Objects gone by the time Execute() completes!
Executor::~Executor()
{
	// free the function objects
	for( multimap<string, Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		string s = i->first;
		if( i->second->type == obj_function )
			delete i->second->f;
		delete i->second;
	}
	// free the constants' string data
	for( size_t i = 0; i < constants.size(); i++ )
	{
		ObjectType type = constants.at( i ).type;
		if( type == obj_string || type == obj_symbol_name ) delete [] constants.at( i ).s;
	}
	// free the code blocks
	for( vector<const Code*>::iterator i = code_blocks.begin(); i != code_blocks.end(); ++i )
	{
		delete *i;
	}
}

Object* Executor::FindFunction( string name, string modulename, size_t offset )
{
	multimap<string, Object*>::iterator i = functions.find( name );
	for( ; i != functions.end(); ++i )
	{
		if( i->second->type == obj_function )
		{
//			if( i->second->f->modulename == modulename && (size_t)i->second->f->addr == offset )
			string m = i->second->f->modulename;
			size_t a = (size_t)i->second->f->addr;
			if(  m == modulename &&  a == offset )
				return i->second;
		}
		else if( i->second->type == obj_native_function )
		{
			// TODO: any check we can do for native fcns module name?
			if( (size_t)i->second->nf.p == offset )
				return i->second;
		}
		else
			throw ICE( "Non-function found in Executor's function table." );
	}
	return NULL;
}

// locate a symbol, possibly in a given module
Object* Executor::FindSymbol( const char* name, Module* mod /*= NULL*/ )
{
	if( mod )
	{
		Object* o = mod->scope->FindSymbol( name );
		return o;
	}
	else
		return scopes->FindSymbol( name );
}

// return a resolved symbol - if sym is a symbol name, find the symbol,
// otherwise return sym unmodified 
Object Executor::ResolveSymbol( Object sym )
{
	if( sym.type != obj_symbol_name )
		return sym;

	// TODO: this function is virtually the same as Frame::FindSymbol.
	// consolidate them???

	Object* obj = scopes->FindSymbol( sym.s, true );

	if( !obj )
	{
		// try functions
		obj = scopes->FindFunction( sym.s );
		if( obj )
			return *obj;

		// module?
		map<string, Module*>::iterator i = modules.find( string( sym.s ) );
		if( i != modules.end() )
			return Object( i->second );

		// allow module names to pass through
		// (for native modules)
		if( module_names.count( sym.s ) != 0 )
			return sym;

		// builtin?
		NativeFunction nf = GetBuiltin( sym.s );
		if( nf.p )
			return Object( nf );

		// try extern symbol
		obj = scopes->FindExternSymbol( sym.s );
		if( obj )
			return *obj;
		
		// type built-ins will conflict with each other,
		// just put the symbol name back on the stack
		//
		// string builtin?
		nf = GetStringBuiltin( sym.s );
		if( nf.p )
			return sym;
		// vector builtin?
		nf = GetVectorBuiltin( sym.s );
		if( nf.p )
			return sym;
		// map builtin?
		nf = GetMapBuiltin( sym.s );
		if( nf.p )
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

	scopes = new ScopeTable();

	// a starting ('global') frame
//	Object *main = FindFunction( string( "@main" ), string( "" ), 0 );
	string cf = current_file;
	string fp = get_file_part( cf );
	string modname = get_stem( fp );
	Object *main = FindFunction( string( "@main" ), modname, 0 );
	Frame* frame = new Frame( NULL, scopes, code->code, code->code, 0, main->f );
	PushFrame( frame );

	// make sure the global scope is always around
	PushScope( new Scope( frame ) );

	if( trace )
		cout << "Execution trace:" << endl;

	// execute until end
	ExecuteCode( code );

	// pop the 'global' scope
	PopScope();

	// free the module objects
	for( vector<Module*>::reverse_iterator i = module_stack.rbegin(); i != module_stack.rend(); ++i )
	{
		(*i)->DeleteScopeData();
		(*i)->DeleteScope();
	}
	for( vector<Module*>::reverse_iterator i = module_stack.rbegin(); i != module_stack.rend(); ++i )
	{
		(*i)->DeleteFrame();
		delete *i;
	}

	// free the scope table
	delete scopes;
	PopFrame();
}

void Executor::ExecuteCode( const Code* const code )
{
	// add this code block to our collection
	code_blocks.push_back( code );

	Opcode op = op_nop;

	// set the ip, bp and end 
	cur_code = (Code*)code;
	end = code->code + code->len;
	bp = code->code;
	ip = bp;

	// execute until end
	while( ip < end && op < op_halt )
	{
		op = ExecuteInstruction();
	}
}

// static used for importing modules and text (via eval())
static Module* s_currently_importing_module = NULL;

Object Executor::ExecuteText( const char* const text )
{
	static dword count = 0;
	ostringstream s;
	s << "[TEXT" << count << "]";
	count++;
	string name = s.str();
	// add the 'text module' name to the list of global constants 
	AddGlobalConstant( Object( obj_symbol_name, copystr( name ) ) );
	const Code* code = LoadText( text, name.c_str() );

	Code* orig_code = cur_code;
	byte* orig_ip = ip;
	byte* orig_bp = bp;
	byte* orig_end = end;

	// find our 'module' function, "name@main"
	Object *eval_main = FindFunction( "@main", name, 0 );
	Frame* frame = new Frame( NULL, scopes, code->code, code->code, 0, eval_main->f, true );
	PushFrame( frame );
	Scope* scope = new Scope( frame, false, true );
	PushScope( scope );

	Module* cur_module = AddModule( name.c_str(), code, scope, frame );

	// currently importing text module, set the flag
	Module* prev_mod = s_currently_importing_module;
	s_currently_importing_module = cur_module;

	// execute
	ExecuteCode( code );
	
	// no longer importing this module, reset the flag
	s_currently_importing_module = prev_mod;

	// add the module to load-ordered stack (for deletion in depth first order)
	module_stack.push_back( cur_module );

	// pop the scope and frame
	PopScope();
	PopFrame();

	cur_code = orig_code;
	ip = orig_ip;
	bp = orig_bp;
	end = orig_end;

	return Object( cur_module );
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
	// TODO: push0, 1, 2, 3 ops are pretty useless. remove them?
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
			tmp = ResolveSymbol( tmp );
			IncRef( tmp );
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
		plhs = FindSymbol( o.s );
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
		plhs = FindSymbol( o.s );
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
		plhs = FindSymbol( o.s );
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
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( arg ), arg );
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
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 0 ), 0 );
		break;
	case op_def_local1:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 1, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 1 ), 1 );
		break;
	case op_def_local2:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 2, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 2 ), 2 );
		break;
	case op_def_local3:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 3, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 3 ), 3 );
		break;
	case op_def_local4:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 4, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 4 ), 4 );
		break;
	case op_def_local5:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 5, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 5 ), 5 );
		break;
	case op_def_local6:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 6, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 6 ), 6 );
		break;
	case op_def_local7:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 7, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 7 ), 7 );
		break;
	case op_def_local8:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 8, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 8 ), 8 );
		break;
	case op_def_local9:
		rhs = stack.back();
		rhs = ResolveSymbol( rhs );
		stack.pop_back();
		// set the local in the current frame
		CurrentFrame()->SetLocal( 9, rhs );
		// define the local in the current scope
		// (this frame cannot be native fcn, obviously)
		CurrentScope()->AddSymbol( CurrentFrame()->GetFunction()->local_names.operator[]( 9 ), 9 );
		break;
	case op_def_function:
		{
		// 3 args: constant index of fcn name, const idx of module name, address of fcn
		arg2 = *((dword*)ip);
		ip += sizeof( dword );
		arg = *((dword*)ip);
		ip += sizeof( dword );
		arg3 = *((dword*)ip);
		ip += sizeof( dword );
		// find the constant for the fcn name
		Object fcnname = GetConstant( arg2 );
		if( fcnname.type != obj_string && fcnname.type != obj_symbol_name )
			throw ICE( "def_function instruction called with an object that is not a function name." );
		string name( fcnname.s );
		// find the constant for the module name
		Object modname = GetConstant( arg );
		if( modname.type != obj_string && modname.type != obj_symbol_name )
			throw ICE( "def_function instruction called with an object that is not a module name." );
		string module_name( modname.s );
		// find the function
		Object* objf = FindFunction( name, module_name, (size_t)arg3 );
		if( !objf )
			throw RuntimeException( boost::format( "Function '%1%' not found." ) % name );
		// if we're currently importing a module, set the functions module ptr
		if( s_currently_importing_module )
			objf->f->module = s_currently_importing_module;
		// is there a local of this name that we need to override?
		Object* local_obj = CurrentScope()->FindSymbol( fcnname.s );
		if( local_obj )
		{
			// get the index of the local
			int local_idx = CurrentScope()->FindSymbolIndex( local_obj, CurrentFrame() );
			// set the local in the current frame
			if( local_idx != -1 )
				CurrentFrame()->SetLocal( local_idx, *objf );
		}
		// add the function to the local scope
		CurrentScope()->AddFunction( name, objf );
		}
		break;
	case op_def_method:
		{
		// 4 args: constant index of fcn name, const index of class name, const idx of module name, address of fcn
		arg = *((dword*)ip);
		ip += sizeof( dword );
		arg2 = *((dword*)ip);
		ip += sizeof( dword );
		dword arg4 = *((dword*)ip);
		ip += sizeof( dword );
		arg3 = *((dword*)ip);
		ip += sizeof( dword );

		// if we're currently importing a module, set the functions module ptr
		if( s_currently_importing_module )
		{
			// find the function name
			Object fcnname = GetConstant( arg );
			if( fcnname.type != obj_string && fcnname.type != obj_symbol_name )
				throw ICE( "def_method instruction called with an object that is not a class name." );
			string function_name( fcnname.s );
			// find the class name
			Object clsname = GetConstant( arg2 );
			if( clsname.type != obj_string && clsname.type != obj_symbol_name )
				throw ICE( "def_method instruction called with an object that is not a class name." );
			string class_name( clsname.s );
//			function_name += "@";
//			function_name += class_name;

			// find the constant for the module name
			Object modname = GetConstant( arg4 );
			if( modname.type != obj_string && modname.type != obj_symbol_name )
				throw ICE( "def_function instruction called with an object that is not a module name." );
			string module_name( modname.s );

			// find the function
			Object* objf = FindFunction( function_name, module_name, (size_t)arg3 );
			if( !objf )
				throw RuntimeException( boost::format( "Function '%1%' not found." ) % function_name );

			objf->f->module = s_currently_importing_module;
		}
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
		for( dword i = 0; i < arg; i++ )
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
		for( dword i = 0; i < arg; i++ )
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
		Object _name = GetConstant( Object( obj_symbol_name, "__name__" ) );
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
		Object _bases = GetConstant( Object( obj_symbol_name, "__bases__" ) );
		m.m->insert( pair<Object, Object>( _bases, basesObj ) );

		// add all of the methods for this class
		for( multimap<string,Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
		{
			Function* f = i->second->f;
			if( f->classname == name.s )
			{
				int i = FindConstant( Object( obj_symbol_name, f->name.c_str() ) );
				if( i == INT_MIN )
					throw ICE( boost::format( "Cannot find function name '%1%' for class construction." ) % f->name.c_str() );
				Object fcnname = GetConstant( i );
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
	case op_jmpt:
		// 1 arg: size
		arg = *((dword*)ip);
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();
		if( o.CoerceToBool() )
			ip = (byte*)(bp + arg);
		else
			ip += sizeof( dword );
		DecRef( o );
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
		DecRef( o );
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
		{
			stack.push_back( Object( false ) );
			break;
		}
		switch( lhs.type )
		{
		case obj_null: stack.push_back( Object( rhs.type == obj_null ) ); break;
		case obj_boolean: stack.push_back( Object( lhs.b == rhs.b ) ); break;
		case obj_number: stack.push_back( Object( lhs.d == rhs.d ) ); break;
		case obj_symbol_name:
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
		case obj_module: stack.push_back( Object( lhs.mod == rhs.mod ) ); break;
		case obj_end: throw ICE( "Invalid object in op_eq." ); break;
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
		switch( lhs.type )
		{
		case obj_null: stack.push_back( Object( rhs.type != obj_null ) ); break;
		case obj_boolean: stack.push_back( Object( lhs.b != rhs.b ) ); break;
		case obj_number: stack.push_back( Object( lhs.d != rhs.d ) ); break;
		case obj_symbol_name:
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
		case obj_module: stack.push_back( Object( lhs.mod != rhs.mod ) ); break;
		case obj_end: throw ICE( "Invalid object in op_neq." ); break;
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
		plhs = FindSymbol( o.s );
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
		plhs = FindSymbol( o.s );
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
		plhs = FindSymbol( o.s );
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
		plhs = FindSymbol( o.s );
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
		plhs = FindSymbol( o.s );
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
	case op_inc:
		{
		// get the tos
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();
		// has to be numeric
		if( o.type != obj_number )
			throw RuntimeException( "Operand to increment operator must be numeric." );
		Object o2( o.d + 1 );
		stack.push_back( o2 );
		}
		break;
	case op_dec:
		{
		// get the tos
		o = stack.back();
		o = ResolveSymbol( o );
		stack.pop_back();
		// has to be numeric
		if( o.type != obj_number )
			throw RuntimeException( "Operand to decrement operator must be numeric." );
		Object o2( o.d - 1 );
		stack.push_back( o2 );
		}
		break;
	case op_call: // call function with <Op0> args on on stack, fcn after args
	case op_call_method: // call function with <Op0> args on on stack, fcn after args
		{
		// 1 arg: number of args passed
		arg = *((dword*)ip);
		ip += sizeof( dword );
		// get the fcn
		o = stack.back();
		o = ResolveSymbol( o );
		DecRef( o );
		stack.pop_back();
		Object callable;
		// is it a symbol name that we need to look-up?
		if( o.type == obj_symbol_name )
		{
			Object* callablePtr = FindSymbol( o.s );
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
				if( arg >= callable.f->num_args - callable.f->default_args.size() )
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
			Object _class = GetConstant( Object( obj_symbol_name, "__class__" ) );
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
		// if a string is being returned (including inside a vector or map/class/instance),
		// it needs to be copied to the calling frame! 
		// (string mem in a frame is freed on frame exit)
		{
		Object o = stack.back();
		stack.pop_back();
		Object obj = CurrentFrame()->CopyStringsFromParent( o );
		stack.push_back( obj );

		// 1 arg: number of scopes to leave
		arg = *((dword*)ip);
		// leave the scopes
		for( dword i = 0; i < arg; i++ )
			PopScope();
		// jump to the new frame's address
		ip = (byte*)CurrentFrame()->GetReturnAddress();
		// pop the argument scope...
		PopScope();
		// ... and the current frame
		PopFrame();
		}
		break;
	case op_exit_loop:
		// 2 args: jump target address, number of scopes to leave
		arg = *((dword*)ip);
		ip += sizeof( dword );
		arg2 = *((dword*)ip);
		ip += sizeof( dword );
		// leave the scopes
		for( dword i = 0; i < arg2; i++ )
			PopScope();
		// jump out of loop
		ip = (byte*)(bp + arg);
		break;
	case op_enter:
		PushScope( new Scope( callstack.back() ) );
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
		if( !IsRefType( lhs.type ) && lhs.type != obj_module && lhs.type != obj_string && lhs.type != obj_symbol_name )
			throw RuntimeException( boost::format( "'%1%' is not a type with members." ) % lhs );
		if( lhs.type == obj_symbol_name )
		{
			// TODO: is this runtime error or ICE?
//			if( rhs.type != obj_string )
			if( rhs.type != obj_string && rhs.type != obj_symbol_name )
				throw ICE( boost::format( "'%1%' is not a valid type for a member." ) % rhs );
			// see if there is a module with this name
			if( module_names.count( string( lhs.s ) ) == 0 )
				throw RuntimeException( boost::format( "Cannot find module '%1%'." ) % lhs.s );
			else
			{
				// see if there is a native module with this name
				map<string, module_fcn_finder>::iterator i = imported_native_modules.find( string( lhs.s ) );
				if( i != imported_native_modules.end() )
				{
					// get the native function for this function
					NativeFunction nf = (i->second)( string( rhs.s ) );
					if( !nf.p )
						throw RuntimeException( boost::format( "Cannot find function '%1%' in native module '%2%'." ) % rhs.s % lhs.s );
					Object fo( nf );
					stack.push_back( fo );
				}
				// no native module, try as a deva symbol/module
				else
				{
					throw RuntimeException( boost::format( "Cannot find module '%1%'." ) % lhs.s );
				}
			}
		}
		// string:
		else if( lhs.type == obj_string )
		{
			// handle string built-in methods
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
		// module:
		else if( lhs.type == obj_module )
		{
			// look up the rhs as a symbol in the module
			Object* obj = FindSymbol( rhs.s, lhs.mod );
			if( !obj )
				throw RuntimeException( boost::format( "Cannot find '%1%' in module." ) % rhs.s );
			IncRef( *obj );
			stack.push_back( *obj );
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
			dword idx = (dword)rhs.d;
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
					// not found? try it as a symbol name
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
						}
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
							// push the method
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
		if( !IsRefType( lhs.type ) && lhs.type != obj_module && lhs.type != obj_string && lhs.type != obj_symbol_name )
			throw RuntimeException( boost::format( "'%1%' is not a type that has methods (string, vector, map, class, instance or module)." ) % lhs );

		if( lhs.type == obj_symbol_name )
		{
			// TODO: is this runtime error or ICE?
//			if( rhs.type != obj_string )
			if( rhs.type != obj_string && rhs.type != obj_symbol_name )
				throw ICE( boost::format( "'%1%' is not a valid type for a member." ) % rhs );
			// see if there is a module with this name
			if( module_names.count( string( lhs.s ) ) == 0 )
				throw RuntimeException( boost::format( "Cannot find module '%1%'." ) % lhs.s );
			else
			{
				// see if there is a native module with this name
				map<string, module_fcn_finder>::iterator i = imported_native_modules.find( string( lhs.s ) );
				if( i != imported_native_modules.end() )
				{
					// get the native function for this function
					NativeFunction nf = (i->second)( string( rhs.s ) );
					if( !nf.p )
						throw RuntimeException( boost::format( "Cannot find function '%1%' in native module '%2%'." ) % rhs.s % lhs.s );
					Object fo( nf );
					stack.push_back( fo );
				}
				// no native module, try as a deva symbol/module
				else
				{
					throw RuntimeException( boost::format( "Cannot find module '%1%'." ) % lhs.s );
				}
			}
		}
		// string:
		else if( lhs.type == obj_string )
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
		// module:
		else if( lhs.type == obj_module )
		{
			// look up the rhs as a symbol in the module
			Object* obj = FindSymbol( rhs.s, lhs.mod );
			if( !obj )
				throw RuntimeException( boost::format( "Cannot find '%1%' in module." ) % rhs.s );
			IncRef( *obj );
			stack.push_back( *obj );
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
				if( rhs.type == obj_symbol_name || rhs.type == obj_string )
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
							// push the class/instance ('this') for instances/classes
							if( lhs.type == obj_instance || lhs.type == obj_class )
							{
								IncRef( lhs );
								stack.push_back( lhs );
							}
						}
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
			int sz = (int)strlen( o.s );
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
			int sz = (int)o.v->size();
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
			int sz = (int)strlen( o.s );
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
				// call 'reserve' on the string to reduce allocations?
				string slice;
				slice.reserve( r.length() / step );
				// then walk it grabbing every 'nth' character
				for( size_t i = 0; i < r.length(); i += step )
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
			int sz = (int)o.v->size();
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
			if( lhs.v->size() <= (dword)idx || idx < 0 )
				throw RuntimeException( "Out-of-bounds error indexing vector." );
			// dec ref the current tos2[tos1], as we're assigning into it
			DecRef( lhs.v->operator[]( idx ) );
			// set the new value
			lhs.v->operator[]( idx ) = o;
		}
		// map/class/instance:
		else
		{
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

		int sz = (int)lhs.v->size();
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

		int sz = (int)lhs.v->size();

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
			if( lhs.v->size() <= (dword)idx || idx < 0 )
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
			if( lhs.v->size() <= (dword)idx || idx < 0 )
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
			if( lhs.v->size() <= (dword)idx || idx < 0 )
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
			if( lhs.v->size() <= (dword)idx || idx < 0 )
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
			if( lhs.v->size() <= (dword)idx || idx < 0 )
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
		for( dword i = 0; i < arg; i++ )
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
		int stack_size = (int)stack.size();
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
		if( stack.size() < (dword)arg+1 )
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

Opcode Executor::SkipInstruction()
{
	// decode opcode
	Opcode op = (Opcode)*ip;

	ip++;
	switch( op )
	{
	// 0 args
	case op_nop:
	case op_pop:
	case op_push_true:
	case op_push_false:
	case op_push_null:
	case op_push_zero:
	case op_push_one:
	case op_push0:
	case op_push1:
	case op_push2:
	case op_push3:
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
	case op_enter:
	case op_leave:
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
	case op_dup1:
	case op_dup2:
	case op_dup3:
	case op_dup_top_n:
	case op_dup_top1:
	case op_dup_top2:
	case op_dup_top3:
	case op_swap:
	case op_rot2:
	case op_rot3:
	case op_rot4:
	case op_halt:
	case op_inc:
	case op_dec:
		break;

	// 1 arg
	case op_push:
	case op_pushlocal:
	case op_pushconst:
	case op_storeconst:
	case op_store_true:
	case op_store_false:
	case op_store_null:
	case op_storelocal:
	case op_def_local:
	case op_new_map:
	case op_new_vec:
	case op_new_class:
	case op_jmp:
	case op_jmpt:
	case op_jmpf:
	case op_add_assign:
	case op_sub_assign:
	case op_div_assign:
	case op_mod_assign:
	case op_add_assign_local:
	case op_sub_assign_local:
	case op_mul_assign_local:
	case op_div_assign_local:
	case op_mod_assign_local:
	case op_call:
	case op_call_method:
	case op_return:
	case op_for_iter:
	case op_for_iter_pair:
	case op_dup:
	case op_rot:
	case op_import:
		ip += sizeof( dword );
		break;

	// 2 args
	case op_exit_loop:
		ip += 2 * sizeof( dword );
		break;

	// 3 args
	case op_def_function:
		ip += 3 * sizeof( dword );
		break;

	// 4 args
	case op_def_method:
		ip += 4 * sizeof( dword );
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
	dword args_passed = (dword)num_args;
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

	if( (dword)num_args > f->num_args )
		throw RuntimeException( boost::format( "Too many arguments passed to function '%1%'." ) % f->name );
	if( (f->num_args - num_args) > (dword)f->NumDefaultArgs() )
		throw RuntimeException( boost::format( "Not enough arguments passed to function '%1%'." ) % f->name );

	// if this is a module, push the module scope and frame
	// and set the ip, bp, end ptrs
	Code* orig_code;
	byte *orig_ip, *orig_bp, *orig_end;
	if( f->InModule() )
	{
		// TODO: do we need to assert that the module was set??
		// assert( f->module != NULL );
		PushFrame( f->module->frame );
		PushScope( f->module->scope );
		orig_code = cur_code;
		orig_end = end;
		orig_bp = bp;
		orig_ip = ip;
		bp = f->module->code->code;
		end = bp + f->module->code->len;
		ip = bp;
		cur_code = (Code*)f->module->code;
	}
	// otherwise this is 'main', set the module to 'main'
	else
	{
		orig_code = cur_code;
		orig_end = end;
		orig_bp = bp;
		orig_ip = ip;
		bp = code_blocks[0]->code;
		end = bp + code_blocks[0]->len;
		ip = bp;
		cur_code = (Code*)code_blocks[0];
	}

	// create a frame for the fcn
	// TODO: REVIEW: shouldn't we actually be passing 'orig_ip' in? (if we do we
	// cause errors in tests that eval or import, complaining that end-of-code
	// is reached before a fcn returns, but...)
//	Frame* frame = new Frame( CurrentFrame(), scopes, orig_ip, orig_ip - sizeof(dword) - 1, num_args, f );
	Frame* frame = new Frame( CurrentFrame(), scopes, ip, orig_ip - sizeof(dword) - 1, num_args, f );
	Scope* scope = new Scope( frame, true );

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
	for( dword i = 0; i < args_passed; i++ )
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
		int non_defaults = (int)(f->num_args - f->NumDefaultArgs());
		for( int i = 0; i < num_defaults; i++ )
		{
			int idx = f->default_args.at( num_args + i - non_defaults );
			Object o = GetConstant( idx );
			frame->SetLocal( num_args+i, o );
		}
	}

	// push the frame onto the callstack
	PushFrame( frame );
	PushScope( scope );
	// jump to the function
	ip = (byte*)(bp + f->addr);
	size_t stack_size = stack.size();
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

	// if this is a module, pop the module scope and frame
	// and restore the ip, bp and end ptrs
	if( f->InModule() )
	{
		PopScope();
		PopFrame();
	}
	// restore the ip, bp, end ptrs
	cur_code = orig_code;
	end = orig_end;
	bp = orig_bp;
	ip = orig_ip;
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
	Scope* scope = new Scope( frame, true );
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
	size_t stack_size = stack.size();
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

bool Executor::ImportBuiltinModule( const char* module_name )
{
	map<string, module_fcn_finder>::iterator i = native_modules.find( string( module_name ) );
	if( i == native_modules.end() )
		return false;
	imported_native_modules.insert( *i );
	return true;
}

// helper function for ImportModule
string Executor::FindModule( string mod )
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
		char* deva_env = getenv( "DEVA" );
		if( deva_env )
		{
			string devapath( deva_env );
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
	}
	// not found, error
	throw RuntimeException( boost::format( "Unable to locate module '%1%' for import." ) % mod );
}

const Code* Executor::LoadText( const char* const text, const char* const name )
{
	// load and parse the text
	ParseReturnValue prv;
	PassOneReturnValue p1rv;

	// NOTE: compiler and semantics global objects should be free'd and NULL at
	// this point!

	// parse the file
	prv = Parse( text, strlen( text ) );

	if( prv.successful )
	{
		PassOneFlags p1f; // currently no pass one flags
		PassTwoFlags p2f;

		// PASS ONE: build the symbol table and check semantics
		p1rv = PassOne( prv, p1f );

		// PASS TWO: compile
		Code* c = PassTwo( name, p1rv, p2f );

		// free parser, compiler memory
		FreeParseReturnValue( prv );
		FreePassOneReturnValue( p1rv );
		// free the semantics and compiler objects (symbol table et al) 
		delete compiler;
		compiler = NULL;
		delete semantics;
		semantics = NULL;

		// return the code
		return c;
	}
	else
		throw RuntimeException( "Unable to load input text." );
}

// helper fcn for parsing and compiling a module
const Code* const Executor::LoadModule( string module_name, string fname )
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

		// PASS ONE: build the symbol table and check semantics
		p1rv = PassOne( prv, p1f );

		// PASS TWO: compile
		Code* c = PassTwo( module_name.c_str(), p1rv, p2f );

		// free parser, compiler memory
		FreeParseReturnValue( prv );
		FreePassOneReturnValue( p1rv );
		// free the semantics and compiler objects (symbol table et al) 
		delete compiler;
		compiler = NULL;
		delete semantics;
		semantics = NULL;

		// return the code
		return c;
	}
	else
		throw RuntimeException( boost::format( "Unable to load module '%1%'." ) % fname );
}

Module* Executor::AddModule( const char* name, const Code* c, Scope* s, Frame* f )
{
	// add the module to the collection
	Module* mod = new Module( c, s, f );
	module_names.insert( name );
	modules.insert( pair<string, Module*>(name, mod ) );
	return mod;
}

//void Executor::FixupConstants()
//{
//	// delta to adjust constant indices is the number of constants defined in
//	// the main module
//	size_t delta = (ex->constants.size() - num_of_constant_symbols);
//
//	// find each 'pushconst' or 'storeconst' instruction and fixup its argument
//	Opcode op = op_nop;
//
//	dword arg;
//
//	// save the ip
//	byte* orig_ip = ip;
//
//	// execute until end
//	while( ip < end && op < op_halt )
//	{
//		//op = ExecuteInstruction();
//		// decode opcode
//		Opcode op = (Opcode)*ip;
//
//		if( op == op_pushconst || op == op_storeconst )
//		{
//			// fixup the arg
//			arg = *((dword*)ip);
//			arg += delta;
//			*((dword*)ip) = arg;
//		}
//		// next instruction
//		SkipInstruction();
//	}
//
//	// reset the ip
//	ip = orig_ip;
//}

Object Executor::ImportModule( const char* module_name )
{
	// prevent importing the same module more than once
	vector< pair<string, ScopeTable*> >::iterator it;
	if( modules.count( string( module_name ) ) != 0 )
		return Object( modules.at( string( module_name ) ) );

	// check the list of builtin modules first
	if( ImportBuiltinModule( module_name ) )
		return Object( obj_null );

	// otherwise look for the .dv/.dvc file to import
	string path = FindModule( module_name );

	// for now, just run the file by short name with ".dvc" extension (i.e. in
	// the current working directory)
	string dvfile( path + ".dv" );
	string dvcfile( path + ".dvc" );
	// save the ip, bp and end ptrs
	Code* orig_code = cur_code;
	byte* orig_ip = ip;
	byte* orig_bp = bp;
	byte* orig_end = end;
	// create a new namespace and set it at the current scope
	// TODO: currently this only adds the "short" name of the module as a
	// namespace. should the full path be used somehow?? foo::bar? foo.bar?
	// foo-bar? foo/bar?
	string mod( module_name );
	mod = get_file_part( mod );

	//////////////////////////////////////////////////////////
	// check to see if the .dvc (output) file exists, and is newer than the .dv (input ) file
	bool use_dvc = false;

	struct stat in_statbuf;
	struct stat out_statbuf;

	// if we can't open the .dvc file, can't load it
	if( stat( dvcfile.c_str(), &out_statbuf ) != -1 )
	{
		if( stat( dvfile.c_str(), &in_statbuf ) != -1 ) 
		{
			// if the output (dvc) is newer than the input (dv), use the dvc
			if( out_statbuf.st_mtime > in_statbuf.st_mtime )
				use_dvc = true;
		}
		// .dvc file exists, but no .dv file
		else
			use_dvc = true;
	}

	const Code* code = NULL;

	// if we're not using the dvc file, 
	// load and compile the code and write the dvc file
	if( !use_dvc )
	{
		code = LoadModule( mod, dvfile );
		if( !code )
			throw RuntimeException( boost::format( "Unable to load module '%1%'." ) % mod );
		WriteCode( dvcfile, code );
	}
	else
	{
		// otherwise we can just read the existing .dvc file
		code = ReadCode( dvcfile );
	}
	// add the constant for this module name
	char* str = copystr( mod );
	if( !cur_code->AddConstant( Object( obj_symbol_name, str ) ) )
		delete[] str;

	// create a new module and add it to the module collection
	// find our 'module' function, "module@main"
	Object *mod_main = FindFunction( "@main", mod, 0 );
	Frame* frame = new Frame( NULL, scopes, code->code, code->code, 0, mod_main->f, true );
	PushFrame( frame );
	Scope* scope = new Scope( frame, false, true );
	PushScope( scope );
	Module* cur_module = AddModule( mod.c_str(), code, scope, frame );

	// currently importing 'mod', set the flag
	Module* prev_mod = s_currently_importing_module;
	s_currently_importing_module = cur_module;

	// execute it
	ExecuteCode( code );

	// no longer importing this module, reset the flag
	s_currently_importing_module = prev_mod;

	// add the module to load-ordered stack (for deletion in depth first order)
	module_stack.push_back( cur_module );

	// pop the scope and frame (because these are _module_-level scopes and
	// frames they will not be deleted - the Module object has a ptr to them
	// which will be deleted at exit)
	PopScope();
	PopFrame();
	
	// restore the ip, bp and end ptrs
	cur_code = orig_code;
	ip = orig_ip;
	bp = orig_bp;
	end = orig_end;

	return Object( cur_module );
}

Code* Executor::GetCode( byte* address )
{
	// walk each code block looking for one with a code ptr equal to our address
	for( vector<const Code*>::iterator i = code_blocks.begin(); i != code_blocks.end(); ++i )
	{
		if( (*i)->code == address )
			return (Code*)*i;
	}
	return NULL;
}

// .dv file writing
void Executor::WriteCode( string filename, const Code* const code )
{
	// open the file for writing
	ofstream file;
	file.open( filename.c_str(), ios::binary );
	if( file.fail() )
		throw RuntimeException( boost::format( "Unable to open file '%1%'" ) % filename );

	// write the header
	// "deva"
	file << file_hdr_deva;
	file << '\0';
	// "2.0.0"
	file << file_hdr_ver;
	file << '\0';
	// 5 bytes of padding to bring header to 16 bytes
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';

	// write constants
	// (do NOT write the 'global' constants, they always exist)
	file << constants_hdr;
	file << '\0';
	// padding to byte boundary
	file << '\0';
	// section header is followed by dword containing the number of const objects
	dword num_consts = code->NumConstants();
	file.write( (char*)&num_consts, sizeof( dword ) );
	// constant data itself is an array of DevaObject structs:
	// byte : object type. only number and string are allowed
	// qword (number) OR 'len+1' bytes (string) : number or null-terminated string
	for( int i = 0; i < code->NumConstants(); i++ )
	{
		Object o = code->GetConstant( i );
		qword qw = 0;
		file << (byte)(o.type);
		switch( o.type )
		{
		case obj_number:
			qw = (qword)o.d;
			file.write( (char*)&qw, sizeof( qword ) );
			break;
		case obj_string:
			file << o.s;
			file << '\0';
			break;
		case obj_size:
			qw = (qword)o.sz;
			file.write( (char*)&qw, sizeof( qword ) );
			break;
		case obj_symbol_name:
			file << o.s;
			file << '\0';
			break;
		default:
			// null shouldn't ever happen, null is 'global'
			// boolean shouldn't ever happen, true and false are 'global'
			throw ICE( "Trying to write Object of invalid type for Constant Pool." );
			break;
		}
	}

	// write functions
	file << functions_hdr;
	file << '\0';
	// padding to byte boundary
	file << '\0';
	file << '\0';
	// section header is followed by a dword containing the number of function objects
	dword dw = (dword)functions.size();
	file.write( (char*)&dw, sizeof( dword ) );
	// function object data itself is an array of Function objects:
	// len+1 bytes : 	name, null-terminated string
	// len+1 bytes : 	filename
	// dword :			starting line
	// len+1 bytes :	classname (zero-length string if non-method)
	// dword : 			number of arguments
	// dword : 			'n' number of default arguments
	// 'n' dwords :		default args (constant pool indices)
	// dword :			number of locals
	// dword :			number of names (externals, undeclared vars, functions)
	// byte[] :			names, len+1 bytes null-terminated string each
	// dword :			offset in code section of the code for this function
	for( multimap<string, Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		//////////////////////////////////////////////////////
		// TODO: only write functions from this file/module!!!
		//////////////////////////////////////////////////////

		Function* f = i->second->f;
		// name
		file << f->name;
		file << '\0';

		// filename (module)
		file << f->filename;
		file << '\0';

		// starting line
		dw = f->first_line;
		file.write( (char*)&dw, sizeof( dword ) );

		// classname, empty if not method
		file << f->classname;
		file << '\0';

		// number of arguments
		dw = f->num_args;
		file.write( (char*)&dw, sizeof( dword ) );
	
		// default args
		dw = (dword)(f->default_args.size());
		file.write( (char*)&dw, sizeof( dword ) );
		for( size_t j = 0; j < f->default_args.size(); j++ )
		{
			dw = (dword)(f->default_args[j]);
			file.write( (char*)&dw, sizeof( dword ) );
		}

		// local names (for debugging & reflection)
		dw = (dword)(f->local_names.size());
		file.write( (char*)&dw, sizeof( dword ) );
		for( size_t j = 0; j < f->local_names.size(); j++ )
		{
			file << f->local_names[j];
			file << '\0';
		}
	
		// offset in code section of the code for this function
		dw = f->addr;
		file.write( (char*)&dw, sizeof( dword ) );
	}

	// write line mapping
	file << linemap_hdr;
	file << '\0';
	// padding to byte boundary
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	file << '\0';
	// section header is followed by a dword containing the number of line map entries
	dword num_linemaps = (dword)(code->lines->L2ASize());
	file.write( (char*)&num_linemaps, sizeof( dword ) );
	// line map data is an array of line map entries:
	// dword : 			line number
	// dword :			address of instruction
	for( map<dword, dword>::iterator i = code->lines->L2ABegin(); i != code->lines->L2AEnd(); ++i )
	{
		dw = i->first;
		file.write( (char*)&dw, sizeof( dword ) );
		dw = i->second;
		file.write( (char*)&dw, sizeof( dword ) );
	}

	// write bytecode
	file.write( (const char*)code->code, code->len );
	
	// close the file
	file.close();
}

// .dv file reading
Code* Executor::ReadCode( string filename )
{
	Code* code = new Code();
	code->lines = new LineMap();

	// open the file for reading
	ifstream file;
	file.open( filename.c_str(), ios::binary );
	if( file.fail() )
		throw RuntimeException( boost::format( "Unable to open input file '%1%' for read." ) % filename );

	// read the header
	char deva[5] = {0};
	char ver[6] = {0};
	file.read( deva, 5 );
	if( strcmp( deva, "deva") != 0 )
		throw RuntimeException( "Invalid .dvc file: header missing 'deva' tag." );
	file.read( ver, 6 );
	if( strcmp( ver, "2.0.0" ) != 0 )
		 throw RuntimeException( boost::format( "Invalid .dvc version number: %1%." ) % ver );
	char pad[6] = {0};
	file.read( pad, 5 );
	if( pad[0] != 0 || pad[1] != 0 || pad[2] != 0 || pad[3] != 0 || pad[4] != 0 )
		throw RuntimeException( "Invalid .dvc file: malformed header after version number." );

	// read the constants
	char consts_hdr[7];
	char consts_hdr_pad[1];
	file.read( consts_hdr, 7 );
	if( strcmp( consts_hdr, ".const" ) != 0 )
		throw RuntimeException( "Invalid .dvc file: constant section header missing or malformed." );
	file.read( consts_hdr_pad, 1 );
	if( consts_hdr_pad[0] != 0 )
		throw RuntimeException( "Invalid .dvc file: constant section header missing or malformed." );
	// section header is followed by dword containing the number of const objects
	dword num_consts = 0;
	file.read( (char*)&num_consts, sizeof( dword ) );
	// constant data itself is an array of DevaObject structs:
	// byte : object type. only number and string are allowed
	// qword (number) OR 'len+1' bytes (string) : number or null-terminated string
	for( dword i = 0; i < num_consts; i++ )
	{
		Object o;
		byte type;
		file.read( (char*)&type, sizeof( byte ) );
		o.type = (ObjectType)type;
		switch( type )
		{
		case obj_number:
			{
			int64_t qw = 0;
			file.read( (char*)&qw, sizeof( qword ) );
			o.d = (double)qw;
			}
			break;
		case obj_string:
			{
			string s;
			getline( file, s, '\0' );
			o.s = copystr( s );
			}
			break;
		case obj_size:
			file.read( (char*)&o.sz, sizeof( dword ) );
			break;
		case obj_symbol_name:
			{
			string s;
			getline( file, s, '\0' );
			o.s = copystr( s );
			}
			break;
		default:
			throw ICE( "Invalid .dvc file: read Object of invalid type for Constant Pool." );
			break;
		}
		code->AddConstant( o );
	}

	// read the function table
	char funcs_hdr[6];
	file.read( funcs_hdr, 6 );
	if( strcmp( funcs_hdr, ".func" ) != 0 )
		throw RuntimeException( "Invalid .dvc file: function section header missing or malformed." );
	char funcs_hdr_pad[2];
	file.read( funcs_hdr_pad, 2 );
	if( funcs_hdr_pad[0] != 0 || funcs_hdr_pad[1] != 0 )
		throw RuntimeException( "Invalid .dvc file: function section header missing or malformed." );
	// section header is followed by a dword containing the number of function objects
	dword num_funcs = 0;
	file.read( (char*)&num_funcs, sizeof( dword ) );
	// function object data itself is an array of Function objects:
	// len+1 bytes : 	name, null-terminated string
	// len+1 bytes : 	filename
	// dword :			starting line
	// len+1 bytes :	classname (zero-length string if non-method)
	// dword : 			number of arguments
	// dword : 			'n' number of default arguments
	// 'n' dwords :		default args (constant pool indices)
	// dword :			number of locals
	// dword :			number of names (externals, undeclared vars, functions)
	// byte[] :			names, len+1 bytes null-terminated string each
	// dword :			offset in code section of the code for this function
	for( dword i = 0; i < num_funcs; i++ )
	{
		Function* f = new Function();
		string s;
		getline( file, s, '\0' );
		// name
		f->name = s;
	
		// filename
		getline( file, s, '\0' );
		f->filename = s;
	
		// first line
		file.read( (char*)&f->first_line, sizeof( dword ) );
	
		// classname
		getline( file, s, '\0' );
		f->classname = s;
	
		// num args
		file.read( (char*)&f->num_args, sizeof( dword ) );
	
		// default args
		dword num_def_args = 0;
		file.read( (char*)&num_def_args, sizeof( dword ) );
		for( size_t j = 0; j < num_def_args; j++ )
		{
			dword idx = 0;
			file.read( (char*)&idx, sizeof( dword ) );
			f->default_args.push_back( idx );
		}

		// local names (for debugging & reflection)
		dword num_locals = 0;
		file.read( (char*)&num_locals, sizeof( dword ) );
		for( size_t i = 0; i < num_locals; i++ )
		{
			getline( file, s, '\0' );
			f->local_names.push_back( s );
		}
	
		// address
		file.read( (char*)&f->addr, sizeof( dword ) );
	
		// module ptr will be set later
		f->module = NULL;
		// module name
		string filepart = get_file_part( f->filename );
		string mod = get_stem( filepart );
		f->modulename = mod;
	
		AddFunction( f );
	}

	// read the line mapping
	char lm_hdr[9];
	char lm_hdr_pad[7];
	file.read( lm_hdr, 9 );
	if( strcmp( lm_hdr, ".linemap" ) != 0 )
		throw RuntimeException( "Invalid .dvc file: line map section header missing or malformed." );
	file.read( lm_hdr_pad, 7 );
	if( lm_hdr_pad[0] != 0 || lm_hdr_pad[1] != 0 || lm_hdr_pad[2] != 0 || lm_hdr_pad[3] != 0 ||
		lm_hdr_pad[4] != 0 || lm_hdr_pad[5] != 0 || lm_hdr_pad[6] != 0 )
		throw RuntimeException( "Invalid .dvc file: line map section header missing or malformed." );
	// section header is followed by a dword containing the number of line map entries
	dword num_linemaps = 0;
	file.read( (char*)&num_linemaps, sizeof( dword ) );
	// line map data is an array of line map entries:
	// dword : 			line number
	// dword :			address of instruction
	for( dword i = 0; i < num_linemaps; i++ )
	{
		dword line = 0;
		dword addr = 0;
		file.read( (char*)&line, sizeof( dword ) );
		file.read( (char*)&addr, sizeof( dword ) );
		code->lines->Add( line, addr );
	}

	// get the size of the byte code
	file.seekg( 0, ios::cur );
	int beg = file.tellg();
	file.seekg( 0, ios::end );
	int end = file.tellg();
	dword bc_length = end - beg;
	file.seekg( beg );

	// allocate memory for the byte code array
	code->code = new byte[bc_length];
	code->len = bc_length;

	// read the file into the byte code array
	file.read( (char*)(code->code), bc_length );

	// close the file
	file.close();

	return code;
}

// disassemble the instruction stream to stdout
void Executor::Decode( const Code* code)
{
	byte* b = code->code;
	byte* p = b;
	byte* end = code->code + code->len;
	Opcode op = op_nop;
	dword line = LineMap::end;
	dword prev_line = 0;

	cout << "Instructions:" << endl;
	while( p < end && op < op_halt )
	{
		// check line number
		prev_line = line;
		line = code->lines->FindLine( p - b );
		if( line != LineMap::end && prev_line != line )
			cout << line << endl;

		// decode opcode
		op = (Opcode)*p;
		p += PrintOpcode( code, op, b, p );
		cout << endl;
	}
}

int Executor::PrintOpcode( const Code* code, Opcode op, const byte* b, byte* p )
{
	int arg, arg2;
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
		o = GetConstant( code, arg );
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_storeconst:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant( code, arg );
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_store_true:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant( code, arg );
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_store_false:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant( code, arg );
		cout << "\t" << arg << " (" << o << ")";
		ret = sizeof( dword );
		break;
	case op_store_null:
		// 1 arg
		arg = *((dword*)p);
		// look-up the constant
		o = GetConstant( code, arg );
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
		{
		// 3 args: constant index of name, const idx of module name, fcn address
		// look-up the module name constant
		arg = *((dword*)(p + sizeof( dword )));
		o = GetConstant( code, arg );

		// look-up the module name constant
		arg2= *((dword*)p);
		Object o2 = GetConstant( code, arg2 );

		// address
		dword arg3= *((dword*)(p + (2 * sizeof( dword ))));

		cout << arg << " " << arg2 << " (" << o << " # " << o2 << "), " << arg3;

		ret = sizeof( dword ) * 3;
		}
		break;
	case op_def_method:
		{
		// 4 args: constant index of function name, constant index of class name, const idx of module name, fcn address
		// look-up the module name constant
		arg = *((dword*)( p + (2 * sizeof( dword ))));
		o = GetConstant( code, arg );

		// look-up the function name constant
		arg2 = *((dword*)p);
		Object o2 = GetConstant( code, arg2 );

		// look-up the class name constant
		dword arg3 = *((dword*)( p + sizeof( dword ) ) );
		Object o3 = GetConstant( code, arg3 );

		// address
		dword arg4 = *((dword*)( p + (3 * sizeof( dword ))));

		cout << arg << " " << arg2 << " " << arg3 << " (" << o << " # " << o2 << " @ " << o3 << "), " << arg4;

		ret = sizeof( dword ) * 4;
		}
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
	case op_jmpt:
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
	case op_inc:
	case op_dec:
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
		o = GetConstant( code, arg );
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
		o = GetConstant( code, arg );
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
	for( multimap<string,Object*>::iterator i = functions.begin(); i != functions.end(); ++i )
	{
		Function* f = i->second->f;
		cout << "function: " << f->name << ", from file: " << f->filename << ", line: " << f->first_line;
		cout << endl;
		cout << f->num_args << " arg(s), default value indices: ";
		for( size_t j = 0; j < f->NumDefaultArgs(); j++ )
			cout << f->default_args.at( j ) << " ";
		cout << endl << f->local_names.size() << " local(s): ";
		for( size_t j = 0; j < f->local_names.size(); j++ )
			cout << f->local_names.operator[]( j ) << " ";
		cout << endl << "code address: " << f->addr << endl;
	}
}

void Executor::DumpConstantPool( const Code* code )
{
	cout << "Constant data pool:" << endl;
	for( int i = 0; i < code->NumConstants(); i++ )
	{
		Object o = code->GetConstant( (int)i );
		if( o.type == obj_string )
			cout << "string: " << o.s << endl;
		else if( o.type == obj_symbol_name )
			cout << "symbol name: " << o.s << endl;
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
	size_t n = (stack.size() > 5 ? 5 : stack.size());
	if( n != 0 )
	{
		for( int i = (int)n-1; i >= 0; i-- )
		{
			cout << stack[stack.size()-(n-i)];
			if( i > 0 )
				cout << ", ";
		}
	}
	cout << "]";
	if( stack.size() > 5 )
		cout << " +" << stack.size()-5 << " more";
	cout << endl;
}

size_t Executor::GetOffsetForCallSite( Frame* f, byte* addr ) const
{
	if( f->IsNative() )
		return addr - bp;
	Module* mod = f->GetFunction()->module;
	if( !mod )
	{
		Code* c = (Code*)code_blocks[0];
		return addr - c->code;
	}
	return addr - mod->code->code;
}

void Executor::DumpTrace( ostream & os )
{
	// nothing to do?
	if( callstack.size() == 0 )
		return;

	os << "Traceback (most recent first):" << endl;
	string fcn, file;
	int line = -1;
	int depth = 1;
	Code* code = NULL;
	Frame* f;

	// current (error) location:
	size_t loc = GetOffsetForCallSite( callstack.back(), ip );
	code = GetCode( bp );
	line = code->lines->FindLine( loc );
	f = callstack.back();
	if( f->IsNative() )
		os << "file: [Native Module]" << ", at: " << loc << ", in [Native Function]" << endl;
	else
		os << "file: " << f->GetFunction()->filename << ", line: " << line << ", at: " << loc << ", in " << f->GetFunction()->name << endl;

	// stack trace:
	for( size_t i = callstack.size() - 1; i > 0; i-- )
	{
		line = -1;
		code = NULL;
		Module* mod = NULL;

		f = callstack[i];
		// ignore module frames, they aren't real 'calls'
		if( f->IsModule() )
			continue;
		Frame* prev;
		prev = callstack[i-1];
		// if previous frame was a module frame, try the one prior to that
		if( prev->IsModule() )
		{
			if( i < 2 )
				return;
			prev = callstack[i-2];
		}

		// this frame determines the fcn name
		if( f->IsNative() )
		{
			fcn = "[Native Function]";
		}
		else
		{
			fcn = f->GetFunction()->name;
		}
		// the frame calling this frame determines the file, module, call site
		// address
		if( prev->IsNative() )
		{
			file = "[Native Module]";
		}
		else
		{
			file = prev->GetFunction()->filename;
			mod = prev->GetFunction()->module;
		}

		// a NULL module indicates the 'main' module/file
		if( mod )
			code = (Code*)mod->code;
		else
			code = (Code*)code_blocks[0];

		size_t call_site = GetOffsetForCallSite( prev, f->GetCallSite() );
		if( code )
			line = code->lines->FindLine( call_site );


		// pad for callstack depth
		for( int c = 0; c < depth; c++ )
			os << " ";

		// display the stack frame info
		if( line == -1 )
			os << "file: " << file << ", at: " << call_site << ", call to " << fcn << endl;
		else
			os << "file: " << file << ", line: " << line << ", at: " << call_site << ", call to " << fcn << endl;
		depth++;
	}
}

} // namespace deva
