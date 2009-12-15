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

// dobject.h
// object type for the deva language intermediate language & virtual machine
// created by jcs, October 03, 2009 

// TODO:
// * 

#ifndef __DOBJECT_H__
#define __DOBJECT_H__

#include <vector>
#include <iostream>

#include "opcodes.h"
#include "symbol.h"
#include "types.h"
#include "smart_ptr.h"

// typedefs for the vector and map types that deva supports
class DevaObject;
typedef vector<DevaObject> DOVector;
typedef map<DevaObject, DevaObject> DOMap;
ostream & operator << ( ostream & os, DevaObject & obj );

// the basic piece of data stored on the data stack
struct DevaObject : public SymbolInfo
{
	union 
	{
		double num_val;
		char* str_val;
		bool bool_val;
		size_t sz_val;
		void* nat_obj_val;
	};
	// objects with destructors not allowed in unions. bleh
	smart_ptr<DOMap> map_val;
	smart_ptr<DOVector> vec_val;

	// name that the object (variable) is referred to with
	// (empty string for constants)
	string name;

	// construction/destruction:
	// default
	DevaObject();
	DevaObject( string nm );
	// number type
	DevaObject( string nm, double n );
	// string type
	DevaObject( string nm, string s );
	// string type, take ownership of the passed in string
	DevaObject( string nm, char* s );
	// boolean type
	DevaObject( string nm, bool b );
	// 'address' type (incl return/jump target etc) or 'size' type (integral
	// number, incl native ptrs etc)
	DevaObject( string nm, size_t offs, bool is_address );
	// 'native object' (C void*) type
	DevaObject( string nm, void* ptr );
    // map type with the given map
    DevaObject( string nm, DOMap* m );
    // vector type with the given vector
    DevaObject( string nm, DOVector* v );
    // given type, empty object
	DevaObject( string nm, SymbolType t );
	// copy constructor needed to ensure each object has a separate copy of data
	DevaObject( const DevaObject & o );
	// copy construct, but with a different name
	DevaObject( string nm, const DevaObject & o );
	~DevaObject();

    // factory method: class type from the given map
    static DevaObject ClassFromMap( string nm, DOMap* m );
    // factory method: instance type from the given map
    static DevaObject InstanceFromMap( string nm, DOMap* m );
	// befriend the factory methods so they have access to private data
    friend DevaObject ClassFromMap( string nm, DOMap* m );
    friend DevaObject InstanceFromMap( string nm, DOMap* m );

	DevaObject& operator = ( const DevaObject & o );
	bool operator < ( const DevaObject & rhs ) const;
	bool operator == ( const DevaObject & rhs ) const;

	// cast operators
	operator const double ();
	operator const int ();
	operator const string ();
	operator const void* ();
	operator const size_t ();
	operator const bool ();

	// size of the object on *disk*
	long Size() const;
};


#endif // __DOBJECT_H__
