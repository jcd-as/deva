// semantics.cpp
// semantic analysis functions for the deva language
// created by jcs, september 12, 2009 

// TODO:
// * 

#include "semantics.h"
#include "parser_ids.h"
#include "scope.h"
#include "compile.h"
#include <iostream>

// each check_xxx() function should:
// * set the value of the expression
// * [optional] double-check syntax:
// 	 - check the number of children
// 	 - check the types of the children
// type-checking of symbols:

// the global scopes table
extern Scopes scopes;

void check_number( iter_t const & i )
{
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = number_type;
	i->value.value( ni );
}

void check_string( iter_t const & i )
{
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = string_type;
	i->value.value( ni );
}

void check_boolean( iter_t const & i )
{
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = boolean_type;
	i->value.value( ni );
}

void check_null( iter_t const & i )
{
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = null_type;
	i->value.value( ni );
}

// 'def' keyword
void check_func( iter_t const & i )
{
	// check the number of children
	// 3 children: id, arg_list, compound_statement | statement
	if( i->children.size() != 3 )
		throw SemanticException( "Function declaration node doesn't have three children", i->value.value() );
	// check the types of the children
	if( i->children[0].value.id() != parser_id( identifier_id ) )
		throw SemanticException( "Invalid identifier for function declaration", i->value.value() );
	if( i->children[1].value.id() != arg_list_decl_id )
		throw SemanticException( "Invalid argument list for function declaration", i->value.value() );
	// TODO: 3rd child has to be some kind of expression. identifier, assign_op
	// etc etc
//	if( i->children[2].value.id() != compound_statement_id 
//		&& i->children[2].value.id() != identifier )
//		throw SemanticException( "Invalid argument list for function declaration", i->value.value() );
}

void check_while_s( iter_t const & i )
{
	// 2 children: condition (boolean/number/string/variable/null), compound or single statement
	NodeInfo condition = i->children[0].value.value();
//	NodeInfo statement = i->children[1].value.value();
	if( condition.type != boolean_type && condition.type != variable_type 
		&& condition.type != number_type && condition.type != string_type 
		&& condition.type != null_type )
		throw SemanticException( "Invalid condition in while statement", condition );
}

void check_for_s( iter_t const & i )
{
	// n children: n identifiers, in_op, statement|compound_statement
	int num_children = i->children.size();
	NodeInfo statements = i->children[num_children - 1].value.value();
	NodeInfo in_op = i->children[num_children - 2].value.value();
	if( i->children[num_children - 2].value.id() != in_op_id )
		throw SemanticException( "Malformed for loop; missing 'in' operator", in_op );
	for( int j = 0; j < num_children - 2; ++j )
	{
		NodeInfo item = i->children[j].value.value();
		if( i->children[j].value.id() != identifier_id )
			throw SemanticException( "Invalid loop variable in for loop", item );
	}
}

void check_if_s( iter_t const & i )
{
	// 2 children: condition, statement|compound_statement
	NodeInfo condition = i->children[0].value.value();
//	NodeInfo statement = i->children[1].value.value();
	if( condition.type != boolean_type && condition.type != variable_type 
		&& condition.type != number_type && condition.type != string_type 
		&& condition.type != null_type )
		throw SemanticException( "Invalid condition in if statement", condition );
}

void check_else_s( iter_t const & i )
{
	// 1 child: statement|compound_statement
}

void check_identifier( iter_t const & i )
{
	string s = i->value.value().sym;
	// disallow keywords
	if( is_keyword( s ) )
		throw SemanticException( "Keyword used where variable name expected", i->value.value() );

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void check_in_op( iter_t const & i )
{
	// 1 child: id
	NodeInfo operand = i->children[0].value.value();
	if( operand.type != variable_type )
		throw SemanticException( "Attempting to loop over an invalid object or type", operand );
}

void check_map_op( iter_t const & i )
{
	// has no children
	// always appears on the rhs of an assignment op
}

void check_vec_op( iter_t const & i )
{
	// has no children
	// always appears on the rhs of an assignment op
}

void check_semicolon_op( iter_t const & i )
{
	// has no children
}

void check_assignment_op( iter_t const & i )
{
	// first child is lhs - must be an lvalue (variable or map/vector key item) 
	// or a const_decl
	// second child is rhs - must be an rvalue (number, string, boolean, null or variable)
	// and the type of the assignment itself MUST be a variable
	// optional third child must be semi-colon if it exists
	NodeInfo lhs = i->children[0].value.value();
	NodeInfo rhs = i->children[1].value.value();
	if( lhs.type != variable_type && i->children[0].value.id() != const_decl_id )
		throw SemanticException( "Left-hand side of assignment operation not an l-value", lhs );
	else if( rhs.type != number_type && rhs.type != string_type 
		&& rhs.type != boolean_type && rhs.type != null_type
		&& rhs.type != variable_type && i->children[1].value.id() != vec_op_id
		&& i->children[1].value.id() != map_op_id )
		throw SemanticException( "Right-hand side of assignment operation not an r-value", rhs );

	// TODO: something in here causes a crash when dumping symbols! (in
	// devac.cpp)
	if( lhs.type == variable_type )
	{
		// TODO: only warn once per scope (this warns on every place it's a lhs
		// to an assignment)
		// warn if redef'ing a symbol
		SymbolTable* st = scopes[i->value.value().scope];
		int idx = st->parent_id;
		// global scope has no parent
		if( idx != -1 )
		{
			SymbolTable* parent_st = scopes[st->parent_id];
			if( parent_st )
			{
				SymbolInfo* si = find_symbol( lhs.sym, parent_st, scopes );
				if( si )
					emit_warning( i->value.value(), string( string( "Redefining symbol. '" ) + lhs.sym + "' already defined in higher scope" ).c_str() );
			}
		}
	}

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void check_logical_op( iter_t const & i )
{
	// children: lhs, rhs, [optional] semi-colon
	NodeInfo lhs = i->children[0].value.value();
	NodeInfo rhs = i->children[1].value.value();

	// both sides operate on expressions that can evaluate to true/false 
	//     (numbers, strings, booleans, nulls)
	// and the type of a logical op MUST be boolean
	if( lhs.type != number_type 
		&& lhs.type != string_type
		&& lhs.type != variable_type
		&& lhs.type != boolean_type
		&& lhs.type != null_type )
		throw SemanticException( "Invalid left-hand operand for logical operator", lhs );

	if( rhs.type != number_type 
		&& rhs.type != string_type
		&& rhs.type != variable_type
		&& rhs.type != boolean_type
		&& rhs.type != null_type )
		throw SemanticException( "Invalid right-hand operand for logical operator", rhs );

	// set the node
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = boolean_type;
	i->value.value( ni );

	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw SemanticException( "Malformed logical operation", i->children[2].value.value() );
	}
}

void check_relational_op( iter_t const & i )
{
	// children: lhs, rhs, [optional] semi-colon
	NodeInfo lhs = i->children[0].value.value();
	NodeInfo rhs = i->children[1].value.value();

	// for ordering relational ops ('>', '<', '>=', '<=')
	// both sides operate on expressions that can evaluate to scalar types with
	// order-able values (numbers and strings) 
	// equality ops ('==' and '!=') can operate on any scalar types
	//     (numbers, strings, booleans, nulls)
	// for both, the type of both sides must be the same
	// and the type of a relational op itself MUST be boolean
	// equality comparison
	if( i->value.value().sym == "==" || i->value.value().sym == "!=" )
	{
		if( lhs.type == number_type )
		{
			if( rhs.type != number_type && rhs.type != variable_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == string_type )
		{
			if( rhs.type != string_type && rhs.type != variable_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == boolean_type )
		{
			if( rhs.type != boolean_type && rhs.type != variable_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == null_type )
		{
			if( rhs.type != null_type && rhs.type != variable_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == variable_type )
		{
			if( rhs.type != variable_type && rhs.type != null_type
				&& rhs.type != boolean_type && rhs.type != string_type
				&& rhs.type != number_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else
			throw SemanticException( "Invalid left-hand operand in equality test", lhs );
	}
	// relational comparison
	else if( i->value.value().sym == ">" || i->value.value().sym == "<"
		|| i->value.value().sym == ">=" || i->value.value().sym == "<=" )
	{
		if( lhs.type == number_type )
		{
			if( rhs.type != number_type && rhs.type != variable_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == string_type )
		{
			if( rhs.type != string_type && rhs.type != variable_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == variable_type )
		{
			if( rhs.type != variable_type && rhs.type != string_type
				&& rhs.type != number_type )
				throw SemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else
			throw SemanticException( "Invalid left-hand operand in equality test", lhs );
	}
	else
		throw SemanticException( "Invalid comparison operator", i->value.value() );

	// set the node
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = boolean_type;
	i->value.value( ni );

	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw SemanticException( "Malformed comparison operation", i->children[2].value.value() );
	}
}

void check_mult_op( iter_t const & i )
{
	// both sides of mult op must be numbers or variables
	// and the type of the mult op itself *must* be numeric
	NodeInfo lhs = i->children[0].value.value();
	NodeInfo rhs = i->children[1].value.value();
	if( lhs.type == number_type || lhs.type == variable_type )
	{
		if( rhs.type != number_type && rhs.type != variable_type )
			throw SemanticException( "Invalid right-hand operand to multiply operation", rhs );

		NodeInfo ni = ((NodeInfo)(i->value.value()));
		ni.type = number_type;
		i->value.value( ni );
	}
	else 
	{
		throw SemanticException( "Invalid left-hand operand to multiply operation", lhs );
	}
	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw SemanticException( "Malformed multiply operation", i->children[2].value.value() );
	}
}

void check_add_op( iter_t const & i )
{
	// both sides of add op must be numbers or
	// both sides must be strings AND the op can't be subtract ('-')
	// first child is lhs
	// second child is rhs
	NodeInfo lhs = i->children[0].value.value();
	NodeInfo rhs = i->children[1].value.value();
	if( lhs.type == number_type )
	{
		if( rhs.type != number_type && rhs.type != variable_type )
			throw SemanticException( "Mismatched operands to add operation", rhs );

		NodeInfo ni = ((NodeInfo)(i->value.value()));
		if( lhs.type == number_type || rhs.type == number_type )
			ni.type = number_type;
		else
			ni.type = variable_type;
		i->value.value( ni );
	}
	else if( lhs.type == string_type )
	{
		if( i->value.value().sym != "+" )
			throw SemanticException( "Invalid string operation", rhs );
		else if( rhs.type != string_type && rhs.type != variable_type )
			throw SemanticException( "Mismatched operands to string concatenat operation", rhs );

		NodeInfo ni = ((NodeInfo)(i->value.value()));
		ni.type = string_type;
		i->value.value( ni );
	}
	else if( lhs.type == variable_type )
	{
		NodeInfo ni = ((NodeInfo)(i->value.value()));
		if( rhs.type == number_type )
		{
			ni.type = number_type;
		}
		else if( rhs.type == string_type )
		{
			ni.type = string_type;
		}
		else if( rhs.type == variable_type )
		{
			ni.type = variable_type;
		}
		else
			throw SemanticException( "Invalid right-hand operand to add operation", rhs );

		i->value.value( ni );
	}
	else
	{
		throw SemanticException( "Invalid left-hand operand to add operation", lhs );
	}
	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw SemanticException( "Malformed add operation", i->children[2].value.value() );
	}
}

void check_unary_op( iter_t const & i )
{
	// 1 or 2 children: operand and optional semi-colon
	NodeInfo operand = i->children[0].value.value();

	// two possible unary ops: '!' and '-'
	// '!' operates on expressions that can evaluate to true/false 
	//     (numbers, strings, booleans, nulls)
	// '-' only operates on numbers
	
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	if( i->value.value().sym == "!" )
	{
		if( operand.type == number_type )
			ni.type = number_type;
		else if( operand.type == string_type )
			ni.type = string_type;
		else if( operand.type == variable_type )
			ni.type = variable_type;
		else if( operand.type == boolean_type )
			ni.type = boolean_type;
		else if( operand.type == null_type )
			ni.type = null_type;
		else
			throw SemanticException( "Invalid operand for unary operator", operand );
	}
	else if( i->value.value().sym == "-" )
	{
		if( operand.type != number_type && operand.type != variable_type )
			throw SemanticException( "Invalid operand for unary operator", operand );

		ni.type = number_type;
	}
	else
		throw SemanticException( "Invalid unary operator", operand );

	i->value.value( ni );

	// optional second child must be semi-colon if it exists
	if( i->children.size() == 2 )
	{
		if( i->children[1].value.id() != semicolon_op_id )
			throw SemanticException( "Malformed unary operator expression", i->children[2].value.value() );
	}
}

void check_dot_op( iter_t const & i )
{
	// type of the result of a dot operator is 'variable_type'
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void check_paren_op( iter_t const & i )
{
}

void check_bracket_op( iter_t const & i )
{
}

void check_arg_list_exp( iter_t const & i )
{
}

void check_arg_list_decl( iter_t const & i )
{
	// n children: '(', n identifiers, ')'
}

void check_key_exp( iter_t const & i )
{
	// 3 children: '[', index, ']'
	// where index is a string, number or variable type
	// key_exp always generates a variable_type, as it is indexing into a
	// collection that could contain anything
	NodeInfo left = i->children[0].value.value();
	NodeInfo operand = i->children[1].value.value();
	NodeInfo right = i->children[2].value.value();

	if( left.sym != "[" )
		throw SemanticException( "Invalid indexing expression", left );
	else if( right.sym != "]" )
		throw SemanticException( "Invalid indexing expression", right );

	if( operand.type != number_type 
		&& operand.type != string_type
		&& operand.type != variable_type )
		throw SemanticException( "Index must be a numeric, string or variable value", operand );

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void pre_check_const_decl( iter_t const & i )
{
	// is always the lhs of assignment op
	// 2 children: 'const' and identifier
	// ensure that this identifier hasn't already been declared non-const
	
	// remember, in pre-check the children nodes' "sym" fields haven't been set yet
	// allow re-def as const, but emit a warning message
	string s = strip_symbol( string( i->children[1].value.begin(), i->children[1].value.end() ) );
	SymbolTable* st = scopes[i->value.value().scope];
	SymbolInfo* si = find_symbol( s, st, scopes );
	if( si )
		emit_warning( i->children[1].value.value(), string( string( "Symbol '" ) + s + "' already defined; redefining as 'const'" ).c_str() );
}

void check_const_decl( iter_t const & i )
{
	// is always the lhs of assignment op
	// 2 children: 'const' and identifier
	NodeInfo keyword = i->children[0].value.value();
	NodeInfo id = i->children[1].value.value();
	if( keyword.sym != "const" )
		throw SemanticException( "Invalid constant declaration", keyword );
	else if( id.type != variable_type )
		throw SemanticException( "Invalid constant declaration", id );

	// set symbol's 'const' flag in the symbol table
	// DON'T DO THIS: handle all variably typing and definition happen
	// dynamically (at run-time)
//	SymbolTable* st = scopes[i->children[1].value.value().scope];
//	SymbolInfo* si = find_symbol( i->children[1].value.value().sym, st, scopes );
//	if( !si )
//		throw SemanticException( string( string( "Symbol " ) + i->children[1].value.value().sym + " not found in any scope" ).c_str(), i->children[1].value.value() );
//	si->is_const = true;
}

void check_constant( iter_t const & i )
{
	// "const" keyword
	// 0 children
}

void check_translation_unit( iter_t const & i )
{
}

void check_compound_statement( iter_t const & i )
{
}

void check_break_statement( iter_t const & i )
{
	// must be inside of the compound_statement part of a loop/decision
	// construct (if/else/while/for)
	// (tbd at runtime? there's no way to walk the AST from child to parent...)
}

void check_continue_statement( iter_t const & i )
{
	// must be inside of the compound_statement part of a loop/decision
	// construct (if/else/while/for)
	// (tbd at runtime? there's no way to walk the AST from child to parent...)
}

void check_return_statement( iter_t const & i )
{
}

