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

// builtin_helpers.h
// helper functions for writing built-in functions/methods
// created by jcs, january 10, 2010 

// TODO:
// * 

#ifndef __BUILTIN_HELPERS__
#define __BUILTIN_HELPERS__

#include "executor.h"

// get the 'this' object off the top of the stack
DevaObject get_this( Executor *ex, const char* fcn, SymbolType type );
// get a fcn argument off the top of the stack
DevaObject get_arg( Executor *ex, const char* fcn, const char* arg );
// get a fcn argument off the top of the stack and verify it is of the correct
// type
DevaObject get_arg_of_type( Executor *ex, const char* fcn, const char* arg, SymbolType type );

#endif // __BUILTIN_HELPERS__
