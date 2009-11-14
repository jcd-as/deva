// util.cpp
// utility functions used by deva language code
// created by jcs, october 22, 2009 

// TODO:
// *

#include "util.h"
#include <stdexcept>
#include <boost/filesystem.hpp>

using namespace boost;

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
