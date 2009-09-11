// symbol.cpp
// deva language symbol information
// created by jcs, september 9, 2009

#include "symbol.h"


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
