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
	op_pop,			// 0 pop top item off stack
	op_push,		// 1 push item onto top of stack
	op_load,		// 2 load a variable from memory to the stack
	op_store,		// 3 store a variable from the stack to memory
	op_defun,		// 4 define function. arg is location in instruction stream, named the fcn name
	op_defarg,		// 5 define an argument to a fcn. argument (to opcode) is arg name
	op_dup,			// 6 dup a stack item. arg is index of item to dup (onto top of stack)
	op_new_map,		// 7 create a new map object and push onto stack
	op_new_vec,		// 8 create a new vector object and push onto stack
	op_tbl_load,	// 9 get item from vector or map
	op_tbl_store,	// 10 set item in vector or map. args: index, value
	op_swap,		// 11 swap top two items on stack (no args)
	op_line_num,	// 12 line number (for debugging). 1st arg is the line number
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
	op_call,		// 30 call a function. arguments on stack. fcn to call either in arg *or* on stack (if no args)
	op_return,		// 31 pop the return address and unconditionally jump to it, stack holds return value
	op_enter,		// 32 enter new scope
	op_leave,		// 33 leave scope
	op_nop,			// 34 no op
	op_halt,		// 35 finish program, 0 or 1 ops (return code)
	op_new_class,	// 36 create a new class object and push onto the stack
	op_new_instance,// 37 create a new instance of a class and push onto the stack
	op_endf,		// 38 end of function. nop indicating end of defun
	op_roll,		// 39 roll the stack from a given position
	op_illegal = 255	// illegal operation, if exists there was a compiler error/fault
};

#endif // __OPCODES_H__
