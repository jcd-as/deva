// map_builtins.h
// built-in methods on map obejcts, for the deva language virtual machine
// created by jcs, october 21, 2009 

// TODO:
// * 

#ifndef __MAP_BUILTINS_H__
#define __MAP_BUILTINS_H__

#include "instructions.h"
#include "executor.h"
#include <string>

using namespace std;

// is this name a built-in function?
bool is_map_builtin( const string & name );

// exectute built-in function
void execute_map_builtin( Executor *ex, const string & name );

#endif // __MAP_BUILTINS_H__
