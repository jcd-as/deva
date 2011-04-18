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

// MS VC++ prior to 2010 doesn't have stdint.h
#ifndef _MSC_VER
#include <stdint.h>
#else
#if _MSV_VER < 1600
#include <stdint.h>
#endif 
#endif // not _MSC_VER

//typedef unsigned char byte;
//typedef unsigned short word;
//typedef unsigned int dword;

// visual c++ / ms-windows equivalents for versions prior to VC 2010
#ifdef _MSC_VER
#if _MSV_VER < 1600
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif
#endif // _MSC_VER

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;


namespace deva
{


// instruction opcodes
/////////////////////////////////////////////////////////////////////////////
// opcodes in the bytecode are stored as a byte with optional dword operands
// <OppN> = (dword) operand N
// tos = top of stack, tosN = N down from top-of-stack

// NOTE: if opcodes are added/removed, they also need to be added/removed from the
// array of opcode names immediately below
enum Opcode
{
	op_nop,
	op_pop,			// pop tos
	op_push,		// push integer <Op0> onto stack
	op_push_true,	// push boolean 'true' to tos
	op_push_false,	// push boolean 'false' to tos
	op_push_null,	// push null to tos
	op_push_zero,	// push the number '0'
	op_push_one,	// push the number '1'
	// no-operand shortcuts to push objects 0-3 to the tos
	op_push0, op_push1, op_push2, op_push3,
	op_pushlocal,	// push local number <Op0>
	// no-operand shortcuts to push locals 0-9 to the tos
	op_pushlocal0, op_pushlocal1, op_pushlocal2, op_pushlocal3, op_pushlocal4,
	op_pushlocal5, op_pushlocal6, op_pushlocal7, op_pushlocal8, op_pushlocal9,
	op_pushconst,	// push value from constant pool slot <W0> onto tos
	op_storeconst,	// store top-of-stack to object named at <Op0> in the const pool
	op_store_true,	// store boolean 'true' to <Op0>
	op_store_false,	// store boolean 'false' to <Op0>
	op_store_null,	// store null to <Op0>
	op_storelocal,	// store tos to local number <W0>
	// no-operand shortcuts to store tos to locals 0-9
	op_storelocal0, op_storelocal1, op_storelocal2, op_storelocal3, op_storelocal4,
	op_storelocal5, op_storelocal6, op_storelocal7, op_storelocal8, op_storelocal9,
	op_def_local,	// define local var <Op0> and store tos into it
	// no-operand shortcuts to define local 0-9 (& store tos into it)
	op_def_local0, op_def_local1, op_def_local2, op_def_local3, op_def_local4,
	op_def_local5, op_def_local6, op_def_local7, op_def_local8, op_def_local9,
	op_def_function,// create a new local function object. <Op0> has <constant index of> function name, <Op1> has the <const idx of> module name, <Op2> has the address
	op_def_method,// create a new method object. <Op0> has <constant index of> function name, <Op1> has the <constant index of> class name, <Op2> has the <const idx of> module name, <Op3> has the address
	op_new_map,		// create a new map object and push onto tos. <Op0> has count of items on stack
	op_new_vec,		// create a new vector object and push onto tos. <Op0> has count of items on stack
	op_new_class,	// create a new class object and push onto the tos. <Op0> has count of items on stack
	op_jmp,			// unconditional jump to the address on the tos
	op_jmpt,		// jump on tos evaluating to true
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
	op_add_assign,	// add <Op0> and tos and store back into <Op0>
	op_sub_assign,	// subtract <Op0> and tos and store back into <Op0>
	op_mul_assign,	// multiply <Op0> and tos and store back into <Op0>
	op_div_assign,	// divide <Op0> and tos and store back into <Op0>
	op_mod_assign,	// modulus <Op0> and tos and store back into <Op0>
	op_add_assign_local,// add local #<Op0> and tos and store back into <Op0>
	op_sub_assign_local,// subtract local #<Op0> and tos and store back into local #<Op0>
	op_mul_assign_local,// multiply local #<Op0> and tos and store back into local #<Op0>
	op_div_assign_local,// divide local #<Op0> and tos and store back into local #<Op0>
	op_mod_assign_local,// modulus local #<Op0> and tos and store back into local #<Op0>
	op_inc,			// increment tos
	op_dec,			// decrement tos
	op_call,		// call function with <Op0> args on stack, fcn on stack after args
	op_call_method,		// call function with <Op0> args on stack, fcn on stack after args
	op_return,		// execute <Op0> leave ops, pop the return address from the frame stack and jump to it, tos holds return value
	op_exit_loop,	// break/continue loop, jumping to <Op0>, executing <Op1> leave ops
	op_enter,		// enter (non-function) scope
	op_leave,		// leave (non-function) scope
	op_for_iter,	// tos has iterable object, call next() on it & push value onto tos - for single loop var loops (e.g. 'for( i in k)')
	op_for_iter_pair,// tos has iterable object, call next() on it & push value onto tos - for double loop var loops (e.g. 'for( i,j in k)')

	op_tbl_load,	// tos = tos1[tos]
	op_method_load,	// load method from a tbl. tos = tos1[tos], but leaves tos1 ('self') on the stack
	op_loadslice2,	// tos = tos2[tos1:tos]
	op_loadslice3,	// tos = tos3[tos2:tos1:tos]
	op_tbl_store,	// tos2[tos1] = tos
	op_storeslice2,	// 
	op_storeslice3,
	// augmented assignment for table stores
	op_add_tbl_store,// tos2[tos1] += tos
	op_sub_tbl_store,// tos2[tos1] -= tos
	op_mul_tbl_store,// tos2[tos1] *= tos
	op_div_tbl_store,// tos2[tos1] /= tos
	op_mod_tbl_store,// tos2[tos1] %= tos

	op_dup,			// tos -> tos<Op0> = tos (dup tos N times)
	// no-operand shortcuts to dup n times:
	op_dup1, op_dup2, op_dup3,
	// TODO: do we need the dup_top and rot ops?
	op_dup_top_n,	// tos<Op0> -> tos<Op0>*2 = tos -> tos<Op0> (dup top N items once)
	// no-operand shortcuts to dup top n:
	op_dup_top1, op_dup_top2, op_dup_top3,
	op_swap,		// tos = tos1, tos1 = tos
	op_rot,			// tos = tos<Op0>, tos1 -> tos<Op0> = tosN-1
	// no-operand shortcuts to rot n times (rot1 is op_swap):
	op_rot2, op_rot3, op_rot4, 

	op_import,

	// 119 (update as opcodes are added above)
	op_halt,
	op_illegal = 255	// illegal operation, if exists there was a compiler error/fault
};

extern const char* opcodeNames[];

} // namespace deva
#endif // __OPCODES_H__
