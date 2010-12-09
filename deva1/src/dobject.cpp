// Copyright (c) 2009 Joshua C. Shepard
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

// dobject.cpp
// object type for the deva language intermediate language & virtual machine
// created by jcs, October 03, 2009 

// TODO:
// * 

#include "dobject.h"
#include <cstring>

DevaObject::DevaObject() : SymbolInfo( sym_null ), name( "" ), map_val( 0 ), vec_val( 0 )
{ }

// copy constructor needed to ensure each object has a separate copy of data
DevaObject::DevaObject( const DevaObject & o ) : map_val( 0 ), vec_val( 0 )
{
	if( &o == this )
		return;
	name = o.name;
	type = o.type;
	is_const = o.is_const;
	switch( type )
	{
	case sym_number:
		num_val = o.num_val;
		break;
	case sym_string:
		str_val = new char[strlen( o.str_val ) + 1];
		strcpy( str_val, o.str_val );
		break;
	case sym_boolean:
		bool_val = o.bool_val;
		break;
	case sym_class:
	case sym_instance:
	case sym_map:
		map_val = o.map_val;
		break;
	case sym_vector:
		vec_val = o.vec_val;
		break;
	case sym_address:
	case sym_size:
		sz_val = o.sz_val;
		break;
	case sym_native_obj:
		nat_obj_val = o.nat_obj_val;
		break;
	case sym_function_call:
		// TODO: anything???
		break;
	case sym_null:
		// nothing to do, null has/needs no value
	case sym_unknown:
		// nothing to do, no known value/type
		break;
	default:
		// TODO: throw error
		break;
	}

}
// copy construct, but with a different name
DevaObject::DevaObject( string nm, const DevaObject & o ) : map_val( 0 ), vec_val( 0 )
{
	if( &o == this )
		return;
	name = nm;
	type = o.type;
	is_const = o.is_const;
	switch( type )
	{
	case sym_number:
		num_val = o.num_val;
		break;
	case sym_string:
		str_val = new char[strlen( o.str_val ) + 1];
		strcpy( str_val, o.str_val );
		break;
	case sym_boolean:
		bool_val = o.bool_val;
		break;
	case sym_class:
	case sym_instance:
	case sym_map:
		map_val = o.map_val;
		break;
	case sym_vector:
		vec_val = o.vec_val;
		break;
	case sym_address:
	case sym_size:
		sz_val = o.sz_val;
		break;
	case sym_native_obj:
		nat_obj_val = o.nat_obj_val;
		break;
	case sym_function_call:
		// TODO: anything???
		break;
	case sym_null:
		// nothing to do, null has/needs no value
	case sym_unknown:
		// nothing to do, no known value/type
		break;
	default:
		// TODO: throw error
		break;
	}

}
// empty
DevaObject::DevaObject( string nm ) : SymbolInfo( sym_end ), name( nm ), map_val( 0 ), vec_val( 0 )
{}
// number type
DevaObject::DevaObject( string nm, double n ) : SymbolInfo( sym_number ), num_val( n ), map_val( 0 ), vec_val( 0 )
{}
// string type
DevaObject::DevaObject( string nm, string s ) : SymbolInfo( sym_string ), name( nm ), map_val( 0 ), vec_val( 0 )
{
	// make a copy of the string passed to us, DON'T take ownership of it!
	str_val = new char[s.size() + 1];
	strcpy( str_val, s.c_str() );
}
// C string type, _take_ownership_ of the passed in string!!
DevaObject::DevaObject( string nm, char* s ) : SymbolInfo( sym_string ), name( nm ), map_val( 0 ), vec_val( 0 )
{
	str_val = s;
}
// boolean type
DevaObject::DevaObject( string nm, bool b ) : SymbolInfo( sym_boolean ), bool_val( b ), name( nm ), map_val( 0 ), vec_val( 0 )
{}
// 'address' or 'size' type
DevaObject::DevaObject( string nm, size_t offs, bool is_address ) : SymbolInfo( sym_address ), sz_val( offs ), name( nm ), map_val( 0 ), vec_val( 0 )
{ if( !is_address ) type = sym_size; }
// 'native object' (C void*) type
DevaObject::DevaObject( string nm, void* ptr ) : SymbolInfo( sym_native_obj ), nat_obj_val( ptr ), name( nm ), map_val( 0 ), vec_val( 0 )
{}
// map type with the given map
DevaObject::DevaObject( string nm, DOMap* m ) : SymbolInfo( sym_map ), name( nm ), map_val( m ), vec_val( 0 )
{}
// vector type with the given vector
DevaObject::DevaObject( string nm, DOVector* v ) : SymbolInfo( sym_vector ), name( nm ), map_val( 0 ), vec_val( v )
{}
// given type, empty/default object
DevaObject::DevaObject( string nm, SymbolType t ) : map_val( 0 ), vec_val( 0 )
{
	type = t;
	name = nm;
	is_const = false;
	switch( t )
	{
	case sym_number:
		num_val = 0.0;
		break;
	case sym_string:
		str_val = NULL;
		break;
	case sym_boolean:
		bool_val = false;
		break;
	case sym_map:
	case sym_class:
	case sym_instance:
		{
		DOMap* m = new DOMap();
		map_val = m;
		break;
		}
	case sym_vector:
		{
		DOVector* v = new DOVector();
		vec_val = v;
		break;
		}
	case sym_address:
	case sym_size:
		sz_val = -1;
		break;
	case sym_native_obj:
		nat_obj_val = NULL;
		break;
	case sym_function_call:
		// TODO: anything???
		break;
	case sym_null:
		// nothing to do, null has/needs no value
	case sym_unknown:
		// nothing to do, no known value/type
		break;
	default:
		// TODO: throw error, invalid
		break;
	}
}
DevaObject::~DevaObject()
{
	if( type == sym_string )
	{
		if( str_val ) delete [] str_val;
	}
}

// factory method: class type from the given map
/*static*/ DevaObject DevaObject::ClassFromMap( string nm, DOMap* m )
{
	DevaObject obj( nm, m );
	obj.type = sym_class;
	return obj;
}

// factory method: instance type from the given map
/*static*/ DevaObject DevaObject::InstanceFromMap( string nm, DOMap* m )
{
	DevaObject obj( nm, m );
	obj.type = sym_instance;
	return obj;
}


DevaObject & DevaObject::operator = ( const DevaObject & o )
{
	if( &o == this )
		return *this;

	if( type == sym_string )
	{
		// free the old string, if any
		if( str_val ) delete [] str_val;
	}
	name = o.name;
	type = o.type;
	is_const = o.is_const;

	switch( type )
	{
	case sym_number:
		num_val = o.num_val;
		break;
	case sym_string:
		str_val = new char[strlen( o.str_val ) + 1];
		strcpy( str_val, o.str_val );
		break;
	case sym_boolean:
		bool_val = o.bool_val;
		break;
	case sym_class:
	case sym_instance:
	case sym_map:
		map_val = o.map_val;
		break;
	case sym_vector:
		vec_val = o.vec_val;
		break;
	case sym_address:
	case sym_size:
		sz_val = o.sz_val;
		break;
	case sym_native_obj:
		nat_obj_val = o.nat_obj_val;
		break;
	case sym_function_call:
		// TODO: anything???
		break;
	case sym_null:
		// nothing to do, null has/needs no value
	case sym_unknown:
		// nothing to do, no known value/type
		break;
	default:
		// TODO: throw error
		break;
	}
	return *this;
}

// needed to use DevaObjects as keys in std::map
bool DevaObject::operator < ( const DevaObject & rhs ) const
{
	if( type < rhs.type )
		return true;
	else if( type > rhs.type )
		return false;
	// types are the same, compare
	switch( type )
	{
	case sym_number:
		if( num_val < rhs.num_val )
			return true;
		else
			return false;
	case sym_string:
		if( strcmp( str_val, rhs.str_val ) < 0 )
			return true;
		else
			return false;
	case sym_boolean:
		if( bool_val == true )
			return true;
		else
			return false;
	case sym_class:
	case sym_instance:
	case sym_map:
		if( name.compare( rhs.name ) < 0 )
			return true;
		else
			return false;
	case sym_vector:
		if( name.compare( rhs.name ) < 0 )
			return true;
		else
			return false;
	case sym_address:
	case sym_size:
		if( sz_val < rhs.sz_val )
			return true;
		else
			return false;
	case sym_native_obj:
		if( nat_obj_val < rhs.nat_obj_val )
			return true;
		else
			return false;
	case sym_function_call:
		if( name.compare( rhs.name ) < 0 )
			return true;
		else
			return false;
	case sym_null:
		if( name.compare( rhs.name ) < 0 )
			return true;
		else
			return false;
	case sym_unknown:
		if( name.compare( rhs.name ) < 0 )
			return true;
		else
			return false;
	default:
		// TODO: throw error
		return true;
	}
}

bool DevaObject::operator == ( const DevaObject & rhs ) const
{
	// can't be the same if the types are different
	if( type != rhs.type )
		return false;
	// types are the same, compare
	switch( type )
	{
	case sym_number:
		// TODO: need to use epsilon or some other more robust method to compare
		// floating point numbers
		if( num_val == rhs.num_val )
			return true;
		else
			return false;
	case sym_string:
		return strcmp( str_val, rhs.str_val ) == 0;
	case sym_boolean:
		return bool_val == rhs.bool_val;
	case sym_class:
	case sym_instance:
	case sym_map:
		// TODO: deep compare instead of "is" pointer compare??
		return (void*)map_val == (void*)rhs.map_val;
	case sym_vector:
		// TODO: deep compare instead of "is" pointer compare??
		return (void*)vec_val == (void*)rhs.vec_val;
	case sym_address:
	case sym_size:
		return sz_val == rhs.sz_val;
	case sym_native_obj:
		return nat_obj_val == rhs.nat_obj_val;
	case sym_function_call:
		// TODO: ???
		return false;
	case sym_null:
		// null always equal to null
		return true;
	case sym_unknown:
		// TODO: deep compare?? name compare?? what does this mean??
		return name == rhs.name;
	default:
		// TODO: throw error
		throw DevaICE( "Unknown type in DevaObject equality comparison." );
	}
}


// cast operators
DevaObject::operator const double ()
{
	if( type != sym_number )
		throw DevaInvalidCast( "Object cannot be cast to double." );
	else
		return num_val;
}

DevaObject::operator const int ()
{
	if( type != sym_number )
		throw DevaInvalidCast( "Object cannot be cast to integer." );
	else
		return (int)num_val;
}

DevaObject::operator const string ()
{
	if( type != sym_string )
		throw DevaInvalidCast( "Object cannot be cast to string." );
	else
		return string( str_val );
}

DevaObject::operator const void* ()
{
	if( type != sym_native_obj )
		throw DevaInvalidCast( "Object cannot be cast to native value." );
	else
		return nat_obj_val;
}

DevaObject::operator const size_t ()
{
	if( type != sym_size && type != sym_address )
		throw DevaInvalidCast( "Object cannot be cast to size or address." );
	else
		return sz_val;
}

DevaObject::operator const bool ()
{
	if( type != sym_boolean )
		throw DevaInvalidCast( "Object cannot be cast to boolean." );
	else
		return bool_val;
}

// size of the object on *disk*
long DevaObject::Size() const
{
	// length of name (plus null terminator), plus 'type' byte
	long sz = name.length() + 2;
	switch( type )
	{
	case sym_number:
		return sz + sizeof( double );
	case sym_string:
		return sz + strlen( str_val ) + 1;
	case sym_boolean:
		return sz + sizeof( long );
	case sym_class:
	case sym_instance:
	case sym_map:
		throw logic_error( "Internal Error. Can't take the size of a map-based object." );
	case sym_vector:
		throw logic_error( "Internal Error. Can't take the size of a vector object." );
	case sym_address:
	case sym_size:
		return sz + sizeof( size_t );
	case sym_native_obj:
		return sz + sizeof( void* );
	case sym_null:
		return sz + sizeof( long );
	case sym_unknown:
	case sym_function_call:
	default:
		return sz;
	}
}

// operator to dump an DevaObject to an iostreams stream
bool prettify_for_output = false;
ostream & operator << ( ostream & os, DevaObject & obj )
{
	static bool prettify_strings = false;
	switch( obj.Type() )
	{
		case sym_number:
			os << obj.num_val;
			break;
		case sym_string:
			if( prettify_strings )
				os << "'" << obj.str_val << "'";
			else
				os << obj.str_val ;
			break;
		case sym_boolean:
			if( obj.bool_val )
				os << "true";
			else
				os << "false";
			break;
		case sym_null:
			os << "null";
			break;
		case sym_map:
			{
			// dump map contents
			os << "{";
			smart_ptr<DOMap> mp( obj.map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				prettify_strings = true;
				os << key << ":";
				prettify_strings = true;
				os << val;
				prettify_strings = false;
				if( ++it != mp->end() )
					os << ", ";
			}
			os << "}";
			break;
			}
		case sym_vector:
			{
			// dump vector contents
			os << "[";
			smart_ptr<DOVector> vec( obj.vec_val );
			for( DOVector::iterator it = vec->begin(); it != vec->end(); ++it )
			{
				DevaObject val = (*it);
				prettify_strings = true;
				os << val;
				prettify_strings = false;
			   	if( it+1 != vec->end() )
					os << ", ";
			}
			os << "]";
			break;
			}
		case sym_address:
		case sym_size:
			os << obj.name << ", address/sized-value = " << obj.sz_val;
			break;
		case sym_native_obj:
			os << obj.name << ", native object = " << obj.nat_obj_val;
			break;
		case sym_function_call:
			os << "function_call: '" << obj.name << "'";
			break;
		case sym_unknown:
			os << "unknown: '" << obj.name << "'";
			break;
		case sym_class:
			os << "class: '" << obj.name << "' = {";
			{
			// dump map contents
			smart_ptr<DOMap> mp( obj.map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				prettify_strings = true;
				os << key << ":";
				prettify_strings = true;
				os << val;
				prettify_strings = false;
				if( ++it != mp->end() )
					os << ", ";
			}
			os << "}";
			break;
			}
		case sym_instance:
			os << "instance: '" << obj.name << "' = {";
			{
			// dump map contents
			smart_ptr<DOMap> mp( obj.map_val );
			for( DOMap::iterator it = mp->begin(); it != mp->end(); )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				prettify_strings = true;
				os << key << ":";
				prettify_strings = true;
				os << val;
				prettify_strings = false;
				if( ++it != mp->end() )
					os << ", ";
			}
			os << "}";
			break;
			}
		default:
			os << "ERROR: unknown type";
	}
	return os;
}

