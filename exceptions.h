// exceptions.h
// exceptions for the deva language
// created by jcs, september 26, 2009 

// TODO:
// * 

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__


// generic deva exception
class DevaException : public exception
{
	const char* const msg;

public:
	DevaException( const char* const text ) : msg( text )
	{}
	~DevaException() throw()
	{}
	// override
	const char* what(){ return msg; }
};

struct DevaSemanticException : public DevaException
{
	NodeInfo node;

public:
	DevaSemanticException( const char* const s, NodeInfo ni ) : DevaException( s ), node( ni )
	{}
	~DevaSemanticException() throw()
	{}
};

class DevaRuntimeException : public DevaException
{
public:
	DevaRuntimeException( const char* const s ) : DevaException( s )
	{}
};

#endif // __EXCEPTIONS_H__
