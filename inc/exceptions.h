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

// exceptions.h
// exceptions for the deva language
// created by jcs, september 26, 2009 

// TODO:
// * 

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <stdexcept>
#include <boost/format.hpp>

using namespace std;

struct DevaSemanticException : public logic_error
{
	int line;

public:
	DevaSemanticException( const char* const s, int l ) : logic_error( s ), line( l )
	{}
	DevaSemanticException( boost::format fmt, int l ) : logic_error( str( fmt ).c_str() ), line( l )
	{}
	~DevaSemanticException() throw()
	{}
};

class DevaRuntimeException : public runtime_error
{
public:
	DevaRuntimeException( const char* const s ) : runtime_error( s )
	{}
	DevaRuntimeException( boost::format fmt ) : runtime_error( str( fmt ).c_str() )
	{}
};

class DevaStackException : public DevaRuntimeException
{
public:
	DevaStackException( const char* const s ) : DevaRuntimeException( s )
	{}
	DevaStackException( boost::format fmt ) : DevaRuntimeException( fmt )
	{}
};

// critical (often non-recoverable) error
class DevaCriticalException : public runtime_error
{
public:
	DevaCriticalException( const char* const s ) : runtime_error( s )
	{}
	DevaCriticalException( boost::format fmt ) : runtime_error( str( fmt ).c_str() )
	{}
};

// internal compiler error: indicates something wrong with the compiler's
// code-gen, internal state etc
class DevaICE : public DevaRuntimeException
{
public:
	DevaICE( const char* const s ) : DevaRuntimeException( s )
	{}
	DevaICE( boost::format fmt ) : DevaRuntimeException( fmt )
	{}
};

#endif // __EXCEPTIONS_H__
