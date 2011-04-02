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

// util.cpp
// utility functions used by deva language code
// created by jcs, october 22, 2009 


#include "util.h"
#include <cstring>
#include <stdexcept>
#include <boost/filesystem.hpp>

using namespace boost;


namespace deva
{

const char dirsep = '/';
const char extsep = '.';
const char* const cwdstr = ".";
const char* const pardirstr = "..";
const char* env_var_path_seps = ":";

// file name and path utility functions
/////////////////////////////////////////////////////////////////////////////
string get_cwd()
{
	filesystem::path p = filesystem::current_path();
	return p.string();
}

string get_extension( string & path )
{
	filesystem::path p( path );
	return p.extension();
}

string get_stem( string & path )
{
	filesystem::path p( path );
	return p.stem();
}

string get_file_part( string & path )
{
	filesystem::path p( path );
	return p.filename();
}

string get_dir_part( string & path )
{
	filesystem::path p( path );
	return p.parent_path().string();
}

bool exists( string & path )
{
	filesystem::path p( path );
	return filesystem::exists( p );
}

vector<string> split_path( string & path )
{
	vector<string> paths;
	filesystem::path p( path );
	for( filesystem::path::iterator i = p.begin(); i != p.end(); ++i )
	{
		// don't push slashes and dots
		if( *i != "." && *i != "/" )
			paths.push_back( *i );
	}
	return paths;
}

string join_paths( const string & base, const string & add )
{
	filesystem::path basepath( base );
	filesystem::path addpath( add );
	basepath /= addpath;
	basepath.normalize();
	return basepath.string();
}

string join_paths( vector<string> & parts )
{
	filesystem::path retpath;
	for( size_t i = 0; i < parts.size(); ++i )
	{
		retpath /= filesystem::path( parts[i] );
	}
	retpath.normalize();
	return retpath.string();
}

void split_env_var_paths( const string & var, vector<string> & paths )
{
	// split it into separate paths (on the ":" char in un*x)
	split( var, env_var_path_seps, paths );
}


// symbol name and string utility functions
/////////////////////////////////////////////////////////////////////////////
// helper to locate instances of 'in' in the string 'src', and replace them with
// 'out'
void replace( string& src, const char* const in, const char* const out )
{
	int len = (int)strlen( in );
	size_t pos = src.find( in );
	int delta = (int)strlen( out );
	while( pos != string::npos )
	{
		// found 'in' at 'i'. replace with 'out'
		src = src.replace( pos, len, out );
		pos = src.find( in, pos + delta );
	}
}

void split( const string & in, const char* const splitchars, vector<string> & ret )
{
	size_t len = in.length();
	// lhs of the first item is always the first character
	size_t left = 0;
	// rhs of the first item is the first splitting character
	size_t right = in.find_first_of( splitchars );
	if( right == string::npos )
		right = len;
	// while lhs is not at the end of the input string
	while( left <= len )
	{
		if( left != right )
			ret.push_back( string( in, left, right - left ) );
		left = right + 1;
		right = in.find_first_of( splitchars, right + 1 );
		if( right == string::npos )
			right = len;
	}
}

// allocate and return a copy of a string
char* copystr( const char* in )
{
	char* ret = new char[strlen( in ) + 1];
	strcpy( ret, in );
	return ret;
}

// allocate and return a copy of a string
char* copystr( string in )
{
	char* ret = new char[in.length() + 1];
	strcpy( ret, in.c_str() );
	return ret;
}

// allocate a concatenation of two strings
char* catstr( const char* s1, const char* s2 )
{
	char* ret = new char[strlen( s1 ) + strlen( s2 ) + 1];
	strcpy( ret, s1 );
	strcat( ret, s2 );
	return ret;
}

// strip the whitespace and leading comments from a string,
// to create a valid symbol name
string strip_symbol( const string& src, const string& c )
{
	// if src starts with a comment character ('#'), remove from that up to the
	// first nl/cr
	string src2;
	size_t comment_pos = src.find_first_of( "#" );
	if( comment_pos != string::npos )
	{
		size_t first_nl_pos = src.find_first_of( "\r\n", comment_pos + 1 );
		if( first_nl_pos != string::npos )
		{
			src2 = src.substr( first_nl_pos + 1, string::npos );
		}
		else src2 = src;
	}
	else src2 = src;
 	size_t p2 = src2.find_last_not_of( c );
 	if( p2 == string::npos ) 
		return string();
	size_t p1 = src2.find_first_not_of( c );
	if( p1 == string::npos )
		p1 = 0;
	return src2.substr( p1, (p2 - p1) + 1 );
}

// strip single and double quotes (for use in getting the value of a string
// variable, for instance)
string strip_quotes( const string& src )
{
	string quotes( "\"'" );

	size_t start = src.find_first_of( quotes );
	if( start == string::npos )
		return src;

	char c;
	if( src[start] == '"' )
		c = '"';
	else
		c = '\'';

	size_t p2 = src.find_last_not_of( c );
	// no characters after 'start' that aren't quotes: zero-length string
	if( p2 == string::npos ) 
		return string( "" );
	size_t p1 = src.find_first_not_of( c );
	if( p1 == string::npos )
		p1 = 0;
	return src.substr( p1, (p2 - p1) + 1 );
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
		// 'new' IS a keyword, but acts as an identifier too
//		|| s == "new"
		// same with 'delete'
//		|| s == "delete"
		|| s == "const"
		|| s == "local" )
		return true;
	else
		return false;
}

// parse a number (decimal, binary, octal or hex)
double parse_number( const char* s )
{
	double n;
	// hex? octal? binary?
	if( s[0] == '0' && strlen( s ) > 1 )
	{
		char* end;
		if( s[1] == 'x' )
		{
			long l = strtol( s+2, &end, 16 );
			n = l;
			return n;
		}
		else if( s[1] == 'o' )
		{
			long l = strtol( s+2, &end, 8 );
			n = l;
			return n;
		}
		else if( s[1] == 'b' )
		{
			long l = strtol( s+2, &end, 2 );
			n = l;
			return n;
		}
		else
		{
			// or real?
			n = atof( s );
			return n;
		}
	}
	// or real?
	n = atof( s );
	return n;
}

} // namespace deva
