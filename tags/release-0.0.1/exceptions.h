// exceptions.h
// exceptions for the deva language
// created by jcs, september 26, 2009 

// TODO:
// * derive from logic_error and runtime_error, instead of 'exception'

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

// internal compiler error: indicates something wrong with the compiler's
// code-gen, internal state etc
class DevaICE : public DevaRuntimeException
{
public:
	DevaICE( const char* const s ) : DevaRuntimeException( s )
	{}
};

#endif // __EXCEPTIONS_H__
