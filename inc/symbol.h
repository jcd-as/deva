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
    sym_offset,			// 6	a size_t number/offset (fcn defs (locations), return addresses etc)
    sym_function_call,	// 7
    sym_unknown,		// 8	a variable (unknown type, has to be looked-up)
	sym_class,			// 9	a class def
	sym_instance,		// 10	a class instance (object)
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
