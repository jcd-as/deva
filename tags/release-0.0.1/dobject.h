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
#include <vector>

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
		map<DevaObject, DevaObject>* map_val;
		vector<DevaObject>* vec_val;
		long func_offset;	// offset into instruction stream to function start
	};

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
	// boolean type
	DevaObject( string nm, bool b );
	// 'function' (incl return) type
	DevaObject( string nm, long offs );
	DevaObject( string nm, SymbolType t );
	// copy constructor needed to ensure each object has a separate copy of data
	DevaObject( const DevaObject & o );
	// copy construct, but with a different name
	DevaObject( string nm, const DevaObject & o );
	~DevaObject();

	DevaObject& operator = ( const DevaObject & o );
	bool operator < ( const DevaObject & rhs ) const;

	// equivalent of destroying and re-creating
	void ChangeType( SymbolType t );
	// set the value from another object, without changing the name
	// fails (returns false) if this is a const object
	bool SetValue( const DevaObject & o );
	// size of the object on *disk*
	long Size() const;
};


#endif // __DOBJECT_H__
