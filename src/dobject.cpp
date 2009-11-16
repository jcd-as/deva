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
	case sym_offset:
		func_offset = o.func_offset;
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
	case sym_offset:
		func_offset = o.func_offset;
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
// string type, _take_ownership_ of the passed in string!!
DevaObject::DevaObject( string nm, char* s ) : SymbolInfo( sym_string ), name( nm ), map_val( 0 ), vec_val( 0 )
{
	str_val = s;
}
// boolean type
DevaObject::DevaObject( string nm, bool b ) : SymbolInfo( sym_boolean ), bool_val( b ), name( nm ), map_val( 0 ), vec_val( 0 )
{}
// 'function' type
DevaObject::DevaObject( string nm, size_t offs ) : SymbolInfo( sym_offset ), func_offset( offs ), name( nm ), map_val( 0 ), vec_val( 0 )
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
	case sym_offset:
		func_offset = -1;
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
		if( str_val ) delete [] str_val;
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
	case sym_offset:
		func_offset = o.func_offset;
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
	case sym_offset:
		if( func_offset < rhs.func_offset )
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
	case sym_offset:
		return func_offset == rhs.func_offset;
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
	case sym_offset:
		return sz + sizeof( size_t );
	case sym_null:
		return sz + sizeof( long );
	case sym_unknown:
	case sym_function_call:
	default:
		return sz;
	}
}

