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

// linemap.h
// line map class for the deva language
// created by jcs, april 11, 2011

// TODO:
// * 

#ifndef __LINEMAP_H__
#define __LINEMAP_H__

#include <map>

using namespace std;

namespace deva
{

// map lines to addresses and addresses to lines
// (specifically to the address of the *first* instruction found on the line)
class LineMap
{
private:
	map<dword, dword> l2a;
	map<dword, dword> a2l;

public:
	static const dword end = (dword)-1;

	LineMap() {}

	void Add( dword line, dword addr )
	{
		map<dword, dword>::iterator i = l2a.find( line );
		// if an entry already exists for this line...
		if( i != l2a.end() )
		{
			// ...and it is for a larger address
			if( i->second > addr )
			{
				// erase the old entry
				l2a.erase( i );
				// and add the new ones
				l2a.insert( make_pair( line, addr ) );
				a2l.insert( make_pair( addr, line ) );
			}
		}
		// otherwise just add the entries
		else
		{
			l2a.insert( make_pair( line, addr ) );
			a2l.insert( make_pair( addr, line ) );
		}
	}
	
	dword FindAddress( dword line )
	{
		map<dword,dword>::iterator i = l2a.find( line );
		if( i == l2a.end() )
			return LineMap::end;
		else return i->second;
	}

	// TODO: this actually needs to look *between* the addresses we know about
	// (in the map) to find the correct line
	dword FindLine( dword addr )
	{
//		map<dword,dword>::iterator i = a2l.find( addr );
//		if( i == a2l.end() )
//			return LineMap::end;
//		else return i->second;

		dword line = 0;
		for( map<dword, dword>::iterator i = a2l.begin(); i != a2l.end(); ++i )
		{
			dword a = i->first;
//			if( a >= addr )
			if( a > addr )
				return line;
			else
				line = i->second;
		}
		return line;
	}
};


} // namespace deva

#endif // __LINEMAP_H__
