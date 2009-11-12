// util.cpp
// utility functions used by deva language code
// created by jcs, october 22, 2009 

// TODO:
// *

#include "util.h"
#include <stdexcept>

string get_cwd()
{
	char* cwd;
	long maxbufsize = pathconf( cwdstr, _PC_PATH_MAX );
	if( maxbufsize == -1 )
		throw logic_error( "Unable to determine max path size. Unable to continue." );
	cwd = new char[maxbufsize];
	(void)getcwd( cwd, (size_t)maxbufsize );
	string curdir( cwd );
	delete [] cwd;
	return curdir;
}

string get_extension( string & path )
{
	// look backwards for the last '.' and return the following part
	size_t pos = path.rfind( extsep );
	if( pos == string::npos )
		throw logic_error( "Invalid file path. Unable to continue." );
	string ret( path );
	ret.erase( 0, pos + 1 );
	return ret;
}

string get_file_part( string & path )
{
	// look backwards for the last '/' and return the following part
	size_t pos = path.rfind( dirsep );
	// no separator? assume whole thing is filename
	if( pos == string::npos )
		return path;
	string ret( path );
	ret.erase( 0, pos + 1 );
	return ret;
}

string get_dir_part( string & path )
{
	// look backwards for the last '/' and return the preceding part
	size_t pos = path.rfind( dirsep );
	// no separator? assume whole thing is filename
	if( pos == string::npos )
		return string( "" );
	string ret( path );
	ret.erase( pos );
	return ret;
}

vector<string> split_path( string & path )
{
	// parse out the parts of path given (separated by '/'s)
	vector<string> paths;
	// find a '/'
	size_t old_pos = 0;
    size_t pos = path.find( dirsep );
    while( pos != string::npos )
    {
		// create a new string up to the '/'
		string n( path, old_pos, pos - old_pos );
		if( n[0] == dirsep )
			n.erase( 0, 1 );
		paths.push_back( n );
		old_pos = pos;
        pos = path.find( dirsep, old_pos + 1 );
    }
	// push the last string
	string n( path, old_pos, path.length() - old_pos );
	if( n[0] == dirsep )
		n.erase( 0, 1 );
	paths.push_back( n );

	return paths;
}

string join_paths( vector<string> & parts )
{
	string ret;
	for( int i = 0; i < parts.size() - 1; ++i )
	{
		ret += parts[i];
		// ensure this part ends with a single dirsep ('/')
		if( i != parts.size()-1 && ret[ret.length()-1] != dirsep )
			ret.push_back( dirsep );
	}

	return ret;
}
