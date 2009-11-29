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

// symbol.cpp
// deva language symbol information
// created by jcs, september 9, 2009

#include "symbol.h"
#include <cstring>


// strip the whitespace and leading comments from a string,
// to create a valid symbol name
string strip_symbol( const string& src, const string& c )
{
	// if src starts with a comment character ('#'), remove from that up to the
	// first nl/cr
	string src2;
	int comment_pos = src.find_first_of( "#" );
	if( comment_pos != string::npos )
	{
		int first_nl_pos = src.find_first_of( "\r\n", comment_pos + 1 );
		if( first_nl_pos != string::npos )
		{
			src2 = src.substr( first_nl_pos + 1, string::npos );
		}
		else src2 = src;
	}
	else src2 = src;
 	int p2 = src2.find_last_not_of( c );
 	if( p2 == string::npos ) 
		return string();
	int p1 = src2.find_first_not_of( c );
	if( p1 == string::npos )
		p1 = 0;
	return src2.substr( p1, (p2 - p1) + 1 );
}

// strip single and double quotes (for use in getting the value of a string
// variable, for instance)
string strip_quotes( const string& src )
{
	string c( "\"'" );

 	int p2 = src.find_last_not_of( c );
 	if( p2 == string::npos ) 
		return string();
	int p1 = src.find_first_not_of( c );
	if( p1 == string::npos )
		p1 = 0;
	return src.substr( p1, (p2 - p1) + 1 );
}

// helper to locate instances of 'in' in the string 'src', and replace them with
// 'out'
void replace( string& src, const char* const in, const char* const out )
{
    int len = strlen( in );
    size_t pos = src.find( in );
    while( pos != string::npos )
    {
        // found 'in' at 'i'. replace with 'out'
        src = src.replace( pos, len, out );
        pos = src.find( in );
    }
}

// "un-escape" a string (e.g. turn '\t' into a tab character etc)
string unescape( const string& src )
{
    string ret( src );
    replace( ret, "\\t", "\t" );
    replace( ret, "\\n", "\n" );
    replace( ret, "\\r", "\r" );
    replace( ret, "\\\\", "\\" );
    replace( ret, "\\\"", "\"" );
    replace( ret, "\\'", "\'" );
    return ret;
}

// is this identifier a keyword?
bool is_keyword( const string & s )
{
	if( s == "null" 
		|| s == "true"
		|| s == "false"
		|| s == "def"
		|| s == "if"
		|| s == "else"
		|| s == "while"
		|| s == "for"
		|| s == "in"
		|| s == "import"
		|| s == "class"
		|| s == "const"
		|| s == "local" )
		return true;
	else
		return false;
}
