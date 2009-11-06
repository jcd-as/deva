// dobject.h
// object type for the deva language intermediate language & virtual machine
// created by jcs, October 03, 2009 

// TODO:
// * reference counting
// * types for C function and UserCode (C "void*")block?
// * type for User-Defined Types (classes) ??

#ifndef __DOBJECT_H__
#define __DOBJECT_H__

#include "opcodes.h"
#include "symbol.h"
#include "types.h"
#include "smart_ptr.h"
#include <vector>

// typedefs for the vector and map types that deva supports
class DevaObject;
typedef vector<DevaObject> DOVector;
typedef map<DevaObject, DevaObject> DOMap;

// the basic piece of data stored on the data stack
struct DevaObject : public SymbolInfo
{
	// TODO: types for C function and UserCode (C "void*")block?
	// TODO: type for User-Defined Types (classes) ??
	union 
	{
		double num_val;
		char* str_val;
		bool bool_val;
//		map<DevaObject, DevaObject>* map_val;
//		vector<DevaObject>* vec_val;
		size_t func_offset;	// offset into instruction stream to function start
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
	// 'offset' type (integral number, incl return/jump target etc)
	DevaObject( string nm, size_t offs );
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

	// size of the object on *disk*
	long Size() const;
};


#endif // __DOBJECT_H__
