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
// object (and related) types for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "opcodes.h"
#include "ordered_set.h"
#include "refcounted.h"

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstring>

using namespace std;


namespace deva
{


// global flag to turn ref-count tracing on/off
extern bool reftrace;

// object type for tagged union object type
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
	obj_symbol_name,		// an identifier, same data as a string
	obj_end = 255			// end of enum marker
};
static const char* object_type_names[] = 
{
	"null",
	"boolean",
	"number",
	"string",
	"vector",
	"map",
	"function",
	"native function",
	"class",
	"object",
	"native object",
	"size/address",
	"symbol name",
	"<invalid>"
};


inline bool IsRefType( ObjectType t ) { return (t == obj_vector || t == obj_map || t == obj_class || t == obj_instance ); }
inline bool IsMapType( ObjectType t ) { return (t == obj_map || t == obj_class || t == obj_instance ); }
inline bool IsVecType( ObjectType t ) { return t == obj_vector; }


// forward decls needed by Object class
class Frame;
struct Function;
class VectorBase;
class MapBase;

// types needed by Object class
typedef RefCounted<VectorBase> Vector;
typedef RefCounted<MapBase> Map;
typedef void (*NativeFunctionPtr)(Frame*);
struct NativeFunction
{
	NativeFunctionPtr p; 
	bool is_method; 
};

struct Object
{
	ObjectType type;
	union
	{
		double d;
		char* s;
		bool b;
		Vector* v;
		Map* m;
		Function* f;
		NativeFunction nf;
		void* no;
		size_t sz;
	};

	Object() : type( obj_end ), d( 0.0 ) {} // invalid object
	Object( ObjectType t ) : type( t ) {} // uninitialized object
	Object( double n ) : type( obj_number ), d( n ) {}
	Object( char* n ) : type( obj_string ), s( n ) {}
	Object( const char* n ) : type( obj_string ), s( const_cast<char*>(n) ) {}
	Object( bool n ) : type( obj_boolean ), b( n ) {}
	Object( Vector* n ) : type( obj_vector ), v( n ) {}
	Object( Map* n ) : type( obj_map ), m( n ) {}
	Object( Function* n ) : type( obj_function ), f( n ) {}
	Object( NativeFunction n ) : type( obj_native_function ), nf( n ) {}
	Object( NativeFunctionPtr n, bool method = false ) : type( obj_native_function ) { nf.p = n; nf.is_method = method; }
	Object( void* n ) : type( obj_native_obj ), no( n ) {}
	Object( size_t n ) : type( obj_size ), sz( n ) {}
	Object( ObjectType t, char* n ) : type( obj_symbol_name ), s( n )
	{ /*assert( t == obj_symbol_name );*/ }
	Object( ObjectType t, const char* n ) : type( obj_symbol_name ), s( const_cast<char*>(n) )
	{ /*assert( t == obj_symbol_name );*/ }

	// creation functions for classes & instances
	// (which are maps internally)
	static Object CreateClass( Map* n );
	void MakeClass( Map* n ){ type = obj_class; m = n; }
	static Object CreateInstance( Map* n );
	void MakeInstance( Map* n ){ type = obj_instance; m = n; }

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
	inline operator const Vector* (){ return v; }
	inline operator const Map* (){ return m; }
	inline operator const Function* (){ return f; }
	inline operator const NativeFunction (){ return nf; }
	inline operator const void* (){ return no; }
	inline operator const size_t (){ return sz; }

	// equality operator
	bool operator == ( const Object& rhs ) const;
	bool operator < ( const Object & rhs ) const;

	// coerce to a boolean (for jmpf op, for instance)
	bool CoerceToBool();
};

// functor for comparing Object ptrs
struct DO_ptr_lt
{
	bool operator()( const Object*  lhs, const Object* rhs ){ return lhs->operator < (*rhs); }
};


// operator << for printing Objects
ostream & operator << ( ostream & os, Object & obj );
// operator << for printing ObjectTypes
ostream & operator << ( ostream & os, ObjectType t );


struct Object;

// TODO: should this be a list<>? dequeue<>?
class VectorBase : public vector<Object>
{
	// current index for enumerating the vector
	size_t index;
	friend void do_vector_rewind( Frame* );
	friend void do_vector_next( Frame* );

public:
	// default constructor
	VectorBase() : vector<Object>(), index( 0 ) {}

	// copy constructor
	VectorBase( const VectorBase & v ) : vector<Object>( v ), index( 0 ) {}

	// create with 'n' empty items
	VectorBase( size_t n ) : vector<Object>( n ), index( 0 ) {}

	// 'slice constructor'
	VectorBase( const VectorBase & v, size_t start, size_t end ) : vector<Object>( v.begin() + start, v.begin() + end ), index( 0 ) {}
};

// functions to create Vector objects
inline Vector* CreateVector() { return Vector::Create(); }
inline Vector* CreateVector( Vector & v ) { return Vector::Create( v ); }
inline Vector* CreateVector( size_t n ) { return Vector::Create( n ); }
inline Vector* CreateVector( Vector & v, size_t start, size_t end ) { return Vector::Create( v, start, end ); }

// TODO: make this a boost::unordered_map (hash map)
// (need to implement a boost hash_function for Objects)
class MapBase : public map<Object, Object>
{
	// current index for enumerating the map pairs
	size_t index;
	friend void do_map_rewind( Frame* );
	friend void do_map_next( Frame* );

public:
	// default constructor
	MapBase() : map<Object, Object>(), index( 0 )
	{}

	// copy constructor
	MapBase( const MapBase & m ) : map<Object, Object>( m ), index( 0 )
	{}
};

// functions to create Map objects
inline Map* CreateMap() { return Map::Create(); }
inline Map* CreateMap( Map & m ) { return Map::Create( m ); }


// helper functions for reference counting
extern bool last_op_was_return;
void IncRef( Object & o );
void IncRefChildren( Object & o );
int DecRef( Object & o );

struct Function
{
	// name
	string name;
	// filename (module)
	string filename;
	// starting line
	dword first_line;
	// classname, empty if not method
	string classname;
	// number of arguments
	dword num_args;
	// TODO: need to store default values for args too
	OrderedSet<int> default_args;
	// number of locals
	dword num_locals;
	// local names (for debugging & reflection)
	vector<string> local_names;
	// offset in code section of the code for this function
	dword addr;

	Function() : first_line( 0 ), num_args( 0 ), num_locals( 0 ), addr( 0 ) {}

	inline bool IsMethod() { return !classname.empty(); }
	inline int NumDefaultArgs() { return default_args.Size(); }

	bool operator == ( const Function & rhs ) const
	{
		if( name == rhs.name && filename == rhs.filename && first_line == rhs.first_line )
			return true;
		else
			return false;
	}
	bool operator < ( const Function & rhs ) const
	{
		if( name == rhs.name )
			return first_line < rhs.first_line;
		else
			return name < rhs.name;
	}
};

// functor for comparing Function ptrs
struct DF_ptr_lt
{
	bool operator()( const Function* lhs, const Function* rhs ){ return lhs->operator < (*rhs); }
};


} // namespace deva

#endif // __OBJECT_H__
