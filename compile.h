// compile.h
// parse/compile functions for the deva language tools
// created by jcs, september 12, 2009 

// TODO:
// * 

#ifndef __COMPILE_H__
#define __COMPILE_H__

#include "symbol.h"
#include "grammar.h"
#include "debug.h"
#include <fstream>

using namespace std;

tree_parse_info<iterator_t, factory_t> ParseFile( ifstream & file );

#endif // __COMPILE_H__
