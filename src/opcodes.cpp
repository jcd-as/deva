// Copyright (c) 2011 Joshua C. Shepard
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

// opcodes.cpp
// virtual machine opcodes for the deva language, v2
// created by jcs, march 26, 2011

// TODO:
// * 

#include "opcodes.h"

namespace deva
{

const char* opcodeNames[] =
{
	"nop",
	"pop",
	"push",
	"push_true",
	"push_false",
	"push_null",
	"push_zero",
	"push_one",
	"push0", 
	"push1",
	"push2",
	"push3",
	"pushlocal",
	"pushlocal0",
	"pushlocal1",
	"pushlocal2",
	"pushlocal3",
   	"pushlocal4",
	"pushlocal5",
   	"pushlocal6",
   	"pushlocal7",
   	"pushlocal8",
   	"pushlocal9",
	"pushconst",
	"storeconst",
	"store_true",
	"store_false",
	"store_null",
	"storelocal",
	"storelocal0",
	"storelocal1",
	"storelocal2",
	"storelocal3",
	"storelocal4",
	"storelocal5",
	"storelocal6",
	"storelocal7",
	"storelocal8",
	"storelocal9",
	"def_local", 
	"def_local0", 
	"def_local1", 
	"def_local2", 
	"def_local3", 
	"def_local4",
	"def_local5", 
	"def_local6", 
	"def_local7", 
	"def_local8", 
	"def_local9",
	"def_function",
	"new_map",
	"new_vec",
	"new_class",
	"jmp",
	"jmpf",
	"eq",
	"neq",
	"lt",
	"lte",
	"gt",
	"gte",
	"or",
	"and",
	"neg",
	"not",
	"add",
	"sub",
	"mul",
	"div",
	"mod",
	"add_assign",
	"sub_assign",
	"mul_assign",
	"div_assign",
	"mod_assign",
	"add_assign_local",
	"sub_assign_local",
	"mul_assign_local",
	"div_assign_local",
	"mod_assign_local",
	"call",
	"call_method",
	"return",
	"exit_loop",
	"enter",
	"leave",
	"for_iter",
	"for_iter_pair",
	"tbl_load",
	"method_load",
	"loadslice2",
	"loadslice3",
	"tbl_store",
	"storeslice2",
	"storeslice3",
	"add_tbl_store",
	"sub_tbl_store",
	"mul_tbl_store",
	"div_tbl_store",
	"mod_tbl_store",
	"dup",
	"dup1", 
	"dup2", 
	"dup3",
	"dup_top_n",
	"dup_top1", 
	"dup_top2", 
	"dup_top3",
	"swap",
	"rot",
	"rot2", 
	"rot3", 
	"rot4", 
	"import",
	"halt",
	"illegal",
};

} // namespace deva
