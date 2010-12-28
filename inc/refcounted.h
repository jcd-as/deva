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

// refcounted.h
// template type for ref-counted objects, for the deva language
// created by jcs, december 28, 2010 

// TODO:
// * 

#ifndef __REFCOUNTED_H__
#define __REFCOUNTED_H__


namespace deva
{


// reference counting template class for reference types (vectors and maps)
template<typename T> class RefCounted : public T
{
	int refcount;
	// private constructor, don't allow creation except via 'Create()'
	RefCounted() { refcount = 1; }
	// private copy constructor
	RefCounted( T & v ) : T( v ), refcount( 1 ) {}
	// create with 'n' empty items
	RefCounted( size_t n ) : T( n ), refcount( 1 ) {}
	// 'slice' copy constructor
	RefCounted( T & v, size_t start, size_t end ) : T( v, start, end ), refcount( 1 ) {}
public:
	// creation fcn
	static RefCounted<T>* Create() { return new RefCounted<T>(); }
	// copy creation fcn
	static RefCounted<T>* Create( T & v ) { return new RefCounted<T>( v ); }
	// create with 'n' empty items
	static RefCounted<T>* Create( int n ) { return new RefCounted<T>( n ); }
	// 'slice' creation fcn
	static RefCounted<T>* Create( T & v, size_t start, size_t end ) { return new RefCounted<T>( v, start, end ); }

	void IncRef() { refcount++; }
	int DecRef() { refcount--; int r = refcount; if( refcount == 0 ) delete this; return r; }
};


} // end namespace deva

#endif // __REFCOUNTED_H__

