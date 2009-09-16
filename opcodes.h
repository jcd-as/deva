// opcodes.h
// opcodes for the deva language intermediate language & virtual machine
// created by jcs, september 14, 2009 

// TODO:
// * 

#ifndef __OPCODES_H__
#define __OPCODES_H__

// TODO: size enum to fit in unsigned char??
enum Opcode
{
	// TODO: need pop/push for each object type?
	op_pop,			// pop top item off stack
//	op_peek,		// look at top item on stack without removing it
	op_push,		// push item onto top of stack
	// TODO: need load/store for each object type?
	op_load,		// load a variable from memory to the stack
	op_store,		// store a variable from the stack to memory
	// TODO: need new for each object type??
	op_new,			// create a new object and place on top of stack
	// TODO: need vector load/store for each object type?
	op_vec_load,	// get item from vector
	op_vec_store,	// set item in vector. args: index, value
	// TODO: need map load/store for each object type?
	op_map_load,	// get item from map
	op_map_store,	// set item in map. args: index, value
	op_jmp,			// unconditional jump to the address on top of the stack
	op_jmpf,		// jump on false
	op_cmp,			// compare top two values on stack
	op_neg,			// negate the top value ('-' operator)
	op_not,			// boolean not the top value ('!' operator)
	op_add,			// add top two values on stack
	op_sub,			// subtract top two values on stack
	op_mul,			// multiply top two values on stack
	op_div,			// divide top two values on stack
	op_mod,			// modulus top two values on stack
	op_output,		// dump top of stack to stdout
	op_call,		// call a function. arguments on stack
	op_return,		// pop the return address and unconditionally jump to it
	op_returnv,		// as return, but stack holds return value and then (at top) return address
	op_nop			// no op
};

#endif // __OPCODES_H__
