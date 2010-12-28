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


namespace deva
{

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
		if( nf == rhs.nf )
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
			return true; // ???
		case obj_number:
			return d < rhs.d;
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
			return nf < rhs.nf;
		case obj_native_obj:
			return no < rhs.no;
		case obj_size:
			return sz < rhs.sz;
		case obj_symbol_name:
			return strcmp( s, rhs.s ) < 0;
		}
	}
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
			return b;
		break;
	case obj_native_obj:
	case obj_native_function:
		if( no != (void*)0 )
			return true;
		break;
	case obj_size:
		if( sz != 0 )
			return true;
		break;
	case obj_string:
	case obj_vector:
	case obj_map:
	case obj_instance:
	case obj_class:
	case obj_function:
	case obj_symbol_name: // technically, this shouldn't happen
		return true;
		break;
	case obj_null:
	default:
		break;
	}
	return false;
}

ostream & operator << ( ostream & os, Object & obj )
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
			os << "map";
			// TODO: dump map contents
//			os << "{";
//			smart_ptr<DOMap> mp( obj.map_val );
//			for( DOMap::iterator it = mp->begin(); it != mp->end(); )
//			{
//				Object key = (*it).first;
//				Object val = (*it).second;
//				prettify_strings = true;
//				os << key << ":";
//				prettify_strings = true;
//				os << val;
//				prettify_strings = false;
//				if( ++it != mp->end() )
//					os << ", ";
//			}
//			os << "}";
			break;
			}
		case obj_vector:
			{
			// dump vector contents
			os << "[";
			for( int i = 0; i < obj.v->size(); i++ )
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
			os << "native function = " << (void*)obj.nf;
			break;
		case obj_class:
			os << "class";
//			os << "class: '" << obj.name << "' = {";
//			{
//			// TODO: dump map contents
//			smart_ptr<DOMap> mp( obj.map_val );
//			for( DOMap::iterator it = mp->begin(); it != mp->end(); )
//			{
//				Object key = (*it).first;
//				Object val = (*it).second;
//				prettify_strings = true;
//				os << key << ":";
//				prettify_strings = true;
//				os << val;
//				prettify_strings = false;
//				if( ++it != mp->end() )
//					os << ", ";
//			}
//			os << "}";
			break;
//			}
		case obj_instance:
			os << "instance";
//			os << "instance: '" << obj.name << "' = {";
//			{
//			// TODO: dump map contents
//			smart_ptr<DOMap> mp( obj.map_val );
//			for( DOMap::iterator it = mp->begin(); it != mp->end(); )
//			{
//				Object key = (*it).first;
//				Object val = (*it).second;
//				prettify_strings = true;
//				os << key << ":";
//				prettify_strings = true;
//				os << val;
//				prettify_strings = false;
//				if( ++it != mp->end() )
//					os << ", ";
//			}
//			os << "}";
			break;
//			}
		default:
			os << "ERROR: unknown type";
	}
	return os;
}

} // namespace deva
