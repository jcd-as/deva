// opcodes.h
// opcodes for the deva language intermediate language & virtual machine
// created by jcs, september 14, 2009 

// TODO:
// * 

#ifndef __OPCODES_H__
#define __OPCODES_H__

// format for virtual machine instructions:
// byte: opcode
// [byte: argument type
// 64bit: argument]
// ... for each arg

// virtual machine & IL opcodes:
enum Opcode
{
	// TODO: need pop/push for each object type?
	op_pop,			// pop top item off stack
//	op_peek,		// look at top item on stack without removing it
	op_push,		// push item onto top of stack
	op_load,		// load a variable from memory to the stack
	op_store,		// store a variable from the stack to memory
	// TODO: need new for each object type??
	op_new,			// create a new object and place on top of stack
	op_new_map,		// create a new map object and push onto stack
	op_new_vec,		// create a new vector object and push onto stack
	op_vec_load,	// get item from vector
	op_vec_store,	// set item in vector. args: index, value
	op_map_load,	// get item from map
	op_map_store,	// set item in map. args: index, value
	op_jmp,			// unconditional jump to the address on top of the stack
	op_jmpf,		// jump on top of stack evaluating to false 
	op_eq,			// == compare top two values on stack
	op_neq,			// != compare top two values on stack
	op_lt,			// < compare top two values on stack
	op_lte,			// <= compare top two values on stack
	op_gt,			// > compare top two values on stack
	op_gte,			// >= compare top two values on stack
	op_or,			// || the top two values
	op_and,			// && the top two values
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
	op_enter,		// enter new scope
	op_leave,		// leave scope
	op_nop,			// no op
	op_halt,		// finish program, 0 or 1 ops (return code)
	op_illegal		// illegal operation, if exists there was a compiler error/fault
};

#endif // __OPCODES_H__
