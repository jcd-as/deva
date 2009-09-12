# makefile for test driver for deva language grammars

TEST_SOURCES=test.cpp symbol.cpp parser_ids.cpp error_report_parsers.cpp
TEST_OBJS=$(patsubst %.cpp, %.o, ${TEST_SOURCES})
DEVAC_SOURCES=devac.cpp symbol.cpp debug.cpp parser_ids.cpp error_report_parsers.cpp scope.cpp compile.cpp
DEVAC_OBJS=$(patsubst %.cpp, %.o, ${DEVAC_SOURCES})
TEST_DEP_FILES=$(patsubst %.cpp, %.dep, ${TEST_SOURCES})
DEVAC_DEP_FILES=$(patsubst %.cpp, %.dep, ${DEVAC_SOURCES})

# source directories:
#VFILES=

CPPFLAGS = -c -g
LDFLAGS = -g

all : tags test devac

tags : devac
	ctags -R 

test : ${TEST_OBJS}
	g++ ${LDFLAGS} -o test ${TEST_OBJS}

devac : ${DEVAC_OBJS}
	g++ ${LDFLAGS} -lboost_program_options-mt -o devac ${DEVAC_OBJS}

%.o : %.cpp
	g++ ${CPPFLAGS} -o $@ $<

clean:
	rm *.o test

# run tests
check:
	./runtests

include ${TEST_DEP_FILES}
include ${DEVAC_DEP_FILES}

%.dep : %.cpp
	@set -e; rm -f $@; \
	gcc -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


