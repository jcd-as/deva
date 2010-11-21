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

vector<string> split_env_var_paths( string var )
{
	// split it into separate paths (on the ":" char in un*x)
	vector<string> paths;
	size_t left = var.find_first_not_of( env_var_path_seps );
	if( left != string::npos )
	{	
		size_t right = var.find_first_of( env_var_path_seps );
		size_t len = var.length();
		while( left != string::npos )
		{
			string s( var, left, right - left );
			paths.push_back( s );

			left = var.find_first_not_of( env_var_path_seps, right );
			right = var.find_first_of( env_var_path_seps, right + 1 );
			// if 'left' is greater than 'right', then we passed an empty string 
			// (two matching split chars in a row), enter it and move forward
			if( left > right )
			{
				paths.push_back( "" );
				right = var.find_first_of( env_var_path_seps, right + 1 );
			}
		}
	}
	return paths;
}
