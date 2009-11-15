dn1 Process this file with autoconf to produce a configure script
AC_PREREQ(2.53)
AC_INIT(src/deva.cpp)
AM_INIT_AUTOMAKE(deva, 0.1.00)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_HEADER_STDC
AC_HEADER_TIME
# I can't get this to work, it always complains that it can't find them
# even though it can find something like 'sys/time.h' just fine if I add that...
#AC_CHECK_HEADERS(boost/program_options/options_description.hpp boost/program_options/positional_options.hpp boost/program_options/variables_map.hpp boost/program_options/parsers.hpp boost/filesystem.hpp boost/spirit.hpp boost/spirit/include/classic_functor_parser.hpp boost/spirit/include/classic_ast.hpp boost/spirit/include/classic_parse_tree.hpp,,echo "Error: requires Boost. You can find the Boost libraries at www.boost.org" && exit,-)
AC_CHECK_LIB(boost_filesystem,_ZN5boost10filesystem6detail10rename_apiERKSsS3_,,echo "Error: requires Boost. You can find the Boost libraries at www.boost.org"; exit)
AC_CHECK_LIB(boost_program_options,_ZNK5boost15program_options11typed_valueIbcE13apply_defaultERNS_3anyE,,echo "Error: requires Boost. You can find the Boost libraries at www.boost.org"; exit)
AC_OUTPUT(src/Makefile Makefile)
