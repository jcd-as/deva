// builtins.h
// built-in functions for the deva language virtual machine
// created by jcs, october 04, 2009 

// TODO:
// * 

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include "instructions.h"
#include "executor.h"
#include <string>

using namespace std;

// is this name a built-in function?
bool is_builtin( string name );

// exectute built-in function
void execute_builtin( Executor *ex, const Instruction & inst );

#endif // __BUILTINS_H__
