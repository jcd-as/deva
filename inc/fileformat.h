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

// fileformat.h
// file format (file header etc) for deva compiled bytecode files
// created by jcs, september 24, 2009 

// TODO:
// * 

#ifndef __FILEFORMAT_H__
#define __FILEFORMAT_H__

// a compiled deva file (.dvc file) consists of a header and a stream of
// instructions and their arguments

// the header is a 16-byte section that looks like this:
struct FileHeader
{
	static const char deva[5];
	static const char ver[6];
	static const char pad[5];
	static unsigned long size(){ return sizeof( deva ) + sizeof( ver ) + sizeof( pad ); }
};

// and the instruction/argument stream is an array of:
//
// format for virtual machine instructions:
// byte: opcode
// [byte: argument type
// 32bit+ (dependent on type): argument]
// ... for each arg
// byte: 255 for end of instruction

#endif // __FILEFORMAT_H__

