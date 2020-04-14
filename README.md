THE DEVA LANGUAGE:


INTRODUCTION:
=============================================================================
This package includes the source code to build the deva language compiler,
execution engine (byte-code interpreter) and tools. Deva is written in C++ as
a small and simple language to be embedded in C++ programs or to write
stand-alone scripts with. It is not intended to compete with major languages
such as Python or Ruby in the richness of its feature set and power or
expressibility. Instead it is intended to be as small and simple as practical
while still offering a reasonably powerful and expressive multi-paradigm 
language.

Deva is basically a procedural imperative language, but includes support for
simple object-oriented and functional programming techniques.

See the document 'deva-language-2.0.odt' for details on the language.


STATUS
=============================================================================
Deva is in a late alpha state. It is basically functional (the test harness
for the test suite is written in deva, for instance), but there are a few
missing features and (presumably) many bugs to be found.

Deva includes a simple interactive shell (devash) and debugger (devadb). They
are not complete or bug-free, but I have found them useful nonetheless.

Deva is/was a learning experiment by myself and for myself. I do not recommend
it for any particular purpose, though it may be of some use as an example of
how a small language might be built in C++ using ANTLR. I have used it for
many small scripting tasks and found it suitable, though clearly it lacks the
breadth of libraries of a language like Python, Perl, Ruby et al.


PRE-REQUISITES:
=============================================================================
In order to build the deva language tools you will need to have: 
(On all platforms):

- the GCC C++ toolset (4.3+). GCC is standard on most Unix-like systems; you 
can find information at gcc.gnu.org. 

- CMake (www.cmake.org). Most Unix systems can install via their package
manager; Homebrew or MacPorts work on Mac OS X.

- Boost C++ libraries (version 1..4.6 or newer). Boost is available from 
www.boost.org if not from your operating system vendor. (For instance, most 
Linux and BSD distributions include a package manager tool to download and 
install packages such as g++ and Boost).

- Java needs to be installed and on the path if the ANTLR grammars are changed

- ANTLR or ANTLRworks needs to be on the java CLASSPATH (www.antlr.org)

- The ANTLR3 C library and headers need to be installed


(On MSVC/Windows):

- Boost 1.4.6+ needs to be installed, built appropriately and added to the 
system paths so MSVC can locate it (we're using the 'autolink' features so 
the toolset should find the appropriate libs to link to)

- The ANTLR3 C library and headers need to be installed and built appropriately 
and the system INCLUDE and LIB paths set to point to it


I regularly build deva and run the test suites on Ubuntu Linux and Mac OS X 
10.6 ('Snow Leopard'). In theory it is portable to any POSIX system, including 
Cygwin on MS Windows, but I have not tried it on any other systems.

I have tested the (CMake) build on:
- Ubuntu Linux 32-bit
- Ubuntu Linux 64-bit
- Darwin 64-bit (Mac OSX 10.6) 
- Cygwin 32-bit (Windows 7)
- MSVC 64-bit (Windows 7)


In order to build some environment variables will need to be set:

Required on all platforms:
  CMAKE_MODULE_PATH: path where FindANTLR.cmake is located
  CMAKE_BUILD_TYPE: (Debug/Release) to control the build type
  CTAGS: on/off for ctags file generation
  IDUTILS: on/off for id-utils file generation
  CSCOPE: on/off for cscope file generation

Possibly needed on UNIX/Cygwin:
  ANTLR: the antlr3c library (e.g. "antlr3c")



INSTALLATION:
=============================================================================
Deva is built using CMake. See the CMakeLists.txt file for details on the
CMake targets and build. Their are targets to install, create a man page and
generate a .deb file (for Debian-based Linux systems).

This will build the deva executables (deva and devac) and install them into the 
base deva directory. 'devac' is a deva byte-code compiler that will compile
'.dv' source files into byte-code compiled '.dvc' files. 'deva' is the language
execution engine. It will run either compiled '.dvc' files or source files  or
'.dv' source files (by first compiling them and then executing them).

For instance, to compile and run the file 'my_code.dv', execute the following
at the command line:

<path_to_deva>/deva my_code.dv

where <path_to_deva> is replaced with the actual directory that you installed 
deva on your machine. (If you added this to your system's PATH then obviously
you won't need to specify the directory, but the deva build/install process
does not place deva into your path).


ADDITIONAL STEPS:
=============================================================================
In order for the deva executables to locate the files they need (library 
modules etc.) and for the test suite to run, you need to set the DEVA 
environment variable to point to the location where you installed. For 
example, my .bashrc contains the following line:

export DEVA=/home/jcs/src/deva

(as /home/jcs/src/deva is the directory where I have deva installed).


TESTS:
=============================================================================
To run the deva test suite you first must have the DEVA environment variable
set, as above. Then simply run:

deva runtests.dv

This will run the entire test suite.

If you have valgrind installed, you can run the test suite under valgrind.
From the deva directory execute:

./deva runtests.dv valgrind

Warning! Valgrind causes applications to run very slowly, so be prepared to 
wait if you run the tests under it. All code should
run valgrind clean.

