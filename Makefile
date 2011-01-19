# makefile for test driver for deva language grammars

# source directories:
VPATH = src

# sources/objs for test executable
DEVA_SOURCES= \
deva.cpp \
scope.cpp \
semantics.cpp \
util.cpp \
error.cpp \
compile.cpp \
executor.cpp \
object.cpp \
api.cpp \
builtins.cpp \
vector_builtins.cpp \
builtins_helpers.cpp \
map_builtins.cpp \
frame.cpp \
scopetable.cpp
DEVA_C_SOURCES=devaLexer.c devaParser.c semantic_walker.c compile_walker.c
DEVA_OBJS=$(patsubst %.cpp, %.o, ${DEVA_SOURCES})
DEVA_C_OBJS=$(patsubst %.c, %.o, ${DEVA_C_SOURCES})
DEVA_DEP_FILES=$(patsubst %.cpp, %.dep, ${DEVA_SOURCES})
DEVA_DEP_C_FILES=$(patsubst %.c, %.dep, ${DEVA_C_SOURCES})

CXXFLAGS = -c -g -I inc -I "." -I /usr/local/include -DDEVA_VERSION=\"0.0.1\" -DDEBUG -DREFCOUNT_TRACE
#-O2

# -undefined dynamic_lookup required on Mac OS X to find Boost symbols...
LDADD = -lantlr3c -lboost_program_options -lboost_filesystem -lboost_system
#LDADD = -lantlr3c -lboost_program_options -lboost_filesystem 
LDFLAGS = -g -L /usr/local/lib
#LDFLAGS = -g -undefined dynamic_lookup

all : tags ID deva

tags : deva
	ctags -R --exclude=deva1 --exclude=tests --exclude=antlr

ID : deva
	mkid -i "C++" --prune=deva1 --prune=tests --prune=antlr

deva : ${DEVA_OBJS} ${DEVA_C_OBJS}
	g++ ${LDFLAGS} -o deva ${DEVA_OBJS} ${DEVA_C_OBJS} ${LDADD} 
	#g++ ${LDFLAGS} -lboost_filesystem-mt -o deva ${DEVA_OBJS} ${DEVA_C_OBJS} ${LDADD}

devaParser.c devaLexer.c devaLexer.h devaParser.h deva.tokens : deva.g
	java org.antlr.Tool -message-format gnu deva.g

semantic_walker.c semantic_walker.h semantic_walker.tokens : semantic_walker.g deva.tokens
	java org.antlr.Tool -message-format gnu semantic_walker.g

compile_walker.c compile_walker.h compile_walker.tokens : compile_walker.g deva.tokens
	java org.antlr.Tool -message-format gnu compile_walker.g

%.o : %.cpp
	g++ ${CXXFLAGS} -o $@ $<

%.o : %.c
	g++ ${CXXFLAGS} -o $@ $<

clean:
	-rm *.o deva *.dep* devaLexer.h devaLexer.c devaParser.h devaParser.c semantic_walker.h semantic_walker.c deva.tokens semantic_walker.tokens compile_walker.h compile_walker.c compile_walker.tokens

# run tests
check:
	deva1/deva antlrtest.dv
#	./deva runtests.dv

include ${DEVA_DEP_FILES}
include ${DEVA_DEP_C_FILES}

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

