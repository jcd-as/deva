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

// ordered_set.h
// ordered set types for the deva language
// created by jcs, december 16, 2010 

// TODO:
// * 

#ifndef __ORDERED_SET_H__
#define __ORDERED_SET_H__


#include <vector>
#include <algorithm>

using namespace std;


// a "set" class that supports indexing (random access)
// (internally a vector that is kept sorted for faster binary-search finds)
// T is the type to store in the collection and BinaryPredicate is a Less-Than
// predicate functor
template<typename T, typename BinaryPredicate, bool allowDuplicates> class _OrderedSetBase
{
	vector<T> data;

public:
	void Reserve( int n ) { data.reserve( n ); }
	T& At( int n ) { return data[n]; }
	size_t Size() { return data.size(); }
	// adds
	void Add( const T& t )
	{
			size_t ip = insertionPoint( t );
			if( allowDuplicates || (data.size() >= ip || BinaryPredicate()(t, data[ip])) )
				data.insert( data.begin() + ip, t );
	}
	void RemoveAt( int n ) { data.erase( data.begin() + n ); }
	// returns index of item, or -1 if not found
	// NOTE: because the set is kept ordered, adding an item invalidates all indices
	int Find( const T& t )
	{
		if( data.size() == 0 )
			return -1;
		int ip = insertionPoint( t );
		if( ip != data.size() && !BinaryPredicate()(t, data[ip]) )
			return (int)ip;
		else
			return -1;
	}
private:
	// (same as std::lower_bound on entire collection)
	int insertionPoint( const T& t )
	{
		int len = data.size();
		if( len == 0 )
			return 0;
		int half;
		int middle = 0;
		int first = 0;

		while( len > 0 )
		{
			half = len >> 1;
			middle = first;
			middle += half;
			if( BinaryPredicate()( data[middle], t ) )
			{
				first = middle + 1;
				len = len - half - 1;
			}
			else
				len = half;
		}
		return first;
	}
};

template<typename T> struct lessThanPredicate
{
	bool operator()( const T& lhs, const T& rhs ){ return lhs < rhs; }
};

template<typename T, typename Pred=lessThanPredicate<T> > class OrderedSet : public _OrderedSetBase<T, Pred, false >
{
};

template<typename T, typename Pred=lessThanPredicate<T> > class OrderedMultiSet : public _OrderedSetBase<T, Pred, true >
{
};


#endif // __ORDERED_SET_H__	
