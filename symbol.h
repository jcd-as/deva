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
    sym_null,			// 0
    sym_number,			// 1
    sym_string,			// 2
	sym_boolean,		// 3
    sym_vector,			// 4
	sym_map,			// 5
    sym_function,		// 6
    sym_function_call,	// 7
    sym_unknown,		// 8
	sym_end = 255		// end of enum and signal for end of instruction arg list
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
