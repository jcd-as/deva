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

// module_math.h
// native math operations module for deva language, v2
// created by jcs, april 3, 2011

// TODO:
// * 

#ifndef __MODULE_MATH_H__
#define __MODULE_MATH_H__

#include "object.h"

using namespace std;


namespace deva
{


// pre-decls:
class Frame;

// pre-decls for builtin executors
void do_math_cos( Frame* f );
void do_math_sin( Frame* f );
void do_math_tan( Frame* f );
void do_math_acos( Frame* f );
void do_math_asin( Frame* f );
void do_math_atan( Frame* f );
void do_math_cosh( Frame* f );
void do_math_sinh( Frame* f );
void do_math_tanh( Frame* f );
void do_math_exp( Frame* f );
void do_math_log( Frame* f );
void do_math_log10( Frame* f );
void do_math_abs( Frame* f );
void do_math_sqrt( Frame* f );
void do_math_pow( Frame* f );
void do_math_modf( Frame* f );
void do_math_fmod( Frame* f );
void do_math_floor( Frame* f );
void do_math_ceil( Frame* f );
void do_math_pi( Frame* f );
void do_math_radians( Frame* f );
void do_math_degrees( Frame* f );
void do_math_round( Frame* f );

extern const string module_math_names[];
// ...and function pointers to the executor functions for them
extern NativeFunction module_math_fcns[];
extern const int num_of_module_math_fcns;

// is a given name a builtin function?
bool IsModuleMathFunction( const string & name );

struct NativeModule;
NativeModule* GetModuleMath();

} // namespace deva

#endif // __MODULE_MATH_H__


