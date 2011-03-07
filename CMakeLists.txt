# cmake build file for deva language, v2
# currently tested on
# [x] ubuntu linux 32-bit
# [ ] ubuntu linux 64-bit
# [ ] darwin 64-bit (Mac OSX 10.6) 
# [x] cygwin 64-bit (Windows 7)

cmake_minimum_required( VERSION 2.6 FATAL_ERROR )
project( deva )

# find boost
#find_package( Boost 1.40 COMPONENTS regex filesystem system program_options )
find_package( Boost 1.40 COMPONENTS filesystem system program_options )

# find antlr3
find_library( ANTLR antlr3c )

# TODO: configure for debug and release builds
# configure defines
add_definitions( -g -DDEVA_VERSION="2.0.0-alpha" -DDEBUG -DREFCOUNT_TRACE )
#add_definitions( -DDEVA_VERSION="2.0.0-alpha" )
#set( CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -DREFCOUNT_TRACE" )
#set( CMAKE_CXX_FLAGS_RELEASE "-O2" )

# include dirs
include_directories( "inc" )
include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )
link_directories( ${Boost_INCLUDE_DIRS} )

# link dirs
link_directories( ${Boost_LIBRARY_DIRS} )

# custom command to build antlr grammars
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
target_link_libraries( deva antlr3c ${Boost_LIBRARIES} )

# custom commands to build ID and tags databases
add_custom_command(
	TARGET deva POST_BUILD
	COMMAND ctags -R --exclude=deva1 --exclude=tests --exclude=antlr
	WORKING_DIRECTORY ${DEVA_SOURCE_DIR}
	COMMENT "Building tags database"
	VERBATIM
)

add_custom_command(
	TARGET deva POST_BUILD
	COMMAND mkid -i "C++" --prune=deva1 --prune=tests --prune=antlr
	WORKING_DIRECTORY ${DEVA_SOURCE_DIR}
	COMMENT "Building ID database"
	VERBATIM
)

add_custom_command(
	TARGET deva POST_BUILD
	COMMAND cscope -b -I inc -s src devaParser.c
	WORKING_DIRECTORY ${DEVA_SOURCE_DIR}
	COMMENT "Building cscope database"
	VERBATIM
)
