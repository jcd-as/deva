// vector_builtins.h
// built-in methods on vector obejcts, for the deva language virtual machine
// created by jcs, october 21, 2009 

// TODO:
// * 

#ifndef __VECTOR_BUILTINS_H__
#define __VECTOR_BUILTINS_H__

#include "instructions.h"
#include "executor.h"
#include <string>

using namespace std;

// is this name a built-in function?
bool is_vector_builtin( const string & name );

// exectute built-in function
void execute_vector_builtin( Executor *ex, const string & name );

#endif // __VECTOR_BUILTINS_H__
