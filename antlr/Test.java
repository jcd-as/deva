import org.antlr.runtime.*;
import org.antlr.runtime.tree.*;

public class Test
{
	public static void main( String[] args ) throws Exception
	{
			System.out.println( args );

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

		// if '-tree' arg was given
		if ( args.length>0 && args[0].equals( "-tree" ) )
		{
			// dump it to stdout
			System.out.println( t.toStringTree() ); 

			///////////////////////////
			// Walk resulting tree; create treenode stream first
			CommonTreeNodeStream nodes = new CommonTreeNodeStream( t );

			// AST nodes have payloads that point into token stream
			nodes.setTokenStream( tokens );

			// Create a tree Walker attached to the nodes stream
			deva_ast walker = new deva_ast( nodes );

			// Invoke the start symbol, rule program
			walker.translation_unit();
		}
	}
}

