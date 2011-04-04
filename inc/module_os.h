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

// module_os.h
// native os module for deva language, v2
// created by jcs, april 2, 2011

// TODO:
// * 

#ifndef __MODULE_OS_H__
#define __MODULE_OS_H__

#include "object.h"
#include <string>

using namespace std;


namespace deva
{


// pre-decls:
class Frame;

// pre-decls for builtin executors
void do_os_exec( Frame* f );
void do_os_getcwd( Frame* f );
void do_os_chdir( Frame* f );
void do_os_splitpath( Frame* f );
void do_os_joinpaths( Frame* f );
void do_os_getdir( Frame* f );
void do_os_getfile( Frame* f );
void do_os_getext( Frame* f );
void do_os_exists( Frame* f );
void do_os_environ( Frame* f );
void do_os_getenv( Frame* f );
void do_os_argv( Frame* f );
void do_os_dirwalk( Frame* f );
void do_os_isdir( Frame* f );
void do_os_isfile( Frame* f );
void do_os_sep( Frame* f );
void do_os_extsep( Frame* f );
void do_os_curdir( Frame* f );
void do_os_pardir( Frame* f );
void do_os_pathsep( Frame* f );

extern const string module_os_names[];
// ...and function pointers to the executor functions for them
extern NativeFunction module_os_fcns[];
extern const int num_of_module_os_fcns;

// is a given name a builtin function?
bool IsModuleOsFunction( const string & name );
NativeFunction GetModuleOsFunction( const string & name );


} // namespace deva

#endif // __MODULE_OS_H__
