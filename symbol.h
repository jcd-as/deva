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
    sym_vector,
	sym_map,
    sym_function,
    sym_unknown
};

struct SymbolInfo
{
    SymbolType type;
    // TODO: what else do we need?

    SymbolInfo() : type( sym_null )
    { }
    SymbolInfo( SymbolType t ) : type( t )
    { }
};

//typedef map<string, SymbolInfo> SymbolTable;
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

// is this identifier a keyword?
bool is_keyword( const string & s );

#endif // __SYMBOL_H__
