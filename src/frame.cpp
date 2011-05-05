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

// frame.cpp
// callstack frame object for the deva language
// created by jcs, january 07, 2010 

// TODO:
// * 

#include "frame.h"
#include "executor.h"
#include <vector>


using namespace std;


namespace deva
{


Frame::Frame( Frame* p, ScopeTable* s, byte* loc, byte* site, int args_passed, Function* f, bool is_mod /*= false*/ ) :
	is_module( is_mod ),
	parent( p ),
	function( f ), 
	is_native( false ), 
	num_args( args_passed ), 
	addr( loc ),
	call_site( site ),
	scopes( s )
{
	num_locals = f->IsMethod() ? f->local_names.size()+1 : f->local_names.size();
	if( num_locals ) locals.resize( num_locals );
}

Frame::Frame( Frame* p, ScopeTable* s, byte* loc, byte* site, int args_passed, NativeFunction f ) :
	is_module( false ),
	parent( p ),
	native_function( f ), 
	is_native( true ), 
	num_args( args_passed ), 
	addr( loc ),
	call_site( site ),
	scopes( s )
{
	num_locals = args_passed;
	if( num_locals ) locals.resize( num_locals );
}

Frame::~Frame()
{
	// free the local strings
	for( vector<char*>::iterator i = strings.begin(); i != strings.end(); ++i )
	{
		delete [] *i;
	}
	// dec ref the args
	int i = 0;
	// TODO: is this right? didn't the child scopes already do this??
	for( ; i < num_args; i++ )
	{
		DecRef( locals[i] );
	}
}

// resolve symbols through the scope table
Object* Frame::FindSymbol( const char* name ) const
{
	// search the locals for this frame
	Object* o = scopes->FindSymbol( name, true );
	if( o )
		return o;

	// function?
	o = scopes->FindFunction( name );
	if( o )
		return o;

	// check builtins
	if( IsBuiltin( string( name ) ) )
		return GetBuiltinObjectRef( string( name ) );
	else
	{
		// extern symbol?
		o = scopes->FindExternSymbol( name );
		if( o )
			return o;
		// type builtins? (string, vector, map)
		else if( IsStringBuiltin( string( name ) ) )
			return GetStringBuiltinObjectRef( string( name ) );
		else if( IsVectorBuiltin( string( name ) ) )
			return GetVectorBuiltinObjectRef( string( name ) );
		else if( IsMapBuiltin( string( name ) ) )
			return GetMapBuiltinObjectRef( string( name ) );
	}
	return NULL;
}

const char* Frame::FindSymbolName( Object* o )
{
	return scopes->FindSymbolName( o );
}

// copy all the strings in 'o' from the parent to here
// ('o' can be a string, or a vector/map/class/instance which can then can
// contain strings - which will be looked for recursively)
// returns an object wrapper of the object passed in that points to the
// copied strings
Object Frame::CopyStringsFromParent( Object & o )
{
	if( o.type == obj_string )
	{
		// add the string to the parent and get a ptr to it
		const char* str = GetParent()->AddString( string( o.s ) );
		return Object( str );
	}
	else if( o.type == obj_vector )
	{
		for( Vector::iterator i = o.v->begin(); i != o.v->end(); ++i )
		{
			*i = CopyStringsFromParent( *i );
		}
		return o;
	}
	else if( o.type == obj_map || o.type == obj_class || o.type == obj_instance )
	{
		for( Map::iterator i = o.m->begin(); i != o.m->end(); ++i )
		{
			Object first = i->first;
			Object obj1 = CopyStringsFromParent( first );
			// if obj1 is a string, force i->first.s to its value
			// (though forcing a 'set' to a const var in a loop like this is questionable at best,
			// in this case it is safe because the strings are identical in value, and Object's 
			// with a string type are compared by strcmp)
			if( obj1.type == obj_string )
			{
				char** s = (char**)&(i->first.s);
				*s = obj1.s;
			}
			Object obj2 = CopyStringsFromParent( i->second );
			if( obj2.type == obj_string )
				i->second = obj2;
		}
		return o;
	}
	else
		return o;
}

} // end namespace deva

