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

using namespace boost::spirit;

// types
////////////////////////
struct NodeInfo
{
	// the scope this node is in. can be looked up in the global scope table
	int scope;
	// the line of code corresponding to this node
	int line;

	// default constructor
	NodeInfo() : scope( 0 ), line( -1 )
	{ }

	NodeInfo( int s, int l ) : scope( s ), line( l	)
	{ }
};

struct SemanticException
{
	const char* const  err;
	int line;

	SemanticException( const char* const s, int l ) : err( s ), line( l )
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
