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

// typedefs.h
// some basic type definitions for the deva language, v2
// created by jcs, april 18, 2011

// TODO:
// * 

#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

// MS VC++ prior to 2010 doesn't have stdint.h
//#ifndef _MSC_VER
//#define __STDC_LIMIT_MACROS
//#include <stdint.h>
//#else
//#if _MSV_VER > 1500
//#define __STDC_LIMIT_MACROS
//#include <stdint.h>
//#endif 
//#endif // not _MSC_VER
//
//// visual c++ / ms-windows equivalents for versions prior to VC 2010
//#ifdef _MSC_VER
//#if _MSV_VER < 1600
//typedef signed __int8     int8_t;
//typedef signed __int16    int16_t;
//typedef signed __int32    int32_t;
//typedef unsigned __int8   uint8_t;
//typedef unsigned __int16  uint16_t;
//typedef unsigned __int32  uint32_t;
//typedef signed __int64       int64_t;
//typedef unsigned __int64     uint64_t;
//
//#define INT8_MAX	(127)
//#define INT8_MIN	(-128)
//#define INT16_MAX	(32767)
//#define INT16_MIN	(-32768)
//#define INT32_MAX	(2147483647)
//#define INT32_MIN	(−2147483648)
//#define INT64_MAX	(9223372036854775807)
//#define INT64_MIN	(-9223372036854775808)
//#define UINT8_MAX	(255)
//#define UINT16_MAX	(65535)
//#define UINT32_MAX	(4294967295)
//#define UINT64_MAX	(18446744073709551615)
//
//#endif
//#endif // _MSC_VER

#include <boost/cstdint.hpp>

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;


#endif // __TYPEDEFS_H__

