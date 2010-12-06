Test.class : Test.java devaLexer.java devaParser.java
	javac -cp "/home/jcs/bin/antlrworks-1.4.jar:./" Test.java devaLexer.java devaParser.java
	
devaParser.java devaLexer.java : deva.g
	java -cp "/home/jcs/bin/antlrworks-1.4.jar:./" org.antlr.Tool deva.g

clean :
	rm *.class
	rm devaLexer.java
	rm devaParser.java
