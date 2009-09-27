// opcodes.h
// opcodes for the deva language intermediate language & virtual machine
// created by jcs, september 14, 2009 

// TODO:
// * 

#ifndef __OPCODES_H__
#define __OPCODES_H__

// virtual machine & IL opcodes:
enum Opcode
{
	// TODO: need pop/push for each object type?
	op_pop,			// 0 pop top item off stack
	op_push,		// 1 push item onto top of stack
	op_load,		// 2 load a variable from memory to the stack
	op_store,		// 3 store a variable from the stack to memory
	op_defun,		// 4 define function. arg is location in instruction stream, named the fcn name
	op_defarg,		// 5 define an argument to a fcn. argument (to opcode) is arg name
	// TODO: need new for each object type??
	op_new,			// 6 create a new object and place on top of stack
	op_new_map,		// 7 create a new map object and push onto stack
	op_new_vec,		// 8 create a new vector object and push onto stack
	op_vec_load,	// 9 get item from vector
	op_vec_store,	// 10 set item in vector. args: index, value
	op_map_load,	// 11 get item from map
	op_map_store,	// 12 set item in map. args: index, value
	op_jmp,			// 13 unconditional jump to the address on top of the stack
	op_jmpf,		// 14 jump on top of stack evaluating to false 
	op_eq,			// 15 == compare top two values on stack
	op_neq,			// 16 != compare top two values on stack
	op_lt,			// 17 < compare top two values on stack
	op_lte,			// 18 <= compare top two values on stack
	op_gt,			// 19 > compare top two values on stack
	op_gte,			// 20 >= compare top two values on stack
	op_or,			// 21 || the top two values
	op_and,			// 22 && the top two values
	op_neg,			// 23 negate the top value ('-' operator)
	op_not,			// 24 boolean not the top value ('!' operator)
	op_add,			// 25 add top two values on stack
	op_sub,			// 26 subtract top two values on stack
	op_mul,			// 27 multiply top two values on stack
	op_div,			// 28 divide top two values on stack
	op_mod,			// 29 modulus top two values on stack
	op_output,		// 30 dump top of stack to stdout
	op_call,		// 31 call a function. arguments on stack
	op_return,		// 32 pop the return address and unconditionally jump to it
	op_returnv,		// 33 as return, but stack holds return value and then (at top) return address
	op_break,		// 34 break out of loop, respecting scope (enter/leave)
	op_enter,		// 35 enter new scope
	op_leave,		// 36 leave scope
	op_nop,			// 37 no op
	op_halt,		// 38 finish program, 0 or 1 ops (return code)
	op_illegal = 255	// illegal operation, if exists there was a compiler error/fault
};

#endif // __OPCODES_H__
