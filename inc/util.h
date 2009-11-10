// util.h
// utility functions used by deva language code
// created by jcs, october 22, 2009 

// TODO:
// * 

#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <vector>

using namespace std;

string get_cwd();

string get_extension( string & path );
string get_file_part( string & path );
string get_dir_part( string & path );
vector<string> split_path( string & path );

#endif // __UTIL_H__
