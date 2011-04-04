// Copyright (c) 2011 Jbithua C. Shepard
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

// module_bit.h
// native bit operations module for deva language, v2
// created by jcs, april 2, 2011

// TODO:
// * 

#ifndef __MODULE_BIT_H__
#define __MODULE_BIT_H__

#include "object.h"
//#include <string>

using namespace std;


namespace deva
{


// pre-decls:
class Frame;

// pre-decls for builtin executors
void do_bit_and( Frame* f );
void do_bit_or( Frame* f );
void do_bit_xor( Frame* f );
void do_bit_complement( Frame* f );
void do_bit_shift_left( Frame* f );
void do_bit_shift_right( Frame* f );

extern const string module_bit_names[];
// ...and function pointers to the executor functions for them
extern NativeFunction module_bit_fcns[];
extern Object module_bit_fcn_objs[];
extern const int num_of_module_bit_fcns;

// is a given name a builtin function?
bool IsModuleBitFunction( const string & name );
NativeFunction GetModuleBitFunction( const string & name );


} // namespace deva

#endif // __MODULE_BIT_H__

