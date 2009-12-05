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

struct DevaSemanticException : public logic_error
{
	NodeInfo node;

public:
	DevaSemanticException( const char* const s, NodeInfo ni ) : logic_error( s ), node( ni )
	{}
	~DevaSemanticException() throw()
	{}
};

class DevaRuntimeException : public runtime_error
{
public:
	DevaRuntimeException( const char* const s ) : runtime_error( s )
	{}
};

class DevaStackException : public DevaRuntimeException
{
public:
	DevaStackException( const char* const s ) : DevaRuntimeException( s )
	{}
};

// critical (often non-recoverable) error
class DevaCriticalException : public runtime_error
{
public:
	DevaCriticalException( const char* const s ) : runtime_error( s )
	{}
};

// internal compiler error: indicates something wrong with the compiler's
// code-gen, internal state etc
class DevaICE : public DevaRuntimeException
{
public:
	DevaICE( const char* const s ) : DevaRuntimeException( s )
	{}
};

#endif // __EXCEPTIONS_H__
