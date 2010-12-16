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

// opcodes.h
// virtual machine opcodes for the deva language, v2
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __OPCODES_H__
#define __OPCODES_H__


typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

// a compiled deva file (.dvc file) consists of:
// - a header
// - a constant data area
// - a 'global' data area
// - a list of function objects (including a "@main" global 'function')
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
const char file_hdr_ver[6] = "1.0.0";
const char file_hdr_pad[5] = "\0\0\0\0";
const size_t sizeofFileHdr = sizeof( file_hdr_deva ) + sizeof( file_hdr_ver ) + sizeof( file_hdr_pad ); // 16


// constant data area
/////////////////////////////////////////////////////////////////////////////
const char constants_hdr[7] = ".const";
const char constants_hdr_pad[1] = "";
const size_t sizeofConstantsHdr = sizeof( constants_hdr ) + sizeof( constants_hdr_pad ); // 8
// header is followed by dword containing the number of const objects
// constant data itself is an array of DevaObject structs:
// byte : object type. only number and string are allowed
// qword (number) OR 'len+1' bytes (string) : number or null-terminated string


// 'global names' data area
/////////////////////////////////////////////////////////////////////////////
const char global_hdr[8] = ".global";
const size_t sizeofGlobalHdr = sizeof( global_hdr ); // 8
// header is followed by a dword containing the number of global names
// global names data itself is an array of null-terminated strings


// function object area
/////////////////////////////////////////////////////////////////////////////
const char functions_hdr[6] = ".func";
const char functions_hdr_pad[2] = "\0";
const size_t sizeofFunctionsHdr = sizeof( functions_hdr ) + sizeof( functions_hdr_pad ); // 8
// header is followed by a dword containing the number of function objects
// function object data itself is an array of DevaFunction objects:
// len+1 bytes : 	name, null-terminated string
// len+1 bytes : 	filename
// dword :			starting line
// dword : 			number of arguments
// dword :			number of locals
// dword :			number of names (externals, undeclared vars, functions)
// bytes :			names, len+1 bytes null-terminated string each
// dword :			offset in code section of the code for this function


// instruction opcodes
/////////////////////////////////////////////////////////////////////////////
// opcodes in the bytecode are stored as a byte with optional dword operands
// <OppN> = (dword) operand N
// tos = top of stack, tosN = N down from top-of-stack

enum Opcode
{
	op_nop,
	op_pop,			// pop tos
	op_push,		// push object named at names slot <Op0> 
	op_push_true,	// push boolean 'true' to tos
	op_push_false,	// push boolean 'false' to tos
	op_push_null,	// push null to tos
	// no-operand shortcuts to push objects 0-3 to the tos
	op_push0, op_push1, op_push2, op_push3,
	op_pushlocal,	// push local number <Op0>
	// no-operand shortcuts to push locals 0-9 to the tos
	op_pushlocal0, op_pushlocal1, op_pushlocal2, op_pushlocal3, op_pushlocal4,
	op_pushlocal5, op_pushlocal6, op_pushlocal7, op_pushlocal8, op_pushlocal9,
	op_store,		// store top-of-stack to object named at <Op0>
	op_store_true,	// store boolean 'true' to <Op0>
	op_store_false,	// store boolean 'false' to <Op0>
	op_store_null,	// store null to <Op0>
	op_storelocal,	// store tos to local number <W0>
	// no-operand shortcuts to store tos to locals 0-9
	op_storelocal0, op_storelocal1, op_storelocal2, op_storelocal3, op_storelocal4,
	op_storelocal5, op_storelocal6, op_storelocal7, op_storelocal8, op_storelocal9,
	op_push_const,	// push value from constant pool slot <W0> onto tos
	op_new_map,		// create a new map object and push onto tos
	op_new_vec,		// create a new vector object and push onto tos
	op_new_class,	// create a new class object and push onto the tos
	op_new_instance,// create a new instance of a class and push onto the tos
	op_jmp,			// unconditional jump to the address on the tos
	op_jmpf,		// jump on tos evaluating to false 
	op_eq,			// == compare tos and tos1
	op_neq,			// != compare tos and tos1
	op_lt,			// < compare tos and tos1
	op_lte,			// <= compare tos and tos1
	op_gt,			// > compare tos and tos1
	op_gte,			// >= compare tos and tos1
	op_or,			// || the tos and tos1
	op_and,			// && the tos and tos1
	op_neg,			// negate the tos ('-' operator)
	op_not,			// boolean not the tos ('!' operator)
	op_add,			// add tos and tos1
	op_sub,			// subtract tos and tos1
	op_mul,			// multiply tos and tos1
	op_div,			// divide tos and tos1
	op_mod,			// modulus tos and tos1
	op_call,		// call function named at <Op0>. arguments on stack
	op_return,		// pop the return address and unconditionally jump to it, tos holds return value
	op_break,		// break loop
	op_continue,	// continue loop
	op_enter,		// enter (non-function) scope
	op_leave,		// leave (non-function) scope
	op_for_iter,	// tos has iterable object, call next() on it & push value onto tos

	op_tbl_load,	// tos = tos1[tos]
	op_slice2,		// tos = tos2[tos1:tos]
	op_slice3,		// tos = tos3[tos2:tos1:tos]
	op_tbl_load_local,	// tos = local tos1[tos]
	op_slice2local,	// tos = local tos2[tos1:tos]
	op_slice3local,	// tos = local tos3[tos2:tos1:tos]

	op_halt,
	op_illegal = 255	// illegal operation, if exists there was a compiler error/fault
};


#endif // __OPCODES_H__
