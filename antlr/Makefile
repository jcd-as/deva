dvtest : test.o devaLexer.o devaParser.o deva_ast.o
	g++ -o dvtest -g -L/usr/local/lib -lantlr3c test.o devaLexer.o devaParser.o deva_ast.o
 
devaParser.o : devaParser.c devaParser.h
	g++ -c -g -I/usr/local/include devaParser.c

devaLexer.o : devaLexer.c devaLexer.h
	g++ -c -g -I/usr/local/include devaLexer.c

deva-ast.o : deva-ast.c deva-ast.h
	g++ -c -g -I/usr/local/include deva-ast.c

test.o : test.cpp devaLexer.h devaParser.h deva_ast.h
	g++ -c -g -I/usr/local/include -o test.o test.cpp
	
devaParser.c devaLexer.c devaLexer.h devaParser.h deva.tokens : deva.g
	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva.g

deva_ast.c deva_ast.h : deva_ast.g
	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva_ast.g

clean :
	rm dvtest
	rm devaLexer.h
	rm devaLexer.c
	rm devaLexer.o
	rm devaParser.h
	rm devaParser.c
	rm devaParser.o
	rm deva_ast.h
	rm deva_ast.c
	rm deva_ast.o
	rm deva.tokens
	rm deva_ast.tokens
