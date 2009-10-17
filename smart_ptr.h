// smart_ptr.h
// reference-counting smart pointer
// created by jcs, October 16, 2009 
// based on code by Nicholas A. Solter and Scott J. Kleper
// NOTE: this is used instead of TR1/boost shared_ptr because this stores the
// ref counts in a map that can be examined to determine what (if anything) is
// not getting destroyed. perhaps there is a good way to do this with shared_ptr
// that i'm not aware of...?

#ifndef __SMART_PTR_H__
#define __SMART_PTR_H__

#include <map>
#include <iostream>
#include <algorithm>
#include "exceptions.h"

using namespace std;

template <typename T> class smart_ptr
{
public:
	smart_ptr() : ptr( 0 )
	{ }
	explicit smart_ptr( T* in_ptr ) : ptr( 0 )
	{
		initPointer( in_ptr );
	}
	~smart_ptr()
	{
		finalizePointer();
	}

	smart_ptr( const smart_ptr<T>& src ) : ptr( 0 )
	{
		initPointer( src.ptr );
	}

	smart_ptr<T>& operator=( T* in_ptr )
	{
		// can ONLY be assigned to when it is EMPTY (i.e. created on a NULL ptr)
		if( ptr )
			throw DevaICE( "non-empty smart_ptr being assigned to!" );

		initPointer( in_ptr );
	}

	smart_ptr<T>& operator=( const smart_ptr<T>& rhs )
	{
		if( &rhs == this)
		{
			return (*this);
		}
		finalizePointer();
		initPointer( rhs.ptr );

		return (*this);
	}

	const T& operator*() const
	{
		return (*ptr);
	}
	const T* operator->() const
	{
		return (ptr);
	}
	T& operator*()
	{
		return (*ptr);
	}
	T* operator->()
	{
		return (ptr);
	}

	operator bool() const
	{
		return ptr != 0;
	}

	operator void*() const
	{
		return ptr;
	}

protected:
	T* ptr;
	static map<T*, int> s_ref_count_map;

	void finalizePointer()
	{
		// ignore finalization of NULL
		if( !ptr )
			return;

		if( s_ref_count_map.find( ptr ) == s_ref_count_map.end() )
		{
			throw DevaICE( "Missing entry in reference count map." );
		}
		s_ref_count_map[ptr]--;
		if( s_ref_count_map[ptr] == 0 )
		{
			// no No more references to this object -- delete it and remove from map
			s_ref_count_map.erase( ptr );
			delete ptr;
			ptr = 0;
		}
	}
	void initPointer( T* in_ptr )
	{
		// if we're initialized with a NULL value, don't bother adding an entry
		// to the map, we're either never going to be set with any real value,
		// or the = op will be called and will init us with a real value
		if( !in_ptr )
			return;
		
		ptr = in_ptr;
		if( s_ref_count_map.find( ptr ) == s_ref_count_map.end() )
		{  
			s_ref_count_map[ptr] = 1;
		}
		else
		{
			s_ref_count_map[ptr]++;
		}
	}

	// static helper fcn to dump the ref count map. for use in
	// debugging/tracking memory
	static void printPair( pair<T* const, int> const p )
	{
		cout << "ptr: " << p.first << " -- ref-count: " << p.second << endl;
	}
public:
	static void dumpRefCountMap()
	{
		for_each( s_ref_count_map.begin(), s_ref_count_map.end(), printPair );
	}
};

template <typename T> map<T*, int>smart_ptr<T>::s_ref_count_map;

#endif // __SMART_PTR_H__
