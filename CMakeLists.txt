# cmake build file for deva language, v2
# currently tested on
# [x] ubuntu linux 32-bit
# [x] ubuntu linux 64-bit
# [x] darwin 64-bit (Mac OSX 10.6) 
# [x] cygwin 32-bit (Windows 7)
# [x] msvc 64-bit (Windows 7)

# required vars for the user to set:
# all platforms:
#	CMAKE_MODULE_PATH: path where FindANTLR.cmake is located
#	CMAKE_BUILD_TYPE: (Debug/Release) to control the build type
#	CTAGS: on/off for ctags file generation
#	IDUTILS: on/off for id-utils file generation
#	CSCOPE: on/off for cscope file generation
# possibly needed vars for the user to set:
# UNIX/Cygwin:
#	ANTLR: the antlr3c library (e.g. "antlr3c")

# NOTES: 
# for all platforms:
# - java needs to be installed and on the path if the ANTLR grammars are changed
# - ANTLR or ANTLRworks needs to be on the java CLASSPATH
# - Boost 1.4.0+ needs to be installed
# - the ANTLR3 C library and headers need to be installed

# for UNIX/Cygwin:

# for MSVC/Windows:
# - Boost 1.4.0+ needs to be installed and built appropriately and added to the system paths so MSVC can locate it (we're using the 'autolink' features so the toolset should find the appropriate libs to link to)
# - the ANTLR3 C library and headers need to be installed and built appropriately and the system INCLUDE and LIB paths set to point to it


cmake_minimum_required( VERSION 2.6 FATAL_ERROR )
project( deva )
include( FindANTLR )

# enable profiling?
option( PROFILE OFF )

# set flag if this is being generated for Visual Studio (msvc)
##############################################################
if( CMAKE_GENERATOR MATCHES "Visual Studio" )
	option( MSVC ON )
endif()

# find boost
############
if( NOT MSVC )
	find_package( Boost 1.40 COMPONENTS regex filesystem system program_options )
	set( BOOST_ROOT "${BOOST_ROOT}" CACHE PATH "Root directory of Boost install" )
	set( Boost_USE_STATIC_LIBS OFF )
	set( Boost_USE_MULTITHREADED ON )
	set( Boost_USE_STATIC_RUNTIME OFF )
	find_package( Boost 1.40.0 REQUIRED COMPONENTS filesystem system program_options )
	if( Boost_FOUND )
		include_directories( ${Boost_INCLUDE_DIRS} )
	endif()
endif()

# find antlr3
#############
find_package( ANTLR REQUIRED )
if( ANTLR_FOUND )
	include_directories( ${ANTLR_INCLUDE_DIR} )
endif()

# find libedit
##############
# TODO: libedit on (non-cygwin) ms-windows??
find_library( EDIT edit )

# configure defines and options
###############################
if( ${UNIX} )
	set( CMAKE_CXX_FLAGS "-DDEVA_VERSION=\\\"2.0.0-alpha\\\"" )
	if( PROFILE )
		set( CMAKE_CXX_FLAGS_DEBUG "-g -Wall -DDEBUG -DREFCOUNT_TRACE -pg" )
		#set( CMAKE_CXX_FLAGS_RELEASE "-O2" )
		set( CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -pg" )
	else()
		set( CMAKE_CXX_FLAGS_DEBUG "-g -Wall -DDEBUG -DREFCOUNT_TRACE" )
		#set( CMAKE_CXX_FLAGS_RELEASE "-O2" )
		set( CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math" )
	endif()
endif()
if( MSVC )
	set( CMAKE_CXX_FLAGS "-DMS_WINDOWS -DDEVA_VERSION=\\\"2.0.0-alpha\\\" -MD -EHsc" )
	set( CMAKE_CXX_FLAGS_DEBUG "-Od -Zi -DDEBUG -DREFCOUNT_TRACE" )
	set( CMAKE_CXX_FLAGS_RELEASE "-O2" )
endif()

# include dirs
##############
include_directories( "inc" )
# need the dir for ANTLR generated h files
include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )

# link dirs
###########
link_directories( ${Boost_LIBRARY_DIRS} )

# custom command to build antlr grammars
########################################
add_custom_command(
	OUTPUT 
		${CMAKE_CURRENT_BINARY_DIR}/devaParser.c ${CMAKE_CURRENT_BINARY_DIR}/devaParser.h
		${CMAKE_CURRENT_BINARY_DIR}/devaLexer.c ${CMAKE_CURRENT_BINARY_DIR}/devaLexer.h 
		${CMAKE_CURRENT_BINARY_DIR}/deva.tokens
	COMMAND java org.antlr.Tool -message-format gnu deva.g
	MAIN_DEPENDENCY deva.g
)
add_custom_command(
	OUTPUT 
		${CMAKE_CURRENT_BINARY_DIR}/semantic_walker.c ${CMAKE_CURRENT_BINARY_DIR}/semantic_walker.h
	COMMAND java org.antlr.Tool -message-format gnu semantic_walker.g
	MAIN_DEPENDENCY semantic_walker.g
	DEPENDS deva.tokens
)
add_custom_command(
	OUTPUT 
		${CMAKE_CURRENT_BINARY_DIR}/compile_walker.c ${CMAKE_CURRENT_BINARY_DIR}/compile_walker.h
	COMMAND java org.antlr.Tool -message-format gnu compile_walker.g
	MAIN_DEPENDENCY compile_walker.g
	DEPENDS deva.tokens
)

# force the antlr-generated .c files to build as c++
set_source_files_properties( devaLexer.c devaParser.c semantic_walker.c compile_walker.c PROPERTIES LANGUAGE CXX )

# main library
##############
add_library( deva-rt SHARED
	src/opcodes.cpp
	src/scope.cpp
	src/semantics.cpp
	src/util.cpp
	src/error.cpp
	src/compile.cpp
	src/executor.cpp
	src/object.cpp
	src/api.cpp
	src/builtins.cpp
	src/vector_builtins.cpp
	src/string_builtins.cpp
	src/builtins_helpers.cpp
	src/map_builtins.cpp
	src/module_os.cpp
	src/module_bit.cpp
	src/module_math.cpp
	src/module_re.cpp
	src/frame.cpp
	src/scopetable.cpp
	devaLexer.c
	devaParser.c
	semantic_walker.c
	compile_walker.c
)

# main executable
#################
add_executable( deva 
	src/deva.cpp
)

# shell executable
##################
if( DEVASH )
	add_executable( devash
		src/devash.cpp
	)
endif()

# add libraries
###############
target_link_libraries( deva deva-rt )
if( DEVASH )
	target_link_libraries( devash deva-rt )
	target_link_libraries( devash ${EDIT} )
endif()
if( ANTLR_FOUND )
	if( ${UNIX} )
		target_link_libraries( deva ${ANTLR})
		if( DEVASH )
			target_link_libraries( devash ${ANTLR})
		endif()
	else()
		target_link_libraries( deva ${ANTLR_LIBRARIES})
		if( DEVASH )
			target_link_libraries( devash ${ANTLR_LIBRARIES})
		endif()
	endif()
endif()
if( Boost_FOUND )
	target_link_libraries( deva ${Boost_LIBRARIES} )
	if( DEVASH )
		target_link_libraries( devash ${Boost_LIBRARIES} )
	endif()
endif()

# custom commands to build ID, tags and cscope databases
########################################################
if( CTAGS )
	add_custom_command(
		TARGET deva POST_BUILD
		COMMAND ctags -R --exclude=deva1 --exclude=tests --exclude=antlr
		WORKING_DIRECTORY ${DEVA_SOURCE_DIR}
		COMMENT "Building tags database"
		VERBATIM
	)
endif()
if( IDUTILS )
	add_custom_command(
		TARGET deva POST_BUILD
		COMMAND mkid -i "C++" --prune=deva1 --prune=tests --prune=antlr
		WORKING_DIRECTORY ${DEVA_SOURCE_DIR}
		COMMENT "Building ID database"
		VERBATIM
	)

endif()
if( CSCOPE )
	add_custom_command(
		TARGET deva POST_BUILD
		COMMAND cscope -b -I inc -s src devaParser.c
		WORKING_DIRECTORY ${DEVA_SOURCE_DIR}
		COMMENT "Building cscope database"
		VERBATIM
	)
endif()

# 'install' target
##################
install( TARGETS deva RUNTIME DESTINATION bin )
install( TARGETS deva-rt LIBRARY DESTINATION lib )

# install man page
##################
if( ${UNIX} )
	install( FILES deva.1 DESTINATION share/man/man1/ )
endif()

# setup package generation for .deb
##########################################
set( CPACK_PACKAGE_DESCRIPTION "The deva programming language" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "The deva programming language is a byte-code compiled language similar in syntax to C and bearing semantic similarities to Python. It is written in C++, and designed to be embedded into C++ programs. The guiding principle behind its design is to be as simple as possible, both to implement and to write code in." )
set( CPACK_PACKAGE_NAME "deva" )
set( CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.4), libgcc1 (>= 1:4.1), libstdc++6 (>= 4.4), libboost-dev (>= 1.40)" )
set( CPACK_PACKAGE_CONTACT "Joshua Shepard " )
set( CPACK_PACKAGE_VENDOR "" )
set( CPACK_PACKAGE_VERSION_MAJOR "2" )
set( CPACK_PACKAGE_VERSION_MINOR "0" )
set( CPACK_PACKAGE_VERSION_PATCH "0" )
set( VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" )
#set( CPACK_GENERATOR "DEB;RPM;" )
set( CPACK_GENERATOR DEB )
set( CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-${CMAKE_SYSTEM_PROCESSOR}" )
set( CPACK_BINARY_BUNDLE "" )
set( CPACK_BINARY_CYGWIN "" )
set( CPACK_BINARY_DEB "ON" )
set( CPACK_BINARY_DRAGNDROP "" )
set( CPACK_BINARY_NSIS "OFF" )
set( CPACK_BINARY_OSXX11 "" )
set( CPACK_BINARY_PACKAGEMAKER "" )
set( CPACK_BINARY_RPM "OFF" )
set( CPACK_BINARY_STGZ "OFF" )
set( CPACK_BINARY_TBZ2 "OFF" )
set( CPACK_BINARY_TGZ "OFF" )
set( CPACK_BINARY_TZ "OFF" )
set( CPACK_BINARY_ZIP "" )


include( CPack )
