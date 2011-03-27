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

// util.h
// utility functions used by deva language code
// created by jcs, october 22, 2009 

// TODO:
// * 

#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <vector>
#include <cmath>

using namespace std;


namespace deva
{

// file name and path utility functions
/////////////////////////////////////////////////////////////////////////////
extern const char dirsep;
extern const char extsep;
extern const char* const cwdstr;
extern const char* const pardirstr;
extern const char* env_var_path_seps;

string get_cwd();
string get_extension( string & path );
string get_file_part( string & path );
string get_dir_part( string & path );
bool exists( string & path );
vector<string> split_path( string & path );
string join_paths( const string & base, const string & add );
string join_paths( vector<string> & parts );
void split_env_var_paths( const string & var, vector<string> &  paths );


// symbol name and string utility functions
/////////////////////////////////////////////////////////////////////////////
void replace( string& src, const char* const in, const char* const out );
void split( const string& in, const char* const splitchars, vector<string> & out );
// allocate and return a copy of a string
char* copystr( const char* in );
// allocate a concatenation of two strings
char* catstr( const char* s1, const char* s2 );

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

// parse a number (decimal, binary, octal or hex)
double parse_number( const char* s );

// is a double an integral value?
inline bool is_integral( double d )
{
	double intpart;
	if( modf( d, &intpart ) == 0.0 ) return true;
	else return false;
}

} // namespace deva

#endif // __UTIL_H__
