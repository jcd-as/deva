// symbol.h
// deva language symbol information
// created by jcs, august 29, 2009

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <map>
#include <string>

using namespace std;

enum SymbolType
{
    sym_null,
    sym_number,
    sym_string,
	sym_boolean,
    sym_vector,
	sym_map,
    sym_function,
    sym_unknown
};

class SymbolInfo
{
protected:
    SymbolType type;

public:
    // is this a constant?
	bool is_const;
	// is this a function argument?
	bool is_argument;

    SymbolInfo() : type( sym_unknown ), is_const( false ), is_argument( false )
    { }
    SymbolInfo( SymbolType t ) : type( t ), is_const( false ), is_argument( false )
    { }

	SymbolType Type(){ return type; }
};

struct SymbolTable : public map<string, SymbolInfo*>
{
	// also need the id of our parent
	int parent_id;

	SymbolTable() : parent_id( -1 )
	{ }
	~SymbolTable()
	{
		// delete all the SymbolInfo ptrs
		for( map<string, SymbolInfo*>::iterator i = begin(); i != end(); ++i )
		{
			delete i->second;
		}
	}
};
	

// strip the whitespace and leading comments from a string,
// to create a valid symbol name
string strip_symbol( const string& src, const string& c = " \t\r\n" );

// is this identifier a keyword?
bool is_keyword( const string & s );

#endif // __SYMBOL_H__
