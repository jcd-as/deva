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

// object.cpp
// object type for the deva language
// created by jcs, december 18, 2010 

#include "object.h"
#include "exceptions.h"
#include "executor.h"
#include <set>

using namespace std;


namespace deva
{

const char* object_type_names[] = 
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
	"module",
	"native module",
	"<invalid>"
};

// static member of RefCounted
template<typename T> vector<T*> RefCounted<T>::dead_pool = vector<T*>();

Object Object::CreateClass( Map* n )
{
	Object ret( n );
	ret.type = obj_class;
	return ret;
}

Object Object::CreateInstance( Map* n )
{
	Object ret( n );
	ret.type = obj_instance;
	return ret;
}

// helper fcns for reference counting. recursively IncRef/DecRef objects
void IncRef( Object & o )
{
	if( IsVecType( o.type ) )
	{
#ifdef DEBUG
		if( reftrace )
			cout << "IncRef: vector: " << o.v << " prior refcount: " << o.v->GetRefCount() << endl;
#endif 
		o.v->IncRef();
	}
	else if( IsMapType( o.type ) )
	{
#ifdef DEBUG
		if( reftrace )
			cout << "IncRef: map: " << o.m << " prior refcount: " << o.m->GetRefCount() << endl;
#endif 
		o.m->IncRef();
	}
}

// inc ref this object's children
// for use when creating a copy of a reference type object
// (e.g. when creating an instance from a class object)
void IncRefChildren( Object & o )
{
	if( IsVecType( o.type ) )
	{
		// walk the vector's contents
		for( size_t i = 0; i < o.v->size(); i++ )
		{
			Object obj = o.v->operator[]( i );
			IncRef( obj );
		}
	}
	else if( IsMapType( o.type ) )
	{
		// walk the map's contents
		for( Map::iterator it = o.m->begin(); it != o.m->end(); ++it )
		{
			IncRef( const_cast<Object&>(it->first) );
			IncRef( it->second );
		}
	}
}

// dec ref this object's children
// for use when destroying an object
void DecRefChildren( Object & o )
{
	if( IsVecType( o.type ) )
	{
		// walk the vector's contents
		for( size_t i = 0; i < o.v->size(); i++ )
		{
			Object obj = o.v->operator[]( i );
			DecRef( obj );
		}
	}
	else if( IsMapType( o.type ) )
	{
		// walk the map's contents
		for( Map::iterator it = o.m->begin(); it != o.m->end(); ++it )
		{
			DecRef( const_cast<Object&>(it->first) );
			DecRef( it->second );
		}
	}
}

int DecRef( Object & o )
{
	if( IsVecType( o.type ) )
	{
		if( !o.v )
			return 0;
		// if the vector is about to be destroyed, release its refs on its contents
		if( o.v->GetRefCount() == 1 )
		{
			DecRefChildren( o );
		}
#ifdef DEBUG
		if( reftrace )
			cout << "DecRef: vector: " << o.v << " prior refcount: " << o.v->GetRefCount() << endl;
#endif 
		int ret = o.v->DecRef();
		if( ret == 0 )
			o.v = NULL;
		return ret;
	}
	else if( IsMapType( o.type ) )
	{
		if( !o.m )
			return 0;
		// if the object is going to be destroyed
		if( o.m->GetRefCount() == 1 )
		{
			// if we're deleting an instance, we need to call the destructor and base class destructors
			if( o.type == obj_instance )
				ex->CallDestructors( o );

			// release its refs on its contents
			DecRefChildren( o );
		}
#ifdef DEBUG
		if( reftrace )
			cout << "DecRef: map: " << o.m << " prior refcount: " << o.m->GetRefCount() << endl;
#endif 
		int ret = o.m->DecRef();
		if( ret == 0 )
			o.m = NULL;
		return ret;
	}
	// non-ref-type
	return 0;
}

// equality operator
bool Object::operator == ( const Object& rhs ) const
{
	if( type != rhs.type )
		return false;
	switch( type )
	{
	case obj_null:
		return true;
	case obj_number:
		if( d == rhs.d )
			return true;
		break;
	case obj_string:
		if( strcmp( s, rhs.s ) == 0 )
			return true;
		break;
	case obj_boolean:
		if( b == rhs.b )
			return true;
		break;
	case obj_vector:
		if( v == rhs.v )
			return true;
		break;
	case obj_map:
	case obj_instance:
	case obj_class:
		if( m == rhs.m )
			return true;
		break;
	case obj_function:
		if( f == rhs.f )
			return true;
		break;
	case obj_native_function:
		if( nf.p == rhs.nf.p )
			return true;
		break;
	case obj_native_obj:
		if( no == rhs.no )
			return true;
		break;
	case obj_size:
		if( sz == rhs.sz )
			return true;
		break;
	case obj_symbol_name:
		if( strcmp( s, rhs.s ) == 0 )
			return true;
		break;
	case obj_module:
		if( mod == rhs.mod )
			return true;
		break;
	case obj_native_module:
		if( nm == rhs.nm )
			return true;
		break;
	default:
		// ???
		break;
	}
	return false;
}

// less-than operator
bool Object::operator < ( const Object & rhs ) const
{
	if( type != rhs.type )
		return type < rhs.type;
	else
	{
		switch( type )
		{
		case obj_null:
			return false;
		case obj_number:
			return d < rhs.d;
		case obj_symbol_name:
		case obj_string:
			return strcmp( s, rhs.s ) < 0;
		case obj_boolean:
			return b < rhs.b;
		case obj_vector:
			return v < rhs.v;
		case obj_map:
		case obj_instance:
		case obj_class:
			return m < rhs.m;
		case obj_function:
			return f < rhs.f;
		case obj_native_function:
			return nf.p < rhs.nf.p;
		case obj_native_obj:
			return no < rhs.no;
		case obj_size:
			return sz < rhs.sz;
		case obj_module:
			return mod < rhs.mod;
		case obj_native_module:
			return nm < rhs.nm;
		case obj_end:
			throw ICE( "Invalid object type (obj_end) in Object::operator <." );
		default:
			throw ICE( "Unknown object type in Object::operator <." );
		}
	}
	return false;
}

// coerce to a boolean (for jmpf op, for instance)
bool Object::CoerceToBool()
{
	switch( type )
	{
	case obj_number:
		if( d != 0 )
			return true;
		break;
	case obj_boolean:
		return (b != 0);
		break;
	case obj_native_obj:
		if( no != (void*)0 )
			return true;
		break;
	case obj_native_function:
		if( nf.p != (void*)0 )
			return true;
		break;
	case obj_size:
		if( sz != 0 )
			return true;
		break;
	case obj_string:
		if( strlen( s ) > 0 )
			return true;
		break;
	case obj_vector:
	case obj_map:
	case obj_instance:
	case obj_class:
	case obj_function:
	case obj_module:
	case obj_native_module:
	case obj_symbol_name: // technically, this shouldn't happen
		return true;
		break;
	case obj_null:
	default:
		break;
	}
	return false;
}

ostream & operator << ( ostream & os, const Object & obj )
{
	static bool prettify_strings = false;

	switch( obj.type )
	{
		case obj_number:
			os << obj.d;
			break;
		case obj_string:
			if( prettify_strings )
				os << "'" << obj.s << "'";
			else
				os << obj.s ;
			break;
		case obj_boolean:
			if( obj.b )
				os << "true";
			else
				os << "false";
			break;
		case obj_null:
			os << "null";
			break;
		case obj_map:
			{
			// dump map contents
			os << "{";
			for( Map::iterator it = obj.m->begin(); it != obj.m->end(); )
			{
				Object key = (*it).first;
				Object val = (*it).second;
				prettify_strings = true;
				os << key << ":";
				prettify_strings = true;
				os << val;
				prettify_strings = false;
				if( ++it != obj.m->end() )
					os << ", ";
			}
			os << "}";
			break;
			}
		case obj_vector:
			{
			// dump vector contents
			os << "[";
			for( size_t i = 0; i < obj.v->size(); i++ )
			{
				Object val = obj.v->at( i );
				prettify_strings = true;
				os << val;
				prettify_strings = false;
			   	if( i+1 != obj.v->size() )
					os << ", ";
			}
			os << "]";
			break;
			}
		case obj_size:
			os << "address/sized-value = " << (size_t)obj.sz;
			break;
		case obj_symbol_name:
			os << obj.s ;
			break;
		case obj_native_obj:
			os << "native object = " << (void*)obj.no;
			break;
		case obj_function:
			// TODO:
			os << "function = " << (void*)obj.f;
			break;
		case obj_native_function:
			// TODO:
			if( obj.nf.is_method )
				os << "native method = " << (void*)obj.nf.p;
			else
				os << "native function = " << (void*)obj.nf.p;
			break;
		case obj_class:
			os << "class: ";
			// dump map contents
			os << "{";
			for( Map::iterator it = obj.m->begin(); it != obj.m->end(); )
			{
				Object key = (*it).first;
				Object val = (*it).second;
				prettify_strings = true;
				os << key << ":";
				prettify_strings = true;
				os << val;
				prettify_strings = false;
				if( ++it != obj.m->end() )
					os << ", ";
			}
			os << "}";
			break;
		case obj_instance:
			os << "instance: ";
			// dump map contents
			os << "{";
			for( Map::iterator it = obj.m->begin(); it != obj.m->end(); )
			{
				Object key = (*it).first;
				Object val = (*it).second;
				prettify_strings = true;
				os << key << ":";
				prettify_strings = true;
				os << val;
				prettify_strings = false;
				if( ++it != obj.m->end() )
					os << ", ";
			}
			os << "}";
			break;
		case obj_module:
			os << "module = " << (void*)obj.mod;
			break;
		case obj_native_module:
			os << "native module = " << (void*)obj.nm;
			break;
		default:
			os << "ERROR: unknown type";
	}
	return os;
}

// operator << for printing ObjectTypes
ostream & operator << ( ostream & os, ObjectType t )
{
	switch( t )
	{
	case obj_null:
		os << "null";
		break;
	case obj_boolean:
		os << "boolean";
		break;
	case obj_number:
		os << "number";
		break;
	case obj_string:
		os << "string";
		break;
	case obj_vector:
		os << "vector";
		break;
	case obj_map:
		os << "map";
		break;
	case obj_function:
		os << "function";
		break;
	case obj_native_function:
		os << "native function";
		break;
	case obj_class:
		os << "class";
		break;
	case obj_instance:
		os << "object";
		break;
	case obj_native_obj:
		os << "native object";
		break;
	case obj_size:
		os << "size/address";
		break;
	case obj_symbol_name:
		os << "symbol name";
		break;
	case obj_module:
		os << "module";
		break;
	case obj_native_module:
		os << "native module";
		break;
	case obj_end:
	default:
		os << "<invalid>";
		break;
	}
	return os;
}


} // namespace deva
