Test.class : Test.java devaLexer.java devaParser.java deva_ast.java
	javac -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" Test.java devaLexer.java devaParser.java deva_ast.java
	
devaParser.java devaLexer.java : deva.g
	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva.g

deva_ast.java : deva_ast.g
	java -cp "/home/jcs/bin/antlrworks-1.4.1.jar:./" org.antlr.Tool deva_ast.g

clean :
	rm *.class
	rm deva.tokens
	rm deva_ast.tokens
	rm devaLexer.java
	rm devaParser.java
	rm deva_ast.java
