#include <antlr3.h>
#include "devaLexer.h"
#include "devaParser.h"
#include "semantic_walker.h"

#include "semantics.h"
#include "exceptions.h"

#include <iostream>
#include <vector>
#include <boost/format.hpp>

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////
//Scope* current_scope = NULL;
//int deva_in_function = 0;
//char* deva_function_name = NULL;

/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////

// TODO: move this to an error/output handling file
// error display function
void devaDisplayRecognitionError( pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames)
{
	// error format sample:
	// devaParser.c:47: error: ‘tokenNames’ was not declared in this scope

	pANTLR3_PARSER			parser;
	pANTLR3_TREE_PARSER	    tparser;
	pANTLR3_INT_STREAM	    is;
	pANTLR3_STRING			ttext;
	pANTLR3_STRING			ftext;
	pANTLR3_EXCEPTION	    ex;
	pANTLR3_COMMON_TOKEN    theToken;
	pANTLR3_BASE_TREE	    theBaseTree;
	pANTLR3_COMMON_TREE	    theCommonTree;

	// Retrieve some info for easy reading.
	ex = recognizer->state->exception;
	ttext =	NULL;

	// See if there is a 'filename' we can use
	if( ex->streamName == NULL )
	{
		if( ((pANTLR3_COMMON_TOKEN)(ex->token))->type == ANTLR3_TOKEN_EOF )
		{
			ANTLR3_FPRINTF( stderr, "-end of input-:" );
		}
		else
		{
			ANTLR3_FPRINTF( stderr, "-unknown source-:" );
		}
	}
	else
	{
		ftext = ex->streamName->to8( ex->streamName );
		ANTLR3_FPRINTF( stderr, "%s:", ftext->chars );
	}

	// Next comes the line number
	ANTLR3_FPRINTF( stderr, "%d: ", recognizer->state->exception->line );
//	ANTLR3_FPRINTF(stderr, " : error %d : %s", recognizer->state->exception->type,
	ANTLR3_FPRINTF( stderr, "error: %s", (pANTLR3_UINT8)	(recognizer->state->exception->message) );

	// How we determine the next piece is dependent on which thing raised the error.
	switch( recognizer->type )
	{
	case ANTLR3_TYPE_PARSER:
		// Prepare the knowledge we know we have
		parser	    = (pANTLR3_PARSER) (recognizer->super);
		tparser	    = NULL;
		is			= parser->tstream->istream;
		theToken    = (pANTLR3_COMMON_TOKEN)(recognizer->state->exception->token);
		ttext	    = theToken->toString( theToken );

		ANTLR3_FPRINTF( stderr, ", at offset %d: ", recognizer->state->exception->charPositionInLine );

		// need newline with the rest of this commented out:
//		ANTLR3_FPRINTF( stderr, "\n" );
		//////

//		if  (theToken != NULL)
//		{
//			if (theToken->type == ANTLR3_TOKEN_EOF)
//			{
//				ANTLR3_FPRINTF(stderr, ", at <EOF>");
//			}
//			else
//			{
//				// Guard against null text in a token
//				ANTLR3_FPRINTF(stderr, "\n    near %s\n    ", ttext == NULL ? (pANTLR3_UINT8)"<no text for the token>" : ttext->chars);
//			}
//		}
		break;

	case ANTLR3_TYPE_TREE_PARSER:
		// developer only error messages
		tparser		= (pANTLR3_TREE_PARSER) (recognizer->super);
		parser		= NULL;
		is			= tparser->ctnstream->tnstream->istream;
		theBaseTree	= (pANTLR3_BASE_TREE)(recognizer->state->exception->token);
		ttext		= theBaseTree->toStringTree( theBaseTree );

		if( theBaseTree != NULL )
		{
			theCommonTree = (pANTLR3_COMMON_TREE)theBaseTree->super;

			if( theCommonTree != NULL )
			{
				theToken = (pANTLR3_COMMON_TOKEN)theBaseTree->getToken( theBaseTree );
			}
			ANTLR3_FPRINTF( stderr, ", at offset %d: ", theBaseTree->getCharPositionInLine(theBaseTree) );
			ANTLR3_FPRINTF( stderr, ", near %s: ", ttext->chars );
		}
		break;

	default:
//		ANTLR3_FPRINTF(stderr, "Base recognizer function displayRecognitionError called by unknown parser type - provide override for this function\n");
		return;
		break;
	}

	// Although this function should generally be provided by the implementation, this one
	// should be as helpful as possible for grammar developers and serve as an example
	// of what you can do with each exception type. In general, when you make up your
	// 'real' handler, you should debug the routine with all possible errors you expect
	// which will then let you be as specific as possible about all circumstances.
	//
	// Note that in the general case, errors thrown by tree parsers indicate a problem
	// with the output of the parser or with the tree grammar itself. The job of the parser
	// is to produce a perfect (in traversal terms) syntactically correct tree, so errors
	// at that stage should really be semantic errors that your own code determines and handles
	// in whatever way is appropriate.
	//
	switch( ex->type )
	{
	case ANTLR3_UNWANTED_TOKEN_EXCEPTION:
		// Indicates that the recognizer was fed a token which seesm to be
		// spurious input. We can detect this when the token that follows
		// this unwanted token would normally be part of the syntactically
		// correct stream. Then we can see that the token we are looking at
		// is just something that should not be there and throw this exception.
		//
		if( tokenNames == NULL )
		{
			ANTLR3_FPRINTF( stderr, "Extraneous input...\n" );
		}
		else
		{
			if( ex->expecting == ANTLR3_TOKEN_EOF )
			{
				ANTLR3_FPRINTF( stderr, "Extraneous input - expected <EOF>\n" );
			}
			else
			{
				ANTLR3_FPRINTF( stderr, "Extraneous input - expected %s ...\n", tokenNames[ex->expecting] );
			}
		}
		break;

	case ANTLR3_MISSING_TOKEN_EXCEPTION:

		// Indicates that the recognizer detected that the token we just
		// hit would be valid syntactically if preceeded by a particular 
		// token. Perhaps a missing ';' at line end or a missing ',' in an
		// expression list, and such like.
		if( tokenNames == NULL )
		{
			ANTLR3_FPRINTF( stderr, "Missing token (%d)...\n", ex->expecting );
		}
		else
		{
			if( ex->expecting == ANTLR3_TOKEN_EOF )
			{
				ANTLR3_FPRINTF( stderr, "Missing <EOF>\n" );
			}
			else
			{
				ANTLR3_FPRINTF( stderr, "Missing %s\n", tokenNames[ex->expecting] );
			}
		}
		break;

	case ANTLR3_RECOGNITION_EXCEPTION:
		// Indicates that the recognizer received a token
		// in the input that was not predicted. This is the basic exception type 
		// from which all others are derived. So we assume it was a syntax error.
		// You may get this if there are not more tokens and more are needed
		// to complete a parse for instance.
		ANTLR3_FPRINTF( stderr, "Syntax error...\n" );    
		break;

	case ANTLR3_MISMATCHED_TOKEN_EXCEPTION:
		// We were expecting to see one thing and got another. This is the
		// most common error if we coudl not detect a missing or unwanted token.
		// Here you can spend your efforts to
		// derive more useful error messages based on the expected
		// token set and the last token and so on. The error following
		// bitmaps do a good job of reducing the set that we were looking
		// for down to something small. Knowing what you are parsing may be
		// able to allow you to be even more specific about an error.
		if( tokenNames == NULL )
		{
			ANTLR3_FPRINTF( stderr, "Syntax error...\n" );
		}
		else
		{
			if( ex->expecting == ANTLR3_TOKEN_EOF )
			{
				ANTLR3_FPRINTF( stderr, "Expected <EOF>\n" );
			}
			else
			{
				ANTLR3_FPRINTF(stderr, "Expected %s ...\n", tokenNames[ex->expecting] );
			}
		}
		break;

	case ANTLR3_NO_VIABLE_ALT_EXCEPTION:
		// We could not pick any alt decision from the input given
		// so god knows what happened - however when you examine your grammar,
		// you should. It means that at the point where the current token occurred
		// that the DFA indicates nowhere to go from here.
		ANTLR3_FPRINTF( stderr, "Cannot match to any predicted input...\n" );
		break;

	case ANTLR3_MISMATCHED_SET_EXCEPTION:
		{
			ANTLR3_UINT32	  count;
			ANTLR3_UINT32	  bit;
			ANTLR3_UINT32	  size;
			ANTLR3_UINT32	  numbits;
			pANTLR3_BITSET	  errBits;

			// This means we were able to deal with one of a set of
			// possible tokens at this point, but we did not see any
			// member of that set.
			ANTLR3_FPRINTF( stderr, "Unexpected input, expected one of : " );

			// What tokens could we have accepted at this point in the
			// parse?
			count = 0;
			errBits = antlr3BitsetLoad( ex->expectingSet );
			numbits = errBits->numBits( errBits );
			size = errBits->size( errBits );

			if( size > 0 )
			{
				// However many tokens we could have dealt with here, it is usually
				// not useful to print ALL of the set here. I arbitrarily chose 8
				// here, but you should do whatever makes sense for you of course.
				// No token number 0, so look for bit 1 and on.
				for( bit = 1; bit < numbits && count < 8 && count < size; bit++ )
				{
					// TODO: This doesn;t look right - should be asking if the bit is set!!
					if( tokenNames[bit] )
					{
						ANTLR3_FPRINTF( stderr, "%s%s", count > 0 ? ", " : "", tokenNames[bit] );
						count++;
					}
				}
				ANTLR3_FPRINTF( stderr, "\n" );
			}
			else
			{
//				ANTLR3_FPRINTF(stderr, "Actually dude, we didn't seem to be expecting anything here, or at least\n");
//				ANTLR3_FPRINTF(stderr, "I could not work out what I was expecting, like so many of us these days!\n");
			}
		}
		break;

	case ANTLR3_EARLY_EXIT_EXCEPTION:
		// We entered a loop requiring a number of token sequences
		// but found a token that ended that sequence earlier than
		// we should have done.
		ANTLR3_FPRINTF( stderr, "Missing elements...\n" );
		break;

	default:
		// We don't handle any other exceptions here, but you can
		// if you wish. If we get an exception that hits this point
		// then we are just going to report what we know about the
		// token.
		ANTLR3_FPRINTF( stderr, "Syntax not recognized...\n" );
		break;
	}

	// Here you have the token that was in error which if this is
	// the standard implementation will tell you the line and offset
	// and also record the address of the start of the line in the
	// input stream. You could therefore print the source line and so on.
	// Generally though, I would expect that your lexer/parser will keep
	// its own map of lines and source pointers or whatever as there
	// are a lot of specific things you need to know about the input
	// to do something like that.
	// Here is where you do it though :-).
}

// TODO: move this to an error/output handling file
void emit_error( DevaSemanticException & e )
{
	// TODO: exceptions need to get the file name somehow
	// format = filename:linenum: msg
//	cout << e.file << ":" << e.line << ":" << " error: " << e.what() << endl;
	cout << "[INPUT]" << ":" << e.line << ":" << " error: " << e.what() << endl;
}


// Main entry point for this example
//
int ANTLR3_CDECL main( int argc, char *argv[] )
{
	semantics = new Semantics();

	// setup the global scope
	semantics->current_scope = new LocalScope( "global", semantics->current_scope );
	semantics->scopes.push_back( semantics->current_scope );

	// Now we declare the ANTLR related local variables we need.
	// Note that unless you are convinced you will never need thread safe
	// versions for your project, then you should always create such things
	// as instance variables for each invocation.
	// -------------------

	// Name of the input file. Note that we always use the abstract type pANTLR3_UINT8
	// for ASCII/8 bit strings - the runtime library guarantees that this will be
	// good on all platforms. This is a general rule - always use the ANTLR3 supplied
	// typedefs for pointers/types/etc.
	//
	pANTLR3_UINT8 fName;

	// The ANTLR3 character input stream, which abstracts the input source such that
	// it is easy to privide inpput from different sources such as files, or 
	// memory strings.
	//
	// For an 8Bit/latin-1/etc memory string use:
	//     input = antlr3New8BitStringInPlaceStream (stringtouse, (ANTLR3_UINT32) length, NULL);
	//
	// For a UTF16 memory string use:
	//     input = antlr3NewUTF16StringInPlaceStream (stringtouse, (ANTLR3_UINT32) length, NULL);
	//
	// For input from a file, see code below
	//
	// Note that this is essentially a pointer to a structure containing pointers to functions.
	// You can create your own input stream type (copy one of the existing ones) and override any
	// individual function by installing your own pointer after you have created the standard 
	// version.
	pANTLR3_INPUT_STREAM input;

	// The lexer is of course generated by ANTLR, and so the lexer type is not upper case.
	// The lexer is supplied with a pANTLR3_INPUT_STREAM from whence it consumes its
	// input and generates a token stream as output. This is the ctx (CTX macro) pointer
	// for your lexer.
	pdevaLexer lxr;

	// The token stream is produced by the ANTLR3 generated lexer. Again it is a structure based
	// API/Object, which you can customise and override methods of as you wish. a Token stream is
	// supplied to the generated parser, and you can write your own token stream and pass this in
	// if you wish.
	pANTLR3_COMMON_TOKEN_STREAM tstream;

	// The Lang parser is also generated by ANTLR and accepts a token stream as explained
	// above. The token stream can be any source in fact, so long as it implements the 
	// ANTLR3_TOKEN_SOURCE interface. In this case the parser does not return anything
	// but it can of course specify any kind of return type from the rule you invoke
	// when calling it. This is the ctx (CTX macro) pointer for your parser.
	pdevaParser psr;

	// The parser produces an AST, which is returned as a member of the return type of
	// the starting rule (any rule can start first of course). This is a generated type
	// based upon the rule we start with.
	devaParser_translation_unit_return devaAST;


	// The tree nodes are managed by a tree adaptor, which doles
	// out the nodes upon request. You can make your own tree types and adaptors
	// and override the built in versions. See runtime source for details and
	// eventually the wiki entry for the C target.
	pANTLR3_COMMON_TREE_NODE_STREAM nodes;

	// Finally, when the parser runs, it will produce an AST that can be traversed by the 
	// the tree parser: c.f. LangDumpDecl.g3t This is the ctx (CTX macro) pointer for your
	// tree parser.
	psemantic_walker treePsr;

	// Create the input stream based upon the argument supplied to us on the command line
	// for this example, the input will always default to ./input if there is no explicit
	// argument.
	if( argc < 2 || argv[1] == NULL )
	{
		ANTLR3_FPRINTF( stderr, "usage:\ndvtest <input-filename>\n" );
		exit( -1 );
	}
	else
	{
	   fName = (pANTLR3_UINT8)argv[1];
	}

	// Create the input stream using the supplied file name
	// (Use antlr38BitFileStreamNew for UTF16 input).
	input = antlr3AsciiFileStreamNew( fName );

	// The input will be created successfully, providing that there is enough
	// memory and the file exists etc
	if( !input )
	{
	   ANTLR3_FPRINTF( stderr, "Unable to open file %s due to malloc() failure\n", (char *)fName );
	}

	// Our input stream is now open and all set to go, so we can create a new instance of our
	// lexer and set the lexer input to our input stream:
	//  (file | memory | ?) --> inputstream -> lexer --> tokenstream --> parser ( --> treeparser )?
	lxr = devaLexerNew( input );      // CLexerNew is generated by ANTLR

	// Need to check for errors
	if( !lxr )
	{
	   ANTLR3_FPRINTF( stderr, "Unable to create the lexer due to malloc() failure1\n" );
	   exit( ANTLR3_ERR_NOMEM );
	}

	// Our lexer is in place, so we can create the token stream from it
	// NB: Nothing happens yet other than the file has been read. We are just 
	// connecting all these things together and they will be invoked when we
	// call the parser rule. ANTLR3_SIZE_HINT can be left at the default usually
	// unless you have a very large token stream/input. Each generated lexer
	// provides a token source interface, which is the second argument to the
	// token stream creator.
	// Note tha even if you implement your own token structure, it will always
	// contain a standard common token within it and this is the pointer that
	// you pass around to everything else. A common token as a pointer within
	// it that should point to your own outer token structure.
	tstream = antlr3CommonTokenStreamSourceNew( ANTLR3_SIZE_HINT, TOKENSOURCE( lxr ) );

	if( !tstream )
	{
	   ANTLR3_FPRINTF( stderr, "Out of memory trying to allocate token stream\n" );
	   exit( ANTLR3_ERR_NOMEM );
	}

	// Finally, now that we have our lexer constructed, we can create the parser
	psr = devaParserNew( tstream );  // CParserNew is generated by ANTLR3

	if( !psr )
	{
	   ANTLR3_FPRINTF( stderr, "Out of memory trying to allocate parser\n" );
	   exit( ANTLR3_ERR_NOMEM );
	}

	// We are all ready to go. Though that looked complicated at first glance,
	// I am sure, you will see that in fact most of the code above is dealing
	// with errors and there isn;t really that much to do (isn;t this always the
	// case in C? ;-).
	//
	// So, we now invoke the parser. All elements of ANTLR3 generated C components
	// as well as the ANTLR C runtime library itself are pseudo objects. This means
	// that they are represented as pointers to structures, which contain any
	// instance data they need, and a set of pointers to other interfaces or
	// 'methods'. Note that in general, these few pointers we have created here are
	// the only things you will ever explicitly free() as everything else is created
	// via factories, that allocate memory efficiently and free() everything they use
	// automatically when you close the parser/lexer/etc.
	//
	// Note that this means only that the methods are always called via the object
	// pointer and the first argument to any method, is a pointer to the structure itself.
	// It also has the side advantage, if you are using an IDE such as VS2005 that can do it
	// that when you type ->, you will see a list of all the methods the object supports.
	devaAST = psr->translation_unit( psr );
	// If the parser ran correctly, we will have a tree to parse. In general I recommend
	// keeping your own flags as part of the error trapping, but here is how you can
	// work out if there were errors if you are using the generic error messages
	if( psr->pParser->rec->state->errorCount > 0 )
	{
	   ANTLR3_FPRINTF( stderr, "%d errors. aborted.\n", psr->pParser->rec->state->errorCount );
	}
	else
	{
		nodes = antlr3CommonTreeNodeStreamNewTree( devaAST.tree, ANTLR3_SIZE_HINT ); // sIZE HINT WILL SOON BE DEPRECATED!!

		// Tree parsers are given a common tree node stream (or your override)
		treePsr = semantic_walkerNew( nodes );

		// walk the tree
		try
		{
			treePsr->translation_unit( treePsr );
		}
		catch( DevaSemanticException & e )
		{
			// display an error
			emit_error( e );

			// clean up
			psr->free( psr );
			psr = NULL;

			tstream->free( tstream );
			tstream = NULL;

			lxr->free( lxr );
			lxr = NULL;

			input->close( input );
			input = NULL;

			nodes->free( nodes );        
			nodes = NULL;

			treePsr->free( treePsr );      
			treePsr = NULL;

			// free the symbol tables
//			for( vector<Scope*>::iterator i = scopes.begin(); i != scopes.end(); ++i )
//			{
//				delete *i;
//				*i = NULL;
//			}
			delete semantics;

			exit( -1 );
		}

		// TODO: only print the text repr of the tree if we're passed an option
		// on the command line
		pANTLR3_STRING s = nodes->root->toStringTree( nodes->root );
		ANTLR3_FPRINTF( stdout, "%s\n", (char*)s->chars );

		// TODO: dump the symbol tables...
		for( vector<Scope*>::iterator i = semantics->scopes.begin(); i != semantics->scopes.end(); ++i )
		{
			if( *i )
				(*i)->Print();
		}

		nodes->free( nodes );        
		nodes = NULL;
		treePsr->free( treePsr );      
		treePsr = NULL;
	}

	// We did not return anything from this parser rule, so we can finish. It only remains
	// to close down our open objects, in the reverse order we created them
	psr->free( psr );
	psr = NULL;

	tstream->free( tstream );
	tstream = NULL;

	lxr->free( lxr );
	lxr = NULL;

	input->close( input );
	input = NULL;

	// free the symbol tables
//	for( vector<Scope*>::iterator i = semantics->scopes.begin(); i != semantics->scopes.end(); ++i )
//	{
//		delete *i;
//		*i = NULL;
//	}
	delete semantics;

	return 0;
}
