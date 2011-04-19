// Copyright (c) 2011 Joshua C. Shepard
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

// code.h
// code block class for the deva language
// created by jcs, april 11, 2011

// TODO:
// * 

#ifndef __CODE_H__
#define __CODE_H__

#include "linemap.h"

#include <vector>
#include <set>

using namespace std;

namespace deva
{

class Code
{
public:
	// byte-code
	byte* code;
	size_t len;

	// number of constants defined/used in this code
//	size_t num_constants;

private:
	// set of constants (including all names)
	vector<Object> constants;
	set<Object> constants_set;

public:
	LineMap* lines;

//	Code() : code( NULL ), len( 0 ), num_constants( 0 ), lines( NULL ) {}
//	Code( byte* c, size_t l, size_t n, LineMap* ln ) : code( c ), len( l ), num_constants( n ), lines( ln ) {}
	Code() : code( NULL ), len( 0 ), lines( NULL ) {}
	Code( byte* c, size_t l, size_t n, LineMap* ln ) : code( c ), len( l ), lines( ln ) {}
	~Code()
	{
		delete[] code;
		delete lines;
		// free the constants' string data
		for( size_t i = 0; i < constants.size(); i++ )
		{
			ObjectType type = constants.at( i ).type;
			if( type == obj_string || type == obj_symbol_name ) delete [] constants.at( i ).s;
		}
	}

	inline bool AddConstant( Object o ) { if( constants_set.count( o ) != 0 ) return false; else { constants_set.insert( o ); constants.push_back( o ); return true; } }
	inline Object GetConstant( int idx ) const { return constants.at( idx ); }
	inline int FindConstant( const Object & o ) const
	{
		if( constants_set.count( o ) == 0 ) return -1;
		for( size_t i = 0; i < constants.size(); i++ )
			if( o == constants.at( i ) )
				return (int)i;
		return -1;
	}
	inline int NumConstants() const { return (int)constants.size(); }
};



} // namespace deva

#endif // __CODE_H__
