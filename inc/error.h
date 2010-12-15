// Copyright (c) 2010 Joshua C. Shepard
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

// error.h
// error reporting for the deva language
// created by jcs, december 09, 2010 

// TODO:
// * 

#ifndef __ERROR_H__
#define __ERROR_H__

#include <antlr3.h>
#include "exceptions.h"

// global object to track current filename
extern const char* current_file;

void emit_error( DevaSemanticException & e );
void emit_error( DevaICE & e );

void emit_warning( char* warning, int line );

// override for ANTLR parser error
void devaDisplayRecognitionError( pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames );


#endif // __ERROR_H__
