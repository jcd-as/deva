import org.antlr.runtime.*;
import org.antlr.runtime.tree.*;

public class Test
{
	public static void main(String[] args) throws Exception
	{
		// Create an input character stream from standard in
		ANTLRInputStream input = new ANTLRInputStream(System.in);

		// Create an ExprLexer that feeds from that stream
		devaLexer lexer = new devaLexer(input);

		// Create a stream of tokens fed by the lexer
		CommonTokenStream tokens = new CommonTokenStream(lexer);

		// Create a parser that feeds off the token stream
		devaParser parser = new devaParser(tokens);

		// Begin parsing at rule prog
		devaParser.translation_unit_return result = parser.translation_unit();

		// get the tree
		Tree t = (Tree)result.getTree();

		// dump it to stdout
		System.out.println( t.toStringTree() ); 
	}
}

