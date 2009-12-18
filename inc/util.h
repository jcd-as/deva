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
// * portable directory separator
// * portable path sep in env vars (ms windows uses ";", for instance)

#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <vector>

using namespace std;

static const char dirsep = '/';
static const char extsep = '.';
static const char* const cwdstr = ".";
static const char* const pardirstr = "..";
static const char* env_var_path_seps = ":";

void replace( string& src, const char* const in, const char* const out );
string get_cwd();
string get_extension( string & path );
string get_file_part( string & path );
string get_dir_part( string & path );
bool exists( string & path );
vector<string> split_path( string & path );
string join_paths( const string & base, const string & add );
string join_paths( vector<string> & parts );
vector<string> split_env_var_paths( string var );

#endif // __UTIL_H__
