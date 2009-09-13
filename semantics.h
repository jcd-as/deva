// semantics.h
// semantic analysis functions for the deva language
// created by jcs, september 12, 2009 

// TODO:
// * 

#ifndef __SEMANTICS_H__
#define __SEMANTICS_H__

#include "types.h"

// declare semantic analysis functions
void check_func( iter_t const & i );
void check_while_s( iter_t const & i );

#endif // __SEMANTICS_H__

