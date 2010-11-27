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

// TODO:
// *

#include "util.h"
#include <cstring>
#include <stdexcept>
#include <boost/filesystem.hpp>

using namespace boost;

// helper to locate instances of 'in' in the string 'src', and replace them with
// 'out'
void replace( string& src, const char* const in, const char* const out )
{
	int len = strlen( in );
	size_t pos = src.find( in );
	int delta = strlen( out );
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
	for( int i = 0; i < parts.size(); ++i )
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
