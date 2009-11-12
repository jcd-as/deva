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
#include <algorithm>

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
		throw DevaSemanticException( "Function declaration node doesn't have three children", i->value.value() );
	// check the types of the children
	if( i->children[0].value.id() != parser_id( identifier_id ) )
		throw DevaSemanticException( "Invalid identifier for function declaration", i->value.value() );
	if( i->children[1].value.id() != arg_list_decl_id )
		throw DevaSemanticException( "Invalid argument list for function declaration", i->value.value() );
	// ensure the arguments are all uniquely named
	int num_children = i->children[1].children.size();
	vector<string> names;
	for( int j = 0; j < num_children; ++j )
	{
		NodeInfo arg = i->children[1].children[j].value.value();
		if( arg.type == variable_type && find( names.begin(), names.end(), arg.sym ) != names.end() )
			throw DevaSemanticException( "Duplicate variable in function argument list", arg );
		names.push_back( arg.sym );
	}
}

static vector<unsigned char> in_loop_stack;

void pre_check_while_s( iter_t const & i )
{
	in_loop_stack.push_back( 0 );
}

void check_while_s( iter_t const & i )
{
	// leaving the loop
	in_loop_stack.pop_back();

	// 2 children: condition (boolean/number/string/variable/null), compound or single statement
	NodeInfo condition = i->children[0].value.value();
	if( i->children[0].value.id() == assignment_op_id ||
		i->children[0].value.id() == add_assignment_op_id ||
		i->children[0].value.id() == sub_assignment_op_id ||
		i->children[0].value.id() == mul_assignment_op_id ||
		i->children[0].value.id() == div_assignment_op_id ||
		i->children[0].value.id() == mod_assignment_op_id )
		throw DevaSemanticException( "Illegal assignment inside 'while' conditional", condition );
	if( condition.type != boolean_type && condition.type != variable_type 
		&& condition.type != number_type && condition.type != string_type 
		&& condition.type != null_type )
		throw DevaSemanticException( "Invalid condition in while statement", condition );
}

void pre_check_for_s( iter_t const & i )
{
	in_loop_stack.push_back( 0 );
}

void check_for_s( iter_t const & i )
{
	// leaving the loop
	in_loop_stack.pop_back();

	// n children: n identifiers, in_op, statement|compound_statement
	int num_children = i->children.size();
	NodeInfo statements = i->children[num_children - 1].value.value();
	NodeInfo in_op = i->children[num_children - 2].value.value();
	if( i->children[num_children - 2].value.id() != in_op_id )
		throw DevaSemanticException( "Malformed for loop; missing 'in' operator", in_op );
	for( int j = 0; j < num_children - 2; ++j )
	{
		NodeInfo item = i->children[j].value.value();
		if( i->children[j].value.id() != identifier_id )
			throw DevaSemanticException( "Invalid loop variable in for loop", item );
	}
}

void check_if_s( iter_t const & i )
{
	// 2 or 3 children: condition, statement|compound_statement, [optional] else
	NodeInfo condition = i->children[0].value.value();
	if( i->children[0].value.id() == assignment_op_id ||
		i->children[0].value.id() == add_assignment_op_id ||
		i->children[0].value.id() == sub_assignment_op_id ||
		i->children[0].value.id() == mul_assignment_op_id ||
		i->children[0].value.id() == div_assignment_op_id ||
		i->children[0].value.id() == mod_assignment_op_id )
		throw DevaSemanticException( "Illegal assignment inside 'if' conditional", condition );
	if( condition.type != boolean_type && condition.type != variable_type 
		&& condition.type != number_type && condition.type != string_type 
		&& condition.type != null_type )
		throw DevaSemanticException( "Invalid condition in if statement", condition );
}

void check_else_s( iter_t const & i )
{
	// 1 child: statement|compound_statement
}

void check_import_s( iter_t const & i )
{
	// 2 children: id and semi-colon
	NodeInfo module = i->children[0].value.value();
	if( module.type != variable_type )
		throw DevaSemanticException( "'import' statement missing module identifier", module );
}

void check_identifier( iter_t const & i )
{
	string s = i->value.value().sym;
	// disallow keywords
	if( is_keyword( s ) )
		throw DevaSemanticException( "Keyword used where variable name expected", i->value.value() );

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void check_module_name( iter_t const & i )
{
	string s = i->value.value().sym;
	// disallow keywords
	if( is_keyword( s ) )
		throw DevaSemanticException( "Keyword used where variable name expected", i->value.value() );

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void check_in_op( iter_t const & i )
{
	// 1 child: id
	NodeInfo operand = i->children[0].value.value();
	if( operand.type != variable_type )
		throw DevaSemanticException( "Attempting to loop over an invalid object or type", operand );
}

void check_map_op( iter_t const & i )
{
	// TODO: anything we can do here??
}

void check_vec_op( iter_t const & i )
{
	// TODO: anything we can do here??
	for( int j = 0; j < i->children.size(); ++j )
	{
		if( i->children[j].value.id() == assignment_op_id ||
			i->children[j].value.id() == add_assignment_op_id ||
			i->children[j].value.id() == sub_assignment_op_id ||
			i->children[j].value.id() == mul_assignment_op_id ||
			i->children[j].value.id() == div_assignment_op_id ||
			i->children[j].value.id() == mod_assignment_op_id )
		{
			NodeInfo ni = i->children[j].value.value();
			throw DevaSemanticException( "Illegal assignment inside vector initializer", ni );
		}
	}
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
		throw DevaSemanticException( "Left-hand side of assignment operation not an l-value", lhs );
	else if( rhs.type != number_type 
		&& rhs.type != string_type 
		&& rhs.type != boolean_type 
		&& rhs.type != null_type
		&& rhs.type != variable_type 
		&& i->children[1].value.id() != vec_op_id
		&& i->children[1].value.id() != map_op_id 
		&& i->children[1].value.id() != new_decl_id )
		throw DevaSemanticException( "Right-hand side of assignment operation not an r-value", rhs );

	// check 'const' of lhs
	SymbolTable* st = scopes[lhs.scope];
	SymbolInfo si = find_symbol( lhs.sym, st, scopes );
	if( si.Type() != sym_end && si.is_const )
		throw DevaSemanticException( "Cannot assign to a const variable.", lhs );
	// TODO: review. any usefulness to this at all? any way to make it useful??
//	if( lhs.type == variable_type )
//	{
//		// TODO: only warn once per scope (this warns on every place it's a lhs
//		// to an assignment)
//		// warn if redef'ing a symbol
//		SymbolTable* st = scopes[i->value.value().scope];
//		int idx = st->parent_id;
//		// global scope has no parent
//		if( idx != -1 )
//		{
//			SymbolTable* parent_st = scopes[st->parent_id];
//			if( parent_st )
//			{
//				SymbolInfo* si = find_symbol( lhs.sym, parent_st, scopes );
//				if( si )
//					emit_warning( i->value.value(), string( string( "Redefining symbol. '" ) + lhs.sym + "' already defined in higher scope" ).c_str() );
//			}
//		}
//	}

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void check_op_assignment_op( iter_t const & i )
{
	// first child is lhs - must be an lvalue (variable or map/vector key item) 
	// second child is rhs - must be a numerical rvalue (number or variable)
	// and the type of the assignment itself MUST be a variable
	// optional third child must be semi-colon if it exists
	NodeInfo lhs = i->children[0].value.value();
	NodeInfo rhs = i->children[1].value.value();
	if( lhs.type != variable_type )
		throw DevaSemanticException( "Left-hand side of math op and assignment not an l-value", lhs );
	else if( rhs.type != number_type 
		&& rhs.type != variable_type 
		&& i->children[1].value.id() != vec_op_id
		&& i->children[1].value.id() != map_op_id )
		throw DevaSemanticException( "Right-hand side of math op and assignment not an r-value", rhs );

	// check 'const' of lhs
	SymbolTable* st = scopes[lhs.scope];
	SymbolInfo si = find_symbol( lhs.sym, st, scopes );
	if( si.Type() != sym_end && si.is_const )
		throw DevaSemanticException( "Cannot assign to a const variable.", lhs );

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
		throw DevaSemanticException( "Invalid left-hand operand for logical operator", lhs );

	if( rhs.type != number_type 
		&& rhs.type != string_type
		&& rhs.type != variable_type
		&& rhs.type != boolean_type
		&& rhs.type != null_type )
		throw DevaSemanticException( "Invalid right-hand operand for logical operator", rhs );

	// set the node
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = boolean_type;
	i->value.value( ni );

	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw DevaSemanticException( "Malformed logical operation", i->children[2].value.value() );
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
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == string_type )
		{
			if( rhs.type != string_type && rhs.type != variable_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == boolean_type )
		{
			if( rhs.type != boolean_type && rhs.type != variable_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == null_type )
		{
			if( rhs.type != null_type && rhs.type != variable_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == variable_type )
		{
			if( rhs.type != variable_type && rhs.type != null_type
				&& rhs.type != boolean_type && rhs.type != string_type
				&& rhs.type != number_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else
			throw DevaSemanticException( "Invalid left-hand operand in equality test", lhs );
	}
	// relational comparison
	else if( i->value.value().sym == ">" || i->value.value().sym == "<"
		|| i->value.value().sym == ">=" || i->value.value().sym == "<=" )
	{
		if( lhs.type == number_type )
		{
			if( rhs.type != number_type && rhs.type != variable_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == string_type )
		{
			if( rhs.type != string_type && rhs.type != variable_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else if( lhs.type == variable_type )
		{
			if( rhs.type != variable_type && rhs.type != string_type
				&& rhs.type != number_type )
				throw DevaSemanticException( "Invalid right-hand operand in equality test", rhs );
		}
		else
			throw DevaSemanticException( "Invalid left-hand operand in equality test", lhs );
	}
	else
		throw DevaSemanticException( "Invalid comparison operator", i->value.value() );

	// set the node
	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = boolean_type;
	i->value.value( ni );

	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw DevaSemanticException( "Malformed comparison operation", i->children[2].value.value() );
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
			throw DevaSemanticException( "Invalid right-hand operand to multiply operation", rhs );

		NodeInfo ni = ((NodeInfo)(i->value.value()));
		ni.type = number_type;
		i->value.value( ni );
	}
	else 
	{
		throw DevaSemanticException( "Invalid left-hand operand to multiply operation", lhs );
	}
	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw DevaSemanticException( "Malformed multiply operation", i->children[2].value.value() );
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
			throw DevaSemanticException( "Mismatched operands to add operation", rhs );

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
			throw DevaSemanticException( "Invalid string operation", rhs );
		else if( rhs.type != string_type && rhs.type != variable_type )
			throw DevaSemanticException( "Mismatched operands to string concatenat operation", rhs );

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
			throw DevaSemanticException( "Invalid right-hand operand to add operation", rhs );

		i->value.value( ni );
	}
	else
	{
		throw DevaSemanticException( "Invalid left-hand operand to add operation", lhs );
	}
	// optional third child must be semi-colon if it exists
	if( i->children.size() == 3 )
	{
		if( i->children[2].value.id() != semicolon_op_id )
			throw DevaSemanticException( "Malformed add operation", i->children[2].value.value() );
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
			throw DevaSemanticException( "Invalid operand for unary operator", operand );
	}
	else if( i->value.value().sym == "-" )
	{
		if( operand.type != number_type && operand.type != variable_type )
			throw DevaSemanticException( "Invalid operand for unary operator", operand );

		ni.type = number_type;
	}
	else
		throw DevaSemanticException( "Invalid unary operator", operand );

	i->value.value( ni );

	// optional second child must be semi-colon if it exists
	if( i->children.size() == 2 )
	{
		if( i->children[1].value.id() != semicolon_op_id )
			throw DevaSemanticException( "Malformed unary operator expression", i->children[2].value.value() );
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
	// n children: '(', n identifiers, ')'
	for( int j = 1; j < i->children.size()-1; ++j )
	{
		if( i->children[j].value.id() == assignment_op_id ||
			i->children[j].value.id() == add_assignment_op_id ||
			i->children[j].value.id() == sub_assignment_op_id ||
			i->children[j].value.id() == mul_assignment_op_id ||
			i->children[j].value.id() == div_assignment_op_id ||
			i->children[j].value.id() == mod_assignment_op_id )
		{
			NodeInfo ni = i->children[j].value.value();
			throw DevaSemanticException( "Illegal assignment inside function arguments", ni );
		}
	}
}

void check_arg_list_decl( iter_t const & i )
{
	// TODO: default args can only be allowed at the *end* of the arg list
}

void check_arg( iter_t const & i )
{
	// arg consists of an identifier and its default value, a boolean, null,
	// number, string or identifier
	if( i->children.size() == 2 )
	{
		// TODO: for some reason the id of the second child is not right, though
		// the value is
		if( i->children[0].value.id() != identifier_id 
//			|| i->children[1].value.id() != boolean_id 
//			|| i->children[1].value.id() != number_id 
//			|| i->children[1].value.id() != string_id 
//			|| i->children[1].value.id() != identifier_id 
			)
		{
			NodeInfo ni = i->children[0].value.value();
			throw DevaSemanticException( "Invalid argument.", ni );
		}
	}
	else
	{
		NodeInfo ni = i->value.value();
		throw DevaSemanticException( "Invalid default argument.", ni );
	}
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
		throw DevaSemanticException( "Invalid indexing expression", left );
	else if( right.sym != "]" )
		throw DevaSemanticException( "Invalid indexing expression", right );

	if( operand.type != number_type 
		&& operand.type != string_type
		&& operand.type != variable_type )
		throw DevaSemanticException( "Index must be a numeric, string or variable value", operand );

	NodeInfo ni = ((NodeInfo)(i->value.value()));
	ni.type = variable_type;
	i->value.value( ni );
}

void pre_check_const_decl( iter_t const & i )
{
	// is always the lhs of assignment op
	// 2 children: 'const' and identifier
	// ensure that this identifier hasn't already been declared non-const
	
	// TODO: this warning never fires, because the compile errors out in the
	// assignment op, which is processed before this, on trying to assign to a
	// constant
	//
	// remember, in pre-check the children nodes' "sym" fields haven't been set yet
	// allow re-def as const, but emit a warning message
	string s = strip_symbol( string( i->children[1].value.begin(), i->children[1].value.end() ) );
	SymbolTable* st = scopes[i->value.value().scope];
	if( find_identifier_in_parent_scopes( s, st, scopes ) )
		emit_warning( i->children[1].value.value(), string( string( "Symbol '" ) + s + "' already defined; redefining as 'const'" ).c_str() );
}

void check_const_decl( iter_t const & i )
{
	// is always the lhs of assignment op
	// 2 children: 'const' and identifier
	NodeInfo keyword = i->children[0].value.value();
	NodeInfo id = i->children[1].value.value();
	if( keyword.sym != "const" )
		throw DevaSemanticException( "Invalid constant declaration", keyword );
	else if( id.type != variable_type )
		throw DevaSemanticException( "Invalid constant declaration", id );

	// set symbol's 'const' flag in the symbol table
	SymbolTable* st = scopes[i->children[1].value.value().scope];
	if( !set_symbol_constness( i->children[1].value.value().sym, st, scopes, true ) )
		throw DevaSemanticException( string( string( "Symbol " ) + i->children[1].value.value().sym + " not found in any scope" ).c_str(), i->children[1].value.value() );
}

void check_new_decl( iter_t const & i )
{
	// is always the rhs of assignment op
	// children: some number of identifiers and an arg_list_exp
	int num_children = i->children.size();
	NodeInfo args = i->children[num_children-1].value.value();
	// FUTURE: if more than a 'module.class' is allowed, 
	// this will need to checkto change
	if( num_children < 2 || num_children > 3 )
		throw DevaSemanticException( "Invalid 'new' declaration", args );
	if( i->children[num_children-1].value.id() != arg_list_exp_id )
		throw DevaSemanticException( "Invalid 'new' statement", args );
	for( int c = 0; c < num_children - 2; ++c )
	{
		NodeInfo id = i->children[c].value.value();
		if( id.type != variable_type )
			throw DevaSemanticException( "Invalid 'new' statement", id );
	}
}

void check_class_decl( iter_t const & i )
{
	// is always the rhs of assignment op
	// 2 children: an identifier and an arg_list_exp
	NodeInfo id = i->children[0].value.value();
	if( i->children.size() < 2 )
		throw DevaSemanticException( "Invalid class definition.", id );
	if( id.type != variable_type )
		throw DevaSemanticException( "Invalid class definition", id );
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
	// must be inside of the compound_statement part of a loop construct (while/for)
	// if the loop-tracking stack is *empty*, we are not inside a loop at all
	if( in_loop_stack.size() == 0 )
		throw DevaSemanticException( "'break' statement only valid inside of a loop", i->value.value() );
}

void check_continue_statement( iter_t const & i )
{
	// must be inside of the compound_statement part of a loop construct (while/for)
	// if the loop-tracking stack is *empty*, we are not inside a loop at all
	if( in_loop_stack.size() == 0 )
		throw DevaSemanticException( "'continue' statement only valid inside of a loop", i->value.value() );
}

void check_return_statement( iter_t const & i )
{
}

