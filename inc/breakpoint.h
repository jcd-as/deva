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

// breakpoint.h
// breakpoint struct for the deva language
// created by jcs, may 23, 2011

// TODO:
// * 

#ifndef __BREAKPOINT_H__
#define __BREAKPOINT_H__

namespace deva
{


class Module;

struct Breakpoint
{
	bool is_valid;
	bool is_active;
	Module* module;
	int line;
	byte* location;

	Breakpoint() : is_valid( false ), is_active( false ), module( NULL ), line( -1 ), location( NULL ) { }
	Breakpoint( Module* mod, int l, byte* loc ) : is_valid( true ), is_active( false ), module( mod ), line (l ), location( loc ) { }

	void Activate() { is_active = true; }
	void Deactivate() { is_active = false; }
};


} // end namespace deva

#endif // __BREAKPOINT_H__
