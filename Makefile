# makefile for test driver for deva language grammars

# sources/objs for test executable
TEST_SOURCES=test.cpp symbol.cpp parser_ids.cpp error_report_parsers.cpp dobject.cpp
TEST_OBJS=$(patsubst %.cpp, %.o, ${TEST_SOURCES})
# sources/objs for devac executable
DEVAC_SOURCES=devac.cpp symbol.cpp fileformat.cpp debug.cpp parser_ids.cpp error_report_parsers.cpp scope.cpp compile.cpp semantics.cpp instructions.cpp dobject.cpp
DEVAC_OBJS=$(patsubst %.cpp, %.o, ${DEVAC_SOURCES})
# sources/objs for deva executable
DEVA_SOURCES=deva.cpp symbol.cpp fileformat.cpp error_report_parsers.cpp scope.cpp executor.cpp instructions.cpp parser_ids.cpp dobject.cpp builtins.cpp vector_builtins.cpp compile.cpp semantics.cpp util.cpp
DEVA_OBJS=$(patsubst %.cpp, %.o, ${DEVA_SOURCES})
# dependency files (header dependencies)
TEST_DEP_FILES=$(patsubst %.cpp, %.dep, ${TEST_SOURCES})
DEVAC_DEP_FILES=$(patsubst %.cpp, %.dep, ${DEVAC_SOURCES})
DEVA_DEP_FILES=$(patsubst %.cpp, %.dep, ${DEVA_SOURCES})

# source directories:
#VFILES=

CPPFLAGS = -c -g
LDFLAGS = -g

all : tags ID test devac deva

tags : devac deva
	ctags -R 

ID : devac deva
	mkid -i "C++"

test : ${TEST_OBJS}
	g++ ${LDFLAGS} -o test ${TEST_OBJS}

devac : ${DEVAC_OBJS}
	g++ ${LDFLAGS} -lboost_program_options -o devac ${DEVAC_OBJS}
	#g++ ${LDFLAGS} -lboost_program_options-mt -o devac ${DEVAC_OBJS}

deva : ${DEVA_OBJS}
	g++ ${LDFLAGS} -lboost_program_options -o deva ${DEVA_OBJS}
	#g++ ${LDFLAGS} -lboost_program_options-mt -o deva ${DEVA_OBJS}

%.o : %.cpp
	g++ ${CPPFLAGS} -o $@ $<

clean:
	rm *.o test devac deva *.dep*

# run tests
check:
	./runtests

include ${TEST_DEP_FILES}
include ${DEVAC_DEP_FILES}
include ${DEVA_DEP_FILES}

%.dep : %.cpp
	@set -e; rm -f $@; \
	gcc -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


