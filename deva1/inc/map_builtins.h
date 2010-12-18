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