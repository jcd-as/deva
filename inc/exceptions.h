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


namespace deva
{

struct SemanticException : public logic_error
{
	int line;

public:
	SemanticException( const char* const s, int l ) : logic_error( s ), line( l )
	{}
	SemanticException( boost::format fmt, int l ) : logic_error( str( fmt ).c_str() ), line( l )
	{}
	~SemanticException() throw()
	{}
};

class RuntimeException : public runtime_error
{
public:
	RuntimeException( const char* const s ) : runtime_error( s )
	{}
	RuntimeException( boost::format fmt ) : runtime_error( str( fmt ).c_str() )
	{}
};

class StackException : public RuntimeException
{
public:
	StackException( const char* const s ) : RuntimeException( s )
	{}
	StackException( boost::format fmt ) : RuntimeException( fmt )
	{}
};

// critical (often non-recoverable) error
class CriticalException : public runtime_error
{
public:
	CriticalException( const char* const s ) : runtime_error( s )
	{}
	CriticalException( boost::format fmt ) : runtime_error( str( fmt ).c_str() )
	{}
};

// internal compiler error: indicates something wrong with the compiler's
// code-gen, internal state etc
class ICE : public RuntimeException
{
public:
	ICE( const char* const s ) : RuntimeException( s )
	{}
	ICE( boost::format fmt ) : RuntimeException( fmt )
	{}
};

// user-raised error
class UserException : public RuntimeException
{
public:
	UserException( const char* const s ) : RuntimeException( s )
	{}
	UserException( boost::format fmt ) : RuntimeException( fmt )
	{}
};


} // namespace deva

#endif // __EXCEPTIONS_H__
