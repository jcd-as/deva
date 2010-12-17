// Copyright (c) 2010 Joshua C. Shepard
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

// executor.cpp
// executive/VM global object/functions for the deva language
// created by jcs, december 16, 2010 

// TODO:
// * 

#include "executor.h"
#include "error.h"

#include <algorithm>

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// executive/VM functions and globals
/////////////////////////////////////////////////////////////////////////////

// global executor object
Executor* ex;


Executor::Executor()
{
	// set-up the constant and function object pools
	functions.reserve( 256 );
	constants.reserve( 256 );

	// add our 'global' function, "@main"
	DevaFunction f;
	f.name = string( "@main" );
	f.filename = string( current_file );
	f.firstLine = 0;
	f.numArgs = 0;
	f.numLocals = 0;
	f.addr = 0;

	functions.push_back( f );
}

Executor::~Executor()
{
	// free the constants' string data
	for( vector<DevaObject>::iterator i = constants.begin(); i != constants.end(); ++i )
	{
		// TODO: ???
//		if( *i.Type() == obj_string ) delete *i.s;
	}
}

int Executor::AddFunction( DevaFunction f )
{
	// don't add it if it's already in the set
	int idx = FindFunction( f );
	if( idx == -1 )
	{	
		functions.push_back( f );
		idx = functions.size()-1;
	}
	// TODO: keep it sorted so we can use a binary search to find things ??
//	sort( functions.begin(), functions.end() );
//	// get new idx

	return idx;
}

int Executor::FindFunction( const DevaFunction & f )
{
	// TODO: replace this with a binary search ??
	for( int i = 0; i < functions.size(); i++ )
	{
		if( functions[i] == f )
			return i;
	}
	return -1;
}

int Executor::AddConstant( DevaObject o )
{
	// don't add it if it's already in the set
	int idx = FindConstant( o );
	if( idx == -1 )
	{
		constants.push_back( o );
		idx = constants.size()-1;
	}
	// TODO: keep it sorted so we can use a binary search to find things ??
//	sort( constants.begin(), constants.end() );
//	// get new idx

	return idx;
}

int Executor::FindConstant( const DevaObject & o )
{
	// TODO: replace this with a binary search ??
	for( int i = 0; i < constants.size(); i++ )
	{
		if( constants[i] == o )
			return i;
	}
	return -1;
}


