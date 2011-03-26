# cmake build file for deva language, v2
# currently tested on
# [x] ubuntu linux 32-bit
# [x] ubuntu linux 64-bit
# [ ] darwin 64-bit (Mac OSX 10.6) 
# [x] cygwin 32-bit (Windows 7)
# [x] msvc 64-bit (Windows 7)

# ONLY TESTED WITH IN-SOURCE BUILDS

# required vars for the user to set:
# all platforms:
#	CMAKE_MODULE_PATH: path where FindANTLR.cmake is located
#	CTAGS: on/off for ctags file generation
#	IDUTILS: on/off for id-utils file generation
#	CSCOPE: on/off for cscope file generation
# VisualStudio/MSVC on Windows:
#	MSVC: on

# NOTES: 
# for all platforms:
# - the java sdk needs to be installed and on the path if the ANTLR grammars are changed
# - ANTLR or ANTLRworks needs to be on the java CLASSPATH
# - Boost 1.4.0+ needs to be installed
# - the ANTLR3 C library and headers need to be installed

# for UNIX/Cygwin:

# for MSVC/Windows:
# - Boost 1.4.0+ needs to be installed and built appropriately and added to the system paths so MSVC can locate it (we're using the 'autolink' features so the toolset should find the appropriate libs to link to)
# - the ANTLR3 C library and headers need to be installed and built appropriately and the system INCLUDE and LIB paths set to point to it

# TODO / FUTURE: 
# [ ] windows 64-bit (Visual C++)

cmake_minimum_required( VERSION 2.6 FATAL_ERROR )
project( deva )
include( FindANTLR )

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

# configure defines
###################
if( ${UNIX} )
	#add_definitions( -g -DDEVA_VERSION="2.0.0-alpha" -DDEBUG -DREFCOUNT_TRACE )
	set( CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -DREFCOUNT_TRACE" )
	set( CMAKE_CXX_FLAGS_RELEASE "-O2" )
endif()
if( MSVC )
	set( CMAKE_CXX_FLAGS "-DMS_WINDOWS -MD -EHsc" )
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

# main executable
#################
add_executable( deva 
	src/deva.cpp
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
	src/frame.cpp
	src/scopetable.cpp
	devaLexer.c
	devaParser.c
	semantic_walker.c
	compile_walker.c
)

# add libraries
###############
if( ANTLR_FOUND )
	target_link_libraries( deva ${ANTLR_LIBRARIES})
endif()
if( Boost_FOUND )
	target_link_libraries( deva ${Boost_LIBRARIES} )
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
