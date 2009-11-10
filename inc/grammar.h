// grammar.h
// deva language grammar for expressions
// created by jcs, august 29, 2009 

// TODO:
// * missing semi-colon ends up with generic "Invalid statement" error :(
// * prevent multiple errors on same line (e.g. 'invalid if' and 'invalid statement')
// * more & better error reporting (try to avoid multiple reports on the same
// 	 line/problem)

#ifndef __GRAMMAR_H__
#define __GRAMMAR_H__

#include <utility>

#include "types.h"
#include "scope.h"
#include "parser_ids.h"

using namespace std;
using namespace boost::spirit;

// function (external) to add symbols to the symbol table(s)
void add_symbol( iterator_t start, iterator_t end );

// function (external) to output an error message
void report_error( file_position pos, char const* msg );

///////////////////////////////////////////////////////////////////////////////
//  Error reporting parsers
///////////////////////////////////////////////////////////////////////////////
struct error_report_parser 
{
    char const* eol_msg;
    char const* msg;

    error_report_parser( char const* eol_msg_, char const* msg_ ):
        eol_msg( eol_msg_ ),
        msg( msg_ )
    { }

    typedef nil_t result_t;

    template <typename ScannerT> int operator()( ScannerT const& scan, result_t& /*result*/ ) const
    {
        if( scan.at_end() )
		{
            if( eol_msg )
			{
                file_position fpos = scan.first.get_position();
				report_error( fpos, eol_msg );
            }
        }
		else
		{
            if( msg )
			{
                file_position fpos = scan.first.get_position();
				report_error( fpos, msg );
            }
        }

        return -1; // fail
    }

};
typedef functor_parser<error_report_parser> error_report_p;

extern error_report_p error_missing_closing_brace;
extern error_report_p error_invalid_else;
extern error_report_p error_invalid_if;
extern error_report_p error_invalid_statement;
extern error_report_p error_invalid_func_decl;
extern error_report_p error_invalid_while;
extern error_report_p error_invalid_for;


///////////////////////////////////////////////////////////////////////////////
// AST node-related variables and functions
///////////////////////////////////////////////////////////////////////////////

// the global scope table
extern Scopes scopes;

// stack of scopes, for building the global scope table
extern ScopeBuilder scope_bldr;

// the scope id counter (0 is global scope)
static int scope_id = 0;

// new scope
static inline void enter_scope( iterator_t begin, iterator_t end )
{
	// next scope id
	++scope_id;

	// dump debug info
//	cout << "entering new scope: " << scope_id << endl;

	// push a new scope
	SymbolTable* sym_tab = new SymbolTable();

	// set the parent scope id
	sym_tab->parent_id = scope_bldr.back().first;
	scope_bldr.push_back( pair<int, SymbolTable*>( scope_id, sym_tab ) );

	scopes[scope_id] = sym_tab;
}

// pop scope and add to global table
static inline void exit_scope( iterator_t begin, iterator_t end )
{
	// pop the scope off our stack
	scope_bldr.pop_back();

	// dump debug info
//	cout << "leaving scope " << sym.first << " with parent " << sym.second.parent_id << endl;
}

// the node set-up function
static inline void set_node( tree_match<iterator_t, factory_t>::node_t & n, iterator_t begin, iterator_t end )
{
	NodeInfo ni;

	// set the file and line number for this construct
	file_position fpos = begin.get_position();
	ni.line = fpos.line;
	ni.file = fpos.file;
	// set the scope
	ni.scope = scope_bldr.back().first;

	n.value.value( ni );
}


///////////////////////////////////////////////////////////////////////////////
//  the deva language grammar
///////////////////////////////////////////////////////////////////////////////
class DevaGrammar : public grammar<DevaGrammar>
{
public:
    template <typename ScannerT> class definition
	{
	public:
		definition( DevaGrammar const& self )
		{
			// grammar specifications
			//////////////////////////////////////////////////////////////////////

			// statements:
			////////////////////////////////////////
			translation_unit =
				access_node_d[
				*top_level_statement
				][&set_node]
				;

			top_level_statement = 
				access_node_d[
				func_decl
				| class_decl
				| statement
				| error_invalid_statement
				][&set_node]
				;

			statement = 
				access_node_d[
				access_node_d[compound_statement][&set_node]
				| while_statement
				| for_statement
				| if_statement
				| import_statement
				| root_node_d[jump_statement] >> semicolon_op >> !end_p
				| exp_statement
				][&set_node]
				;

			class_decl =
				access_node_d[
				root_node_d[str_p( "class" )]
				>> identifier >> !( no_node_d[ch_p( ":" )] >> list_p( identifier, no_node_d[comma_op] ) )
				>> no_node_d[ch_p( "{" )]
				>> *func_decl
				>> no_node_d[ch_p( "}" )]
				>> !end_p
				][&set_node]
				;

			func_decl =
				access_node_d[
				root_node_d[func] 
					>> (identifier | error_invalid_func_decl)[&enter_scope]
					>> (access_node_d[arg_list_decl][&set_node] | error_invalid_func_decl) 
					>> (access_node_d[compound_statement][&set_node] | error_invalid_func_decl)[&exit_scope]
				][&set_node]
				;

			while_statement = 
				access_node_d[
				root_node_d[while_s] 
					>> 
						(
							(
								no_node_d[ch_p( "(" )] 
									>> exp >> no_node_d[ch_p( ")" )] 
									>> access_node_d[compound_statement][&set_node]
							) 
							| error_invalid_while
						)
				][&set_node]
				;

			for_statement =
				access_node_d[
				root_node_d[for_s] 
					>> 
					(
						(
						 	no_node_d[ch_p( "(" )][&enter_scope]
								>> 
								(
									exp 
									>> !(no_node_d[ch_p( "," )] >> exp)
								) 
								>> in_exp >> no_node_d[ch_p( ")" )] 
								>> access_node_d[compound_statement][&set_node]
						)[&exit_scope]
						| error_invalid_for
					)
				][&set_node]
				;

			if_statement = 
				access_node_d[
				root_node_d[if_s] 
				>> 
				(
					(
						no_node_d[ch_p( "(" )] 
						>> exp 
						>> no_node_d[ch_p( ")" )] 
						>> statement 
						| access_node_d[compound_statement][&set_node]
					)
					| error_invalid_if
				)
				>> !else_statement
				][&set_node]
				;

			else_statement =
				access_node_d[
				root_node_d[else_s] 
				>> (statement | access_node_d[compound_statement][&set_node] | if_statement | error_invalid_else)
				][&set_node]
				;

			import_statement = 
				access_node_d[
				root_node_d[str_p( "import" )]
				>> module_name >> semicolon_op >> !end_p
				][&set_node]
				;

			compound_statement =
				no_node_d[ch_p( "{" )][&enter_scope]
				>> *statement 
				>> (no_node_d[ch_p( "}" )][&exit_scope] | error_missing_closing_brace) 
				>> !end_p
				;

			jump_statement =
				access_node_d[
				break_statement
				| continue_statement
				| return_statement
				][&set_node]
				;

			break_statement =
				access_node_d[
				leaf_node_d[str_p( "break" )]
				][&set_node]
				;
			
			continue_statement =
				access_node_d[
				leaf_node_d[str_p( "continue" )]
				][&set_node]
				;

			return_statement =
				access_node_d[
				root_node_d[str_p( "return" )] >> !exp
				][&set_node]
				;

			// all statements end with a semi-colon
			exp_statement =
				access_node_d[
				root_node_d[exp] >> semicolon_op >> !end_p
				][&set_node]
				;

			// expressions:
			////////////////////////////////////////
			exp = 
				access_node_d[
				root_node_d[assignment_exp] >> !end_p
				][&set_node]
				;

			assignment_exp =
				access_node_d[
				const_decl >> root_node_d[assignment_op] >> (number | string)
				| identifier >> root_node_d[assignment_op] >> new_decl
				| logical_exp >> +(root_node_d[add_assignment_op] >> logical_exp)
				| logical_exp >> +(root_node_d[sub_assignment_op] >> logical_exp)
				| logical_exp >> +(root_node_d[mul_assignment_op] >> logical_exp)
				| logical_exp >> +(root_node_d[div_assignment_op] >> logical_exp)
				| logical_exp >> +(root_node_d[mod_assignment_op] >> logical_exp)
				| logical_exp >> *(root_node_d[assignment_op] >> logical_exp)
				][&set_node]
				;

			const_decl = 
				access_node_d[
				constant >> identifier
				][&set_node]
				;

			new_decl =
				access_node_d[
				root_node_d[str_p( "new" )]
				>> list_p( identifier, no_node_d[ch_p( '.' )] )
				>> arg_list_exp
				][&set_node]
				;

			logical_exp =
				access_node_d[
				relational_exp >> *(root_node_d[logical_op] >> relational_exp)
				| (map_op | vec_op)
				][&set_node]
				;

			relational_exp = 
				access_node_d[
				add_exp >> *(root_node_d[relational_op] >> add_exp)
				][&set_node]
				;

			add_exp =
				access_node_d[
				mult_exp >> *(root_node_d[add_op] >> mult_exp)
				][&set_node]
				;

			mult_exp = 
				access_node_d[
				unary_exp >> *(root_node_d[mult_op] >> unary_exp)
				][&set_node]
				;

			unary_exp = 
				access_node_d[
				postfix_exp 
				| (root_node_d[unary_op] >> postfix_exp)
				][&set_node]
				;

			postfix_exp = 
				access_node_d[
				no_node_d[ch_p( "(" )] >> postfix_only_exp >> no_node_d[ch_p( ")" )] >> +(root_node_d[dot_op] >> postfix_only_exp)
				| postfix_only_exp >> +(root_node_d[dot_op] >> postfix_only_exp)
				| root_node_d[identifier] >> access_node_d[arg_list_exp][&set_node]
				| root_node_d[identifier] >> access_node_d[key_exp][&set_node]
				| primary_exp
				][&set_node]
				;

			// postfix-only exp: only matches an arg list or map key expression,
			// not any arbitrary expression
			postfix_only_exp = 
				root_node_d[identifier] >> !(access_node_d[arg_list_exp][&set_node] | access_node_d[key_exp][&set_node])
				;

			// argument list ("()"s & contents)
			// (we discard the commas, but keep the parens, since we need
			// something to differentiate from an identifier/number/etc when
			// there is only one arg.)
			arg_list_exp = 
				confix_p( open_paren_op, !list_p( exp, no_node_d[comma_op] ), close_paren_op )
				;

			// argument list declaration ("()"s & contents)
			// (we discard the commas, but keep the parens, since we need
			// something to differentiate from an identifier/number/etc when
			// there is only one arg.)
			arg_list_decl = 
				confix_p( open_paren_op, !list_p( arg, no_node_d[comma_op] ), close_paren_op )
				;

			arg =
				access_node_d[
				identifier
				>> !(no_node_d[ch_p( '=' )]
					>> (boolean
					| null
					| identifier
					| number
					| string))
				][&set_node]
				;

			// map key (inside "[]"s) - only 'math' expresssions allowed inside, 
			// not general expressions
			key_exp = 
				confix_p( open_bracket_op, add_exp, close_bracket_op )
				;

			// 'in' keyword expression
			in_exp =
				access_node_d[
				root_node_d[in_op] >> exp
				][&set_node]
				;

			primary_exp =
				access_node_d[
				root_node_d[factor_exp]
				][&set_node]
				;

			factor_exp = 
				access_node_d[
				boolean
				| null
				| identifier
				| number
				| string
				| inner_node_d[confix_p( ch_p( "(" ), exp, ch_p( ")" ) )]
				][&set_node]
				;

			// operators
			////////////////////////////////////////
			unary_op = 
				access_node_d[
				ch_p( "!" ) | ch_p( "-" )
				][&set_node]
				;

			add_op = 
				access_node_d[
				ch_p( "+" ) | ch_p( "-" )
				][&set_node]
				;

			mult_op =
				access_node_d[
				ch_p( "*" ) | ch_p( "/" ) | ch_p( "%" )
				][&set_node]
				;

			relational_op = 
				access_node_d[
				str_p( ">=" ) | str_p( "<=" ) | ch_p( ">" ) | ch_p( "<" ) | str_p( "==" ) | str_p( "!=" )
				][&set_node]
				;

			dot_op = 
				access_node_d[
				ch_p( "." )
				][&set_node]
				;

			open_paren_op = 
				access_node_d[
				ch_p( "(" )
				][&set_node]
				;
			close_paren_op = 
				access_node_d[
				ch_p( ")" )
				][&set_node]
				;

			open_bracket_op = 
				access_node_d[
				ch_p( "[" )
				][&set_node]
				;
			close_bracket_op = 
				access_node_d[
				ch_p( "]" )
				][&set_node]
				;

			comma_op = 
				access_node_d[
				ch_p( "," )
				][&set_node]
				;

			logical_op = 
				access_node_d[
				str_p( "&&" ) | str_p( "||" )
				][&set_node]
				;

			assignment_op = 
				access_node_d[
				ch_p( "=" )
				][&set_node]
				;

			add_assignment_op = 
				access_node_d[
				str_p( "+=" )
				][&set_node]
				;

			sub_assignment_op = 
				access_node_d[
				str_p( "-=" )
				][&set_node]
				;

			mul_assignment_op = 
				access_node_d[
				str_p( "*=" )
				][&set_node]
				;

			div_assignment_op = 
				access_node_d[
				str_p( "/=" )
				][&set_node]
				;

			mod_assignment_op = 
				access_node_d[
				str_p( "%=" )
				][&set_node]
				;

			// map construction op
			map_op = 
				access_node_d[
				no_node_d[ch_p( "{" ) >> *space_p >> ch_p( "}" )]
				][&set_node]
				;

			// vector construction op
			vec_op = 
				access_node_d[
				no_node_d[ch_p( "[" ) >> *space_p >> ch_p( "]" )]
				][&set_node]
				;

			open_brace_op = 
				access_node_d[
				ch_p( "{" )
				][&set_node]
				;
			close_brace_op = 
				access_node_d[
				ch_p( "}" )
				][&set_node]
				;

			semicolon_op = 
				access_node_d[
				ch_p( ";" )
				][&set_node]
				;

			in_op = 
				access_node_d[
				str_p( "in" )
				][&set_node]
				;

			// constants & variables
			////////////////////////////////////////
			string = 
				access_node_d[
				leaf_node_d[confix_p( "\"", *c_escape_ch_p, "\"" )]
				| leaf_node_d[confix_p( "'", *c_escape_ch_p, "'" )]
				][&set_node]
				;

			number = 
				access_node_d[
				leaf_node_d[str_p( "0x" ) >> hex_p]
				| leaf_node_d[str_p( "0o" ) >> oct_p]
				| leaf_node_d[str_p( "0b" ) >> bin_p]
				| leaf_node_d[real_p]
				][&set_node]
				;

			boolean = 
				access_node_d[
				leaf_node_d[str_p( "true" ) | str_p( "false" )]
				][&set_node]
				;

			null =
				access_node_d[
				leaf_node_d[str_p( "null" )]
				][&set_node]
				;

			constant = 
				access_node_d[
				str_p( "const" )
				][&set_node]
				;

			func = 
				access_node_d[
				str_p( "def" )
				][&set_node]
				;

			while_s =
				access_node_d[
				str_p( "while" )
				][&set_node]
				;

			for_s = 
				access_node_d[
				str_p( "for" )
				][&set_node]
				;

			if_s = 
				access_node_d[
				str_p( "if" )
				][&set_node]
				;

			else_s = 
				access_node_d[
				str_p( "else" )
				][&set_node]
				;

			identifier = 
				access_node_d[
				leaf_node_d[((alpha_p | ch_p( "_" )) >> *(alnum_p | ch_p( "_" ) ))[&add_symbol]]
				][&set_node]
				;

			module_name = access_node_d[
				leaf_node_d[identifier >> *(ch_p( "/" ) >> identifier)]
				][&set_node]
				;

			// end grammar specifications
			//////////////////////////////////////////////////////////////////////

			// debugging macros
			// to enable these, uncomment the #define at the top of the file
			// statements
			BOOST_SPIRIT_DEBUG_RULE( translation_unit );
			BOOST_SPIRIT_DEBUG_RULE( top_level_statement );
			BOOST_SPIRIT_DEBUG_RULE( statement );
			BOOST_SPIRIT_DEBUG_RULE( func_decl );
			BOOST_SPIRIT_DEBUG_RULE( class_decl );
			BOOST_SPIRIT_DEBUG_RULE( while_statement );
			BOOST_SPIRIT_DEBUG_RULE( for_statement );
			BOOST_SPIRIT_DEBUG_RULE( if_statement );
			BOOST_SPIRIT_DEBUG_RULE( else_statement );
			BOOST_SPIRIT_DEBUG_RULE( import_statement );
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
			BOOST_SPIRIT_DEBUG_RULE( new_decl );
			BOOST_SPIRIT_DEBUG_RULE( assignment_exp );
			BOOST_SPIRIT_DEBUG_RULE( logical_exp );
			BOOST_SPIRIT_DEBUG_RULE( relational_exp );
			BOOST_SPIRIT_DEBUG_RULE( add_exp );
			BOOST_SPIRIT_DEBUG_RULE( mult_exp );
			BOOST_SPIRIT_DEBUG_RULE( unary_exp );
			BOOST_SPIRIT_DEBUG_RULE( postfix_exp );
			BOOST_SPIRIT_DEBUG_RULE( postfix_only_exp );
			BOOST_SPIRIT_DEBUG_RULE( arg_list_exp );
			BOOST_SPIRIT_DEBUG_RULE( arg_list_decl );
			BOOST_SPIRIT_DEBUG_RULE( arg );
			BOOST_SPIRIT_DEBUG_RULE( key_exp );
			BOOST_SPIRIT_DEBUG_RULE( in_exp );
			BOOST_SPIRIT_DEBUG_RULE( primary_exp );
			BOOST_SPIRIT_DEBUG_RULE( factor_exp );
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
			BOOST_SPIRIT_DEBUG_RULE( add_assignment_op );
			BOOST_SPIRIT_DEBUG_RULE( sub_assignment_op );
			BOOST_SPIRIT_DEBUG_RULE( mul_assignment_op );
			BOOST_SPIRIT_DEBUG_RULE( div_assignment_op );
			BOOST_SPIRIT_DEBUG_RULE( mod_assignment_op );
			BOOST_SPIRIT_DEBUG_RULE( logical_op );
			BOOST_SPIRIT_DEBUG_RULE( map_op );
			BOOST_SPIRIT_DEBUG_RULE( vec_op );
			BOOST_SPIRIT_DEBUG_RULE( in_op );
			// constants
			BOOST_SPIRIT_DEBUG_RULE( string );
			BOOST_SPIRIT_DEBUG_RULE( number );
			BOOST_SPIRIT_DEBUG_RULE( boolean );
			BOOST_SPIRIT_DEBUG_RULE( null );
			BOOST_SPIRIT_DEBUG_RULE( constant );
			BOOST_SPIRIT_DEBUG_RULE( func );
			BOOST_SPIRIT_DEBUG_RULE( while_s );
			BOOST_SPIRIT_DEBUG_RULE( for_s );
			BOOST_SPIRIT_DEBUG_RULE( if_s );
			BOOST_SPIRIT_DEBUG_RULE( else_s );
			BOOST_SPIRIT_DEBUG_RULE( identifier );
			BOOST_SPIRIT_DEBUG_RULE( module_name );

			// define ids for the rules
			// statements
			translation_unit_id = translation_unit.id();
			top_level_statement_id = top_level_statement.id();
			statement_id = statement.id();
			func_decl_id = func_decl.id(); 
			class_decl_id = class_decl.id(); 
			while_statement_id = while_statement.id();
			for_statement_id = for_statement.id();
			if_statement_id = if_statement.id();
			else_statement_id = else_statement.id();
			import_statement_id = import_statement.id();
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
			new_decl_id = new_decl.id();
			assignment_exp_id = assignment_exp.id();
			logical_exp_id = logical_exp.id();
			relational_exp_id = relational_exp.id();
			add_exp_id = add_exp.id(); 
			mult_exp_id = mult_exp.id(); 
			unary_exp_id = unary_exp.id(); 
			postfix_exp_id = postfix_exp.id(), 
			postfix_only_exp_id = postfix_only_exp.id(), 
			arg_list_exp_id = arg_list_exp.id(), 
			arg_list_decl_id = arg_list_decl.id(), 
			arg_id = arg.id(), 
			key_exp_id = key_exp.id(), 
			in_exp_id = in_exp.id(), 
			primary_exp_id = primary_exp.id(), 
			factor_exp_id = factor_exp.id(); 
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
			add_assignment_op_id = add_assignment_op.id();
			sub_assignment_op_id = sub_assignment_op.id();
			mul_assignment_op_id = mul_assignment_op.id();
			div_assignment_op_id = div_assignment_op.id();
			mod_assignment_op_id = mod_assignment_op.id();
			map_op_id = map_op.id();
			vec_op_id = vec_op.id();
			in_op_id = in_op.id();
			// constants
			string_id = string.id(); 
			number_id = number.id(); 
			boolean_id = boolean.id();
			null_id = null.id();
			constant_id = constant.id();
			func_id = func.id();
			while_s_id = while_s.id();
			for_s_id = for_s.id();
			if_s_id = if_s.id();
			else_s_id = else_s.id();
			identifier_id = identifier.id();
			module_name_id = module_name.id();
		}

		// declare the rules
		rule<ScannerT> 
			// statements
			translation_unit,
			top_level_statement,
			statement,
			compound_statement,
			while_statement,
			for_statement,
			if_statement,
			else_statement,
			import_statement,
			func_decl,
			class_decl,
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
			new_decl,
			assignment_exp,
			logical_exp,
			relational_exp,
			add_exp, 
			mult_exp, 
			unary_exp, 
			postfix_exp, 
			postfix_only_exp, 
			arg_list_exp, 
			arg_list_decl, 
			arg,
			key_exp, 
			in_exp, 
			primary_exp, 
			factor_exp, 
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
			add_assignment_op,
			sub_assignment_op,
			mul_assignment_op,
			div_assignment_op,
			mod_assignment_op,
			map_op,
			vec_op,
			in_op,
			// constants
			string, 
			number, 
			boolean,
			null,
			constant,
			func,
			while_s,
			for_s,
			if_s,
			else_s,
			identifier,
			module_name;

		rule<ScannerT> const& start() const { return translation_unit; }
	};
};

#endif // __GRAMMAR_H__
