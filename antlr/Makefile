#dvtest : test.o devaLexer.o devaParser.o deva_ast.o
#	g++ -o dvtest -g -L/usr/local/lib -lantlr3c test.o devaLexer.o devaParser.o deva_ast.o
# 
#devaParser.o : devaParser.c devaParser.h
#	g++ -c -g -I/usr/local/include devaParser.c
#
#devaLexer.o : devaLexer.c devaLexer.h
#	g++ -c -g -I/usr/local/include devaLexer.c
#
#deva-ast.o : deva-ast.c deva-ast.h
#	g++ -c -g -I/usr/local/include deva-ast.c
#
#test.o : test.cpp test.h devaLexer.h devaParser.h deva_ast.h
#	g++ -c -g -I/usr/local/include -o test.o test.cpp
#	
#devaParser.c devaLexer.c devaLexer.h devaParser.h deva.tokens : deva.g
#	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva.g
#
#deva_ast.c deva_ast.h deva_ast.tokens : deva_ast.g
#	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva_ast.g
#
#clean :
#	rm dvtest
#	rm devaLexer.h
#	rm devaLexer.c
#	rm devaLexer.o
#	rm devaParser.h
#	rm devaParser.c
#	rm devaParser.o
#	rm deva_ast.h
#	rm deva_ast.c
#	rm deva_ast.o
#	rm deva.tokens
#	rm deva_ast.tokens

#########################3
# makefile for test driver for deva language grammars

# source directories:
#VPATH = src

# sources/objs for test executable
TEST_SOURCES=dvtest.cpp scope.cpp
TEST_C_SOURCES=devaLexer.c devaParser.c deva_ast.c
TEST_OBJS=$(patsubst %.cpp, %.o, ${TEST_SOURCES})
TEST_C_OBJS=$(patsubst %.cpp, %.o, ${TEST_C_SOURCES})
TEST_DEP_FILES=$(patsubst %.cpp, %.dep, ${TEST_SOURCES})
TEST_DEP_C_FILES=$(patsubst %.c, %.dep, ${TEST_C_SOURCES})

CXXFLAGS = -c -g -I inc -I /usr/local/include -DVERSION=\"0.0.1\" -DDEBUG
#-O2

# -undefined dynamic_lookup required on Mac OS X to find Boost symbols...
LDFLAGS = -g -L /usr/local/lib -lantlr3c 
#LDFLAGS = -g -undefined dynamic_lookup

all : tags ID dvtest

tags : dvtest
	ctags -R 

ID : dvtest
	mkid -i "C++"

dvtest : ${TEST_OBJS} ${TEST_C_OBJS}
	g++ ${LDFLAGS} -lboost_filesystem -o dvtest ${TEST_OBJS} ${TEST_C_OBJS}
	#g++ ${LDFLAGS} -lboost_filesystem-mt -o dvtest ${TEST_OBJS} ${TEST_C_OBJS}

devaParser.c devaLexer.c devaLexer.h devaParser.h deva.tokens : deva.g
	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva.g

deva_ast.c deva_ast.h deva_ast.tokens : deva_ast.g
	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva_ast.g

%.o : %.cpp
	g++ ${CXXFLAGS} -o $@ $<

%.o : %.c
	g++ ${CXXFLAGS} -o $@ $<

clean:
	rm *.o dvtest *.dep* devaLexer.h devaLexer.c devaParser.h devaParser.c deva_ast.h deva_ast.c deva.tokens deva_ast.tokens

# run tests
#check:
#	./deva runtests.dv

include ${TEST_DEP_FILES}
include ${TEST_DEP_C_FILES}

%.dep : %.cpp
	@set -e; rm -f $@; \
	gcc -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.dep : %.c
	@set -e; rm -f $@; \
	gcc -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

