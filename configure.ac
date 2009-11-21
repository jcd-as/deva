dn1 Process this file with autoconf to produce a configure script
AC_PREREQ(2.53)
AC_INIT(src/deva.cpp)
AM_INIT_AUTOMAKE(deva, 0.9.1)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_SED
AC_HEADER_STDC
AC_HEADER_TIME
AX_BOOST_BASE([1.36])
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_FILESYSTEM
#if [ test -z "$HAVE_BOOST_PROGRAM_OPTIONS" ] ; then
#	echo "Boost program options library is required. You can find Boost at www.boost.org."
#	exit
#fi
#if [ test $HAVE_BOOST_FILESYSTEM ] ; then
#	echo "Boost filesystem library is required. You can find Boost at www.boost.org."
#	exit
#fi
AC_SUBST(MY_CXXFLAGS)
AC_SUBST(MY_LDFLAGS)
AX_CXX_CHECK_FLAG(-fno-inline-small-functions,,,export MY_CXXFLAGS=-fno-inline-small-functions,)
AX_LD_CHECK_FLAG(-undefined dynamic_lookup,,,export MY_LDFLAGS="-undefined dynamic_lookup",)
AC_OUTPUT(src/Makefile Makefile)
