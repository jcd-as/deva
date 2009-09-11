// grammar.h
// deva language grammar for expressions
// created by jcs, august 29, 2009 

// TODO:
// * line counting !!!
// * accept 'f().x' (currently '(f()).x' will work but not 'f().x')
//
// * control flow:
// 		- loops (while, for)
// 		- selection (if, else)
// * declarations:
// 		- functions
// * misc:
//		- imports

#ifndef __GRAMMAR_H__
#define __GRAMMAR_H__

#include <boost/spirit.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>

using namespace std;
using namespace boost::spirit;

// function (external) to add symbols to the symbol table(s)
void add_symbol( char const* start, char const* end );


// declare ids for the rules
// statements
parser_id translation_unit_id;
parser_id statement_id;
parser_id jump_statement_id;
parser_id break_statement_id;
parser_id continue_statement_id;
parser_id return_statement_id;
parser_id compound_statement_id;
parser_id exp_statement_id;
parser_id open_brace_op_id;
parser_id close_brace_op_id;
parser_id semicolon_op_id;
// expressions
parser_id exp_id;
parser_id const_decl_id;
parser_id assignment_exp_id;
parser_id logical_exp_id;
parser_id relational_exp_id;
parser_id add_exp_id;
parser_id mult_exp_id;
parser_id unary_exp_id;
parser_id postfix_exp_id;
parser_id postfix_only_exp_id;
parser_id arg_list_exp_id;
parser_id key_exp_id;
parser_id primary_exp_id;
parser_id factor_exp_id;
//parser_id comment_id;
// operators
parser_id relational_op_id;
parser_id unary_op_id;
parser_id mult_op_id;
parser_id add_op_id;
parser_id dot_op_id;
parser_id open_paren_op_id;
parser_id close_paren_op_id;
parser_id open_bracket_op_id;
parser_id close_bracket_op_id;
parser_id comma_op_id;
parser_id logical_op_id;
parser_id assignment_op_id;
parser_id map_op_id;
parser_id vec_op_id;
// constants
parser_id string_id;
parser_id number_id;
parser_id boolean_id;
parser_id null_id;
parser_id constant_id;
parser_id identifier_id;

class DevaExpression : public grammar<DevaExpression>
{
public:
    template <typename ScannerT> class definition
	{
	public:
		definition( DevaExpression const& self )
		{
			// grammar specifications
			//////////////////////////////////////////////////////////////////////

			// statements:
			////////////////////////////////////////
			translation_unit =
				*statement;

			statement = 
				compound_statement
				| root_node_d[jump_statement] >> semicolon_op >> !end_p
				| exp_statement;

			compound_statement =
				no_node_d[ch_p( "{" )] >> *exp_statement >> no_node_d[ch_p( "}" )];

			jump_statement =
				break_statement
				| continue_statement
				| return_statement;

			break_statement =
				leaf_node_d[str_p( "break" )];
			
			continue_statement =
				leaf_node_d[str_p( "continue" )];

			return_statement =
				root_node_d[str_p( "return" )] >> !exp;

			// all statements end with a semi-colon
			exp_statement =
				root_node_d[exp] >> semicolon_op >> !end_p;

			// expressions:
			////////////////////////////////////////
			exp = 
				root_node_d[assignment_exp] >> !end_p;

			assignment_exp =
				const_decl >> root_node_d[assignment_op] >> (number | string)
				| logical_exp >> *(root_node_d[assignment_op] >> logical_exp);

			const_decl = 
				constant >> identifier;

			logical_exp =
				relational_exp >> *(root_node_d[logical_op] >> relational_exp)
				| (map_op | vec_op);

			relational_exp = 
				add_exp >> *(root_node_d[relational_op] >> add_exp);

			add_exp =
				mult_exp >> *(root_node_d[add_op] >> mult_exp);

			mult_exp = 
				unary_exp >> *(root_node_d[mult_op] >> unary_exp);

			unary_exp = 
				postfix_exp 
				| (root_node_d[unary_op] >> postfix_exp);

			postfix_exp = 
				root_node_d[identifier] >> arg_list_exp
				| root_node_d[identifier] >> key_exp
				| primary_exp >> root_node_d[dot_op] >> postfix_only_exp
				| primary_exp;
			
			// postfix-only exp: only matches an arg list or map key expression,
			// not any arbitrary expression
			postfix_only_exp = 
				root_node_d[identifier] >> !(arg_list_exp | key_exp);

			// argument list (inside "()"s)
			// (we discard the commas, but keep the parens, since we need
			// something to differentiate from an identifier/number/etc when
			// there is only one arg.)
			arg_list_exp = 
				confix_p( open_paren_op, !list_p( exp, no_node_d[comma_op] ), close_paren_op );

			// map key (inside "[]"s) - only 'math' expresssions allowed inside, 
			// not general expressions
			key_exp = 
				confix_p( open_bracket_op, add_exp, close_bracket_op );

			primary_exp =
				root_node_d[factor_exp];

			factor_exp = 
				boolean
				| null
				| identifier
				| number
				| string
				| inner_node_d[ confix_p( ch_p( "(" ), exp, ch_p( ")" ) )];

//			comment = leaf_node_d[comment_p( "#" )];

			// operators
			////////////////////////////////////////
			unary_op = 
				ch_p( "!" ) | ch_p( "-" );

			add_op = 
				ch_p( "+" ) | ch_p( "-" );

			mult_op =
 
				ch_p( "*" ) | ch_p( "/" ) | ch_p( "%" );

			relational_op = 
				str_p( ">=" ) | str_p( "<=" ) | ch_p( ">" ) | ch_p( "<" ) | str_p( "==" ) | str_p( "!=" );

			dot_op = 
				ch_p( "." );

			open_paren_op = 
				ch_p( "(" );
			close_paren_op = 
				ch_p( ")" );

			open_bracket_op = 
				ch_p( "[" );
			close_bracket_op = 
				ch_p( "]" );

			comma_op = 
				ch_p( "," );

			logical_op = 
				str_p( "&&" ) | str_p( "||" );

			assignment_op = 
				ch_p( "=" );

			// map construction op
			map_op = 
				no_node_d[ch_p( "{" ) >> *space_p >> ch_p( "}" )];

			// vector construction op
			vec_op = 
				no_node_d[ch_p( "[" ) >> *space_p >> ch_p( "]" )];

			open_brace_op = ch_p( "{" );
			close_brace_op = ch_p( "}" );

			semicolon_op = ch_p( ";" );

			// constants & variables
			////////////////////////////////////////
			string = 
				leaf_node_d[confix_p( "\"", *c_escape_ch_p, "\"" )]
				| leaf_node_d[confix_p( "'", *c_escape_ch_p, "'" )];

			number = 
				leaf_node_d[real_p];

			boolean = 
				leaf_node_d[str_p( "true" ) | str_p( "false" )];

			null =
				leaf_node_d[str_p( "null" )];

			constant = 
				str_p( "const" );

			identifier = 
				leaf_node_d[((alpha_p | ch_p( "_" )) >> *(alnum_p | ch_p( "_" ) ))[&add_symbol]];

			// end grammar specifications
			//////////////////////////////////////////////////////////////////////

			// debugging macros
			// to enable these, uncomment the #define at the top of the file
			// statements
			BOOST_SPIRIT_DEBUG_RULE( translation_unit );
			BOOST_SPIRIT_DEBUG_RULE( statement );
			BOOST_SPIRIT_DEBUG_RULE( compound_statement );
			BOOST_SPIRIT_DEBUG_RULE( jump_statement );
			BOOST_SPIRIT_DEBUG_RULE( break_statement );
			BOOST_SPIRIT_DEBUG_RULE( continue_statement );
			BOOST_SPIRIT_DEBUG_RULE( return_statement );
			BOOST_SPIRIT_DEBUG_RULE( exp_statement );
			BOOST_SPIRIT_DEBUG_RULE( open_brace_op );
			BOOST_SPIRIT_DEBUG_RULE( close_brace_op );
			BOOST_SPIRIT_DEBUG_RULE( semicolon_op );
			// expressions
			BOOST_SPIRIT_DEBUG_RULE( exp );
			BOOST_SPIRIT_DEBUG_RULE( const_decl );
			BOOST_SPIRIT_DEBUG_RULE( assignment_exp );
			BOOST_SPIRIT_DEBUG_RULE( logical_exp );
			BOOST_SPIRIT_DEBUG_RULE( relational_exp );
			BOOST_SPIRIT_DEBUG_RULE( add_exp );
			BOOST_SPIRIT_DEBUG_RULE( mult_exp );
			BOOST_SPIRIT_DEBUG_RULE( unary_exp );
			BOOST_SPIRIT_DEBUG_RULE( postfix_exp );
			BOOST_SPIRIT_DEBUG_RULE( postfix_only_exp );
			BOOST_SPIRIT_DEBUG_RULE( arg_list_exp );
			BOOST_SPIRIT_DEBUG_RULE( key_exp );
			BOOST_SPIRIT_DEBUG_RULE( primary_exp );
			BOOST_SPIRIT_DEBUG_RULE( factor_exp );
//			BOOST_SPIRIT_DEBUG_RULE( comment );
			// operators
			BOOST_SPIRIT_DEBUG_RULE( unary_op );
			BOOST_SPIRIT_DEBUG_RULE( add_op );
			BOOST_SPIRIT_DEBUG_RULE( mult_op );
			BOOST_SPIRIT_DEBUG_RULE( relational_op );
			BOOST_SPIRIT_DEBUG_RULE( dot_op );
			BOOST_SPIRIT_DEBUG_RULE( open_paren_op );
			BOOST_SPIRIT_DEBUG_RULE( close_paren_op );
			BOOST_SPIRIT_DEBUG_RULE( open_bracket_op );
			BOOST_SPIRIT_DEBUG_RULE( close_bracket_op );
			BOOST_SPIRIT_DEBUG_RULE( comma_op );
			BOOST_SPIRIT_DEBUG_RULE( assignment_op );
			BOOST_SPIRIT_DEBUG_RULE( logical_op );
			BOOST_SPIRIT_DEBUG_RULE( map_op );
			BOOST_SPIRIT_DEBUG_RULE( vec_op );
			// constants
			BOOST_SPIRIT_DEBUG_RULE( string );
			BOOST_SPIRIT_DEBUG_RULE( number );
			BOOST_SPIRIT_DEBUG_RULE( boolean );
			BOOST_SPIRIT_DEBUG_RULE( null );
			BOOST_SPIRIT_DEBUG_RULE( constant );
			BOOST_SPIRIT_DEBUG_RULE( identifier );

			// define ids for the rules
			// statements
			translation_unit_id = translation_unit.id();
			statement_id = statement.id();
			compound_statement_id = compound_statement.id();
			jump_statement_id = jump_statement.id();
			break_statement_id = break_statement.id();
			continue_statement_id = continue_statement.id();
			return_statement_id = return_statement.id();
			exp_statement_id = exp_statement.id();
			open_brace_op_id = open_brace_op.id();
			close_brace_op_id = close_brace_op.id();
			semicolon_op_id = semicolon_op.id();
			// expressions
			exp_id = exp.id();
			const_decl_id = const_decl.id();
			assignment_exp_id = assignment_exp.id();
			logical_exp_id = logical_exp.id();
			relational_exp_id = relational_exp.id();
			add_exp_id = add_exp.id(); 
			mult_exp_id = mult_exp.id(); 
			unary_exp_id = unary_exp.id(); 
			postfix_exp_id = postfix_exp.id(), 
			postfix_only_exp_id = postfix_only_exp.id(), 
			arg_list_exp_id = arg_list_exp.id(), 
			key_exp_id = key_exp.id(), 
			primary_exp_id = primary_exp.id(), 
			factor_exp_id = factor_exp.id(); 
//			comment_id = comment.id(); 
			// operators
			unary_op_id = unary_op.id(); 
			mult_op_id = mult_op.id(); 
			add_op_id = add_op.id(); 
			relational_op_id = relational_op.id(); 
			dot_op_id = dot_op.id(); 
			open_paren_op_id = open_paren_op.id(); 
			close_paren_op_id = close_paren_op.id(); 
			open_bracket_op_id = open_bracket_op.id(); 
			close_bracket_op_id = close_bracket_op.id(); 
			comma_op_id = comma_op.id(); 
			logical_op_id = logical_op.id();
			assignment_op_id = assignment_op.id();
			map_op_id = map_op.id();
			vec_op_id = vec_op.id();
			// constants
			string_id = string.id(); 
			number_id = number.id(); 
			boolean_id = boolean.id();
			null_id = null.id();
			constant_id = constant.id();
			identifier_id = identifier.id();
		}

		// declare the rules
		rule<ScannerT> 
			// statements
			translation_unit,
			statement,
			compound_statement,
			jump_statement,
			break_statement,
			continue_statement,
			return_statement,
			exp_statement,
			open_brace_op,
			close_brace_op,
			semicolon_op,
			// expressions
			exp,
			const_decl,
			assignment_exp,
			logical_exp,
			relational_exp,
			add_exp, 
			mult_exp, 
			unary_exp, 
			postfix_exp, 
			postfix_only_exp, 
			arg_list_exp, 
			key_exp, 
			primary_exp, 
			factor_exp, 
//			comment, 
			unary_op, 
			mult_op, 
			add_op, 
			relational_op, 
			dot_op,
			open_paren_op,
			close_paren_op,
			open_bracket_op,
			close_bracket_op,
			comma_op,
			logical_op,
			assignment_op,
			map_op,
			vec_op,
			// constants
			string, 
			number, 
			boolean,
			null,
			constant,
			identifier;

		rule<ScannerT> const& start() const { return translation_unit; }
	};
};

#endif // __GRAMMAR_H__
