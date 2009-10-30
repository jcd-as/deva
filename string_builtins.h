// string_builtins.h
// built-in methods on strings, for the deva language virtual machine
// created by jcs, october 30, 2009 

// TODO:
// * 

#ifndef __STRING_BUILTINS_H__
#define __STRING_BUILTINS_H__

#include "instructions.h"
#include "executor.h"
#include <string>

using namespace std;

// is this name a built-in function?
bool is_string_builtin( const string & name );

// exectute built-in function
void execute_string_builtin( Executor *ex, const string & name );

#endif // __STRING_BUILTINS_H__

