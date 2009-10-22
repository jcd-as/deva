// fileformat.cpp
// file format (file header etc) for deva compiled bytecode files
// created by jcs, october 21, 2009 

// TODO:
// * 

#include "fileformat.h"

// define the static members of the FileHeader struct
const char FileHeader::deva[5] = "deva";
const char FileHeader::ver[6] = "1.0.0";
const char FileHeader::pad[5] = "\0\0\0\0";

