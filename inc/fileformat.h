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

