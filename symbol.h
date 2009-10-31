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
    sym_function,		// 6	both fcn defs (locations) and return addresses
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

    SymbolInfo() : type( sym_unknown ), is_const( false )
    { }
    SymbolInfo( SymbolType t ) : type( t ), is_const( false )
    { }

	SymbolType Type() const { return type; }
};

struct SymbolTable : public map<string, SymbolInfo>
{
	// also need the id of our parent
	int parent_id;

	SymbolTable() : parent_id( -1 )
	{ }
};
	

// strip the whitespace and leading comments from a string,
// to create a valid symbol name
string strip_symbol( const string& src, const string& c = " \t\r\n" );

// strip single and double quotes (for use in getting the value of a string
// variable, for instance)
string strip_quotes( const string& src );

// "un-escape" a string (e.g. turn '\t' into a tab character etc)
string unescape( const string& src );

// is this identifier a keyword?
bool is_keyword( const string & s );

#endif // __SYMBOL_H__
