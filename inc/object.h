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

// object.h
// object type for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "opcodes.h"
#include "ordered_set.h"

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstring>

using namespace std;


namespace deva
{

enum ObjectType
{
	obj_null,
	obj_boolean,
	obj_number,
	obj_string,
	obj_vector,
	obj_map,
	obj_function,
	obj_native_function,	// a native (C) function
	obj_class,
	obj_instance,
	obj_native_obj,			// a native (C/C++) object (void*)
	obj_size,				// an integral sized value (internal use only, for code addresses etc)
	obj_end = 255			// end of enum marker
};

class Frame;
typedef void (*NativeFunction)(Frame*);
class DevaVector;
struct DevaMap;
struct DevaFunction;

struct DevaObject
{
	ObjectType type;
	union
	{
		double d;
		char* s;
		bool b;
		DevaVector* v;
		DevaMap* m;
		DevaFunction* f;
		NativeFunction nf;
		void* no;
		size_t sz;
	};

	DevaObject() : type( obj_end ) {} // invalid object
	DevaObject( ObjectType t ) : type( t ) {} // uninitialized object
	DevaObject( double n ) : type( obj_number ), d( n ) {}
	DevaObject( char* n ) : type( obj_string ), s( n ) {}
	DevaObject( bool n ) : type( obj_boolean ), b( n ) {}
	DevaObject( DevaVector* n ) : type( obj_vector ), v( n ) {}
	DevaObject( DevaMap* n ) : type( obj_map ), m( n ) {}
	DevaObject( DevaFunction* n ) : type( obj_function ), f( n ) {}
	DevaObject( NativeFunction n ) : type( obj_native_function ), nf( n ) {}
	DevaObject( void* n ) : type( obj_native_obj ), no( n ) {}
	DevaObject( size_t n ) : type( obj_size ), sz( n ) {}

	// creation functions for classes & instances
	// (which are maps internally)
	static DevaObject* CreateClass( DevaMap* n )
	{ DevaObject* ret = new DevaObject(); ret->m = n; ret->type = obj_class; return ret; }
	void MakeClass( DevaMap* n ){ type = obj_class; m = n; }
	static DevaObject* CreateInstance( DevaMap* n )
	{ DevaObject* ret = new DevaObject(); ret->m = n; ret->type = obj_instance; return ret; }
	void MakeInstance( DevaMap* n ){ type = obj_instance; m = n; }

	// helper functions
	inline bool IsNull(){ return type == obj_null; }
	inline bool IsNumber(){ return type == obj_number; }
	inline bool IsString(){ return type == obj_string; }
	inline bool IsBoolean(){ return type == obj_boolean; }
	inline bool IsVector(){ return type == obj_vector; }
	inline bool IsMap(){ return type == obj_map; }
	inline bool IsFunction(){ return type == obj_function; }
	inline bool IsNativeFunction(){ return type == obj_native_function; }
	inline bool IsNativeObj(){ return type == obj_native_obj; }
	inline bool IsSize(){ return type == obj_size; }

	inline operator const bool (){ return b; }
	inline operator const double (){ return d; }
	inline operator const string (){ return s; }
	inline operator const DevaVector* (){ return v; }
	inline operator const DevaMap* (){ return m; }
	inline operator const DevaFunction* (){ return f; }
	inline operator const NativeFunction (){ return nf; }
	inline operator const void* (){ return no; }
	inline operator const size_t (){ return sz; }

	// equality operator
	bool operator == ( const DevaObject& rhs ) const;
	bool operator < ( const DevaObject & rhs ) const;

	// coerce to a boolean (for jmpf op, for instance)
	bool CoerceToBool();
};

// functor for comparing DevaObject ptrs
struct DO_ptr_lt
{
	bool operator()( const DevaObject*  lhs, const DevaObject* rhs ){ return lhs->operator < (*rhs); }
};

// TODO: should this be a list<>? dequeue<>?
struct DevaVector : public vector<DevaObject>
{
	// current index for enumerating the vector
	size_t index;

	// default constructor
	DevaVector() : vector<DevaObject>(), index( 0 )
	{}

	// copy constructor
	DevaVector( const DevaVector & v ) : vector<DevaObject>( v ), index( 0 )
	{}

	// 'slice constructor'
	DevaVector( const DevaVector & v, size_t start, size_t end ) : vector<DevaObject>( v.begin() + start, v.begin() + end ), index( 0 )
	{}
};

// TODO: make this a boost::unordered_map (hash map)
// (need to implement a boost hash_function for DevaObjects)
struct DevaMap : public map<DevaObject, DevaObject>
{
	// current index for enumerating the map pairs
	size_t index;

	// default constructor
	DevaMap() : map<DevaObject, DevaObject>(), index( 0 )
	{}

	// copy constructor
	DevaMap( const DevaMap & m ) : map<DevaObject, DevaObject>( m ), index( 0 )
	{}
};

struct DevaFunction
{
	// name
	string name;
	// filename
	string filename;
	// starting line
	dword first_line;
	// number of arguments
	dword num_args;
	// TODO: need to store default values for args too
	OrderedSet<int> default_args;
	// number of locals
	dword num_locals;
	// local names (for debugging & reflection)
	OrderedSet<string> local_names;
	// offset in code section of the code for this function
	dword addr;

	bool operator == ( const DevaFunction & rhs ) const
	{
		if( name == rhs.name && filename == rhs.filename && first_line == rhs.first_line )
			return true;
		else
			return false;
	}
	bool operator < ( const DevaFunction & rhs ) const
	{
		if( name == rhs.name )
			return first_line < rhs.first_line;
		else
			return name < rhs.name;
	}
};

// functor for comparing DevaFunction ptrs
struct DF_ptr_lt
{
	bool operator()( const DevaFunction* lhs, const DevaFunction* rhs ){ return lhs->operator < (*rhs); }
};

// operator << for printing DevaObjects
ostream & operator << ( ostream & os, DevaObject & obj );

} // namespace deva

#endif // __OBJECT_H__
