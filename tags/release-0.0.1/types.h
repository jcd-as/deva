// types.h
// typedefs for the deva language
// created by jcs, september 12, 2009 

// TODO:
// * 

#ifndef __TYPES_H__
#define __TYPES_H__

#include <boost/spirit.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_functor_parser.hpp>

#include <string>

using namespace std;
using namespace boost::spirit;

// types
////////////////////////
enum expression_t
{
	null_type,			// null keyword
	boolean_type,		// true/false keywords
	string_type,		// string constant
	number_type,		// number constant
	vector_type,		// vector variable
	map_type,			// map variable
	variable_type,		// variable (incl function call, map/vector lookup)
	function_decl_type,	// function declaration
	no_type				// non-typed construct (e.g. 'if' statement)
};

struct NodeInfo
{
	// filename the node came from
	string file;
	// the text of the node
	string sym;
	// the scope this node is in. can be looked up in the global scope table
	int scope;
	// the line of code corresponding to this node
	int line;
	// the expression type that this node evaluates to, if any
	// (set by the semantic checker)
	expression_t type;

	// default constructor
	NodeInfo() : sym( "" ), scope( 0 ), line( -1 ), type( no_type )
	{ }

	NodeInfo( int s, int l ) : sym( "" ), scope( s ), line( l ), type( no_type )
	{ }
};

// typedefs
////////////////////////
// the parser iterator
typedef position_iterator<char const*> iterator_t;
// the node data factory type
typedef node_val_data_factory<NodeInfo> factory_t;
typedef tree_match<iterator_t, factory_t> parse_tree_match_t;
typedef parse_tree_match_t::tree_iterator iter_t;


#endif // __TYPES_H__
