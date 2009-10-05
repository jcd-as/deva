// dobject.cpp
// object type for the deva language intermediate language & virtual machine
// created by jcs, October 03, 2009 

// TODO:
// * reference counting

#include "dobject.h"
#include <cstring>

DevaObject::DevaObject() : SymbolInfo( sym_null ), name( "" )
{ }

// copy constructor needed to ensure each object has a separate copy of data
DevaObject::DevaObject( const DevaObject & o )
{
	if( &o == this )
		return;
	name = o.name;
	type = o.type;
	is_const = o.is_const;
	is_argument = o.is_argument;
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
	case sym_map:
		// TODO: verify this
		map_val = new map<DevaObject, DevaObject>();
		*map_val = *(o.map_val);
		break;
	case sym_vector:
		// TODO: verify this
		vec_val = new vector<DevaObject>();
		*vec_val = *(o.vec_val);
		break;
	case sym_function:
		// TODO: any better default value for fcn types?
		// TODO: need to create copy??
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
DevaObject::DevaObject( string nm, const DevaObject & o )
{
	if( &o == this )
		return;
	name = nm;
	type = o.type;
	is_const = o.is_const;
	is_argument = o.is_argument;
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
	case sym_map:
		// TODO: verify this
		map_val = new map<DevaObject, DevaObject>();
		*map_val = *(o.map_val);
		break;
	case sym_vector:
		// TODO: verify this
		vec_val = new vector<DevaObject>();
		*vec_val = *(o.vec_val);
		break;
	case sym_function:
		// TODO: any better default value for fcn types?
		// TODO: need to create copy??
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
DevaObject::DevaObject( string nm ) : SymbolInfo( sym_end ), name( nm )
{}
DevaObject::DevaObject( string nm, double n ) : SymbolInfo( sym_number ), num_val( n ), name( nm )
{}
DevaObject::DevaObject( string nm, string s ) : SymbolInfo( sym_string ), name( nm )
{
	// make a copy of the string passed to us, DON'T take ownership of it!
	str_val = new char[s.size() + 1];
	strcpy( str_val, s.c_str() );
}
DevaObject::DevaObject( string nm, bool b ) : SymbolInfo( sym_boolean ), bool_val( b ), name( nm )
{}
DevaObject::DevaObject( string nm, long offs ) : SymbolInfo( sym_function ), func_offset( offs ), name( nm )
{}
DevaObject::DevaObject( string nm, SymbolType t )
{
	type = t;
	name = nm;
	is_const = false;
	is_argument = false;
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
		map_val = new map<DevaObject, DevaObject>();
		break;
	case sym_vector:
		vec_val = new vector<DevaObject>();
		break;
	case sym_function:
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
	else if( type == sym_map )
		if( map_val ) delete map_val;
	else if( type == sym_vector )
		if( vec_val ) delete vec_val;
}

DevaObject & DevaObject::operator = ( const DevaObject & o )
{
	if( &o == this )
		return *this;
	name = o.name;
	type = o.type;
	is_const = o.is_const;
	is_argument = o.is_argument;
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
	case sym_map:
		// TODO: verify this
		map_val = new map<DevaObject, DevaObject>();
		*map_val = *(o.map_val);
		break;
	case sym_vector:
		// TODO: verify this
		vec_val = new vector<DevaObject>();
		*vec_val = *(o.vec_val);
		break;
	case sym_function:
		// TODO: any better default value for fcn types?
		// TODO: need to create copy??
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
	case sym_function:
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

// equivalent of destroying and re-creating
// TODO: should this be done by creating new object and make this class
// immutable?? how does this work with ref-counting??
void DevaObject::ChangeType( SymbolType t )
{
	if( type == sym_string )
		delete [] str_val;
	else if( type == sym_map )
		delete map_val;
	else if( type == sym_vector )
		delete vec_val;

	type = t;

	switch( t )
	{
	case sym_number:
		num_val = 0.0;
		break;
	case sym_string:
		// TODO: any better default value for string types?
		str_val = NULL;
		break;
	case sym_boolean:
		bool_val = false;
		break;
	case sym_map:
		map_val = new map<DevaObject, DevaObject>();
		break;
	case sym_vector:
		vec_val = new vector<DevaObject>();
		break;
	case sym_function:
		func_offset = -1;
		break;
	case sym_null:
		// nothing to do, null has/needs no value
	case sym_unknown:
		// nothing to do, no known value/type
		break;
	case sym_function_call:
	default:
		// TODO: throw error, can't change type
		break;
	}
}
// set the value from another object, without changing the name
// fails (returns false) if this is a const object
bool DevaObject::SetValue( const DevaObject & o )
{
	if( is_const )
		return false;

	ChangeType( o.type );

	switch( type )
	{
	case sym_number:
		num_val = o.num_val;
		break;
	case sym_string:
		// TODO: any better default value for string types?
		// make a copy of the string passed to us, DON'T take ownership of it!
		str_val = new char[strlen( o.str_val ) + 1];
		strcpy( str_val, o.str_val );
		break;
	case sym_boolean:
		bool_val = o.bool_val;
		break;
	case sym_map:
		map_val = new map<DevaObject, DevaObject>();
		// TODO: copy the map
		break;
	case sym_vector:
		vec_val = new vector<DevaObject>();
		// TODO: copy the vector
		break;
	case sym_function:
		func_offset = o.func_offset;
		break;
	case sym_null:
		// nothing to do, null has/needs no value
	case sym_unknown:
		// nothing to do, variable with no known value
		break;
	case sym_function_call:
	default:
		// TODO: throw error, can't change
		break;
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
	case sym_map:
		// TODO: implement
		return 0;
	case sym_vector:
		// TODO: implement
		return 0;
	case sym_function:
		return sz + sizeof( long );
	case sym_null:
		return sz + sizeof( long );
	case sym_unknown:
	case sym_function_call:
	default:
		return sz;
	}
}

