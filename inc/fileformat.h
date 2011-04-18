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

// fileformat.h
// .dv file format definitions for the deva language, v2
// created by jcs, april 17, 2011

// TODO:
// * 

#ifndef __FILEFORMAT_H__
#define __FILEFORMAT_H__

namespace deva
{


// a compiled deva file (.dvc file) consists of:
// - a header
// - a constant data area
// - a 'global' data area
// - a list of function objects (including a "@main" global 'function')
// - a line mapping structure (line to instruction offset)
// - and a stream of instructions and their operands


// header
/////////////////////////////////////////////////////////////////////////////
// the header is a 16-byte section that looks like this:
//struct FileHeader
//{
//	static const byte deva[5];	// "deva"
//	static const byte ver[6];	// "2.0.0"
//	static const byte pad[5];	// "\0\0\0\0\0"
//	static unsigned long size(){ return sizeof( deva ) + sizeof( ver ) + sizeof( pad ); }
//};
// define the static members of the FileHeader struct
const char file_hdr_deva[5] = "deva";
const char file_hdr_ver[6] = "2.0.0";
const char file_hdr_pad[5] = "\0\0\0\0";
const dword sizeofFileHdr = sizeof( file_hdr_deva ) + sizeof( file_hdr_ver ) + sizeof( file_hdr_pad ); // 16


// constant data area
/////////////////////////////////////////////////////////////////////////////
const char constants_hdr[7] = ".const";
const char constants_hdr_pad[1] = "";
const dword sizeofConstantsHdr = sizeof( constants_hdr ) + sizeof( constants_hdr_pad ); // 8
// header is followed by dword containing the number of const objects
// constant data itself is an array of DevaObject structs:
// byte : object type. only number and string are allowed
// qword (number) OR 'len+1' bytes (string) : number or null-terminated string


// function object area
/////////////////////////////////////////////////////////////////////////////
const char functions_hdr[6] = ".func";
const char functions_hdr_pad[2] = "\0";
const dword sizeofFunctionsHdr = sizeof( functions_hdr ) + sizeof( functions_hdr_pad ); // 8
// header is followed by a dword containing the number of function objects
// function object data itself is an array of DevaFunction objects:
// len+1 bytes : 	name, null-terminated string
// len+1 bytes : 	filename
// dword :			starting line
// len+1 bytes :	classname (zero-length string if non-method)
// dword : 			number of arguments
// dword : 			'n' number of default arguments
// 'n' dwords :		default args (constant pool indices)
// dword :			number of locals
// dword :			number of names (externals, undeclared vars, functions)
// byte[] :			names, len+1 bytes null-terminated string each
// dword :			offset in code section of the code for this function


// line mapping data area
/////////////////////////////////////////////////////////////////////////////
const char linemap_hdr[9] = ".linemap";
const char linemap_hdr_pad[7] = "\0\0\0\0\0\0";
const dword sizeofLinemapHdr = sizeof( linemap_hdr ) + sizeof( linemap_hdr_pad ); // 8
// header is followed by a dword containing the number of line map entries
// line map data itself is an array of entries:
// dword : 	line
// dword :	address of instruction


} // namespace deva

#endif // __FILEFORMAT_H__
