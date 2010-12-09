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

// debug.h
// debug functions for deva language compiler
// created by jcs, september 9, 2009

// TODO:
// * 

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "symbol.h"
#include "grammar.h"

// utility fcns:
////////////////////////
void indent( int ind );
// output whether something succeeded or failed
int print_parsed( bool parsed );


// AST evaluation fcns:
////////////////////////
void dumpNode( const char* name, iter_t const& i, int & indents );
void eval_expression( iter_t const& i );
void evaluate( tree_parse_info<iterator_t, factory_t> info );


#endif // __DEBUG_H__
