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

#include <vector>

using namespace std;


namespace deva
{


// reference counting template class for reference types (vectors and maps)
template<typename T> class RefCounted : public T
{
	int refcount;
	// private constructor, don't allow creation except via 'Create()'
	RefCounted() : refcount( 0 ) {}
	// private copy constructor
	RefCounted( T & v ) : T( v ), refcount( 0 ) {}
	// create with 'n' empty items
	RefCounted( size_t n ) : T( n ), refcount( 0 ) {}
	// 'slice' copy constructor
	RefCounted( T & v, size_t start, size_t end ) : T( v, start, end ), refcount( 0 ) {}

	// collection 'pool' of all items to be deleted
	static vector<T*> dead_pool;

public:
	// creation fcn
	static RefCounted<T>* Create() { return new RefCounted<T>(); }
	// copy creation fcn
	static RefCounted<T>* Create( T & v ) { return new RefCounted<T>( v ); }
	// create with 'n' empty items
	static RefCounted<T>* Create( size_t n ) { return new RefCounted<T>( n ); }
	// 'slice' creation fcn
	// TODO: inc ref the copied items!
	static RefCounted<T>* Create( T & v, size_t start, size_t end ) { return new RefCounted<T>( v, start, end ); }

	inline void IncRef() { refcount++; }
	inline int DecRef()
	{
		refcount--;
		int r = refcount;
		if( refcount == 0 )
		{
			dead_pool.push_back( this );
		}
		return r;
	}
	inline int GetRefCount() { return refcount; }

	// clear the dead pool (delete all dead items collected)
	static void ClearDeadPool()
	{
		// this won't compile, not sure why... something about the 'vector<T*>'...
//		for( vector<T*>::iterator i = dead_pool.begin(); i != dead_pool.end(); ++i )
//		{
//			T* p = *i;
//			dead_pool.erase( i );
//			delete p;
//		}
		// ...so do this instead
		for( size_t i = 0; i < dead_pool.size(); i++ )
		{
			T* p = *(dead_pool.begin() + i);
			delete p;
		}
		dead_pool.clear();
	}
};



} // end namespace deva

#endif // __REFCOUNTED_H__

