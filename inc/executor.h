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

// executor.h
// executive/VM global object/functions for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include "opcodes.h"
#include "object.h"

using namespace std;


class Executor
{
	byte* ip;
	byte** code_blocks;

public:
	// (sorted, by name) list of function objects
	vector<DevaFunction> functions;

	// (sorted) constant pool 
	vector<DevaObject> constants;

	// global names

public:
	Executor();
	~Executor();
	int AddFunction( DevaFunction f );
	int FindFunction( const DevaFunction & f );
	inline DevaFunction GetFunction( int idx ) { return functions[idx]; }
	int AddConstant( DevaObject o );
	int FindConstant( const DevaObject & o );
	inline DevaObject GetConstant( int idx ) { return constants[idx]; }
};

extern Executor* ex;


#endif // __EXECUTOR_H__
