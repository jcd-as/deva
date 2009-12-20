// Copyright (c) 2009 Joshua C. Shepard
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// debug.cpp
// debug functions for deva language compiler
// created by jcs, september 9, 2009

// TODO:
// * 

#include "debug.h"

#include <iostream>
#include <string>

// output whether something succeeded or failed
int print_parsed( bool parsed )
{
	if( parsed )
	{
		cout << "parsed successfully" << endl;
		return 0;
	}
	else
	{
		cout << "parse failed!" << endl;
		return 1;
	}
}

void indent( int ind )
{
	for( int i = 0; i < ind; i++ )
		cout << '\t';
}

void dumpNode( const char* name, iter_t const& i, int & indents )
{
	// dump scope and line information
	indent( indents );
	cout << "in scope " << i->value.value().scope << ", at line: " << i->value.value().line << endl;
//	cout << "in file " << i->value.value().file << endl;
	// dump node name
	indent( indents );
	string s( i->value.begin(), i->value.end() );
	cout << name << ": " << strip_symbol( s ) << endl;
	// dump node children
	indents++;
	for( int c = 0; c < i->children.size(); c++ )
		eval_expression( i->children.begin() + c );
	indents--;
}

void eval_expression( iter_t const& i )
{
	static int indents = 0;

	indent( indents );
    cout << "In eval_expression. i->value = " <<
        strip_symbol( string(i->value.begin(), i->value.end()) ) <<
        " i->children.size() = " << i->children.size() << endl;

	// number
    if( i->value.id() == parser_id( number_id ) )
    {
		dumpNode( "num", i, indents );
    }
	// string
	else if( i->value.id() == parser_id( string_id ) )
	{
		dumpNode( "string", i, indents );
	}
	// boolean
	else if( i->value.id() == parser_id( boolean_id ) )
	{
		dumpNode( "boolean", i, indents );
	}
	// null
	else if( i->value.id() == parser_id( null_id ) )
	{
		dumpNode( "null", i, indents );
	}
	// constant (keyword 'const')
	else if( i->value.id() == parser_id( constant_id ) )
	{
		dumpNode( "const", i, indents );
	}
	// local (keyword 'local')
	else if( i->value.id() == parser_id( local_id ) )
	{
		dumpNode( "local", i, indents );
	}
	// func (keyword 'def' for defining functions)
	else if( i->value.id() == parser_id( func_id ) )
	{
		dumpNode( "func", i, indents );
	}
	// while_s (keyword 'while')
	else if( i->value.id() == parser_id( while_s_id ) )
	{
		dumpNode( "while_s", i, indents );
	}
	// for_s (keyword 'for')
	else if( i->value.id() == parser_id( for_s_id ) )
	{
		dumpNode( "for_s", i, indents );
	}
	// if_s (keyword 'if')
	else if( i->value.id() == parser_id( if_s_id ) )
	{
		dumpNode( "if_s", i, indents );
	}
	// else_s (keyword 'else')
	else if( i->value.id() == parser_id( else_s_id ) )
	{
		dumpNode( "else_s", i, indents );
	}
	// import
	else if( i->value.id() == parser_id( import_statement_id ) )
	{
		dumpNode( "import", i, indents );
	}
	// identifier
	else if( i->value.id() == parser_id( identifier_id ) )
	{
		dumpNode( "identifier", i, indents );
	}
	// module_name
	else if( i->value.id() == parser_id( module_name_id ) )
	{
		dumpNode( "module_name", i, indents );
	}
	// in op ('in' keyword in for loops)
	else if( i->value.id() == parser_id( in_op_id ) )
	{
		dumpNode( "in_op", i, indents );
	}
	// map construction op
	else if( i->value.id() == parser_id( map_op_id ) )
	{
		dumpNode( "map_op", i, indents );
	}
	// vector construction op
	else if( i->value.id() == parser_id( vec_op_id ) )
	{
		dumpNode( "vec_op", i, indents );
	}
	// semicolon op
	else if( i->value.id() == parser_id( semicolon_op_id ) )
	{
		dumpNode( "semicolon_op", i, indents );
	}
	// assignment op
	else if( i->value.id() == parser_id( assignment_op_id ) )
	{
		dumpNode( "assignment_op", i, indents );
	}
	// logical op
	else if( i->value.id() == parser_id( logical_op_id ) )
	{
		dumpNode( "logical_op", i, indents );
	}
	// relational op
	else if( i->value.id() == parser_id( relational_op_id ) )
	{
		dumpNode( "relational_op", i, indents );
	}
	// mult_op
	else if( i->value.id() == parser_id( mult_op_id ) )
	{
		dumpNode( "mult_op", i, indents );
	}
	// add_op
	else if( i->value.id() == parser_id( add_op_id ) )
	{
		dumpNode( "add_op", i, indents );
	}
	// unary_op
	else if( i->value.id() == parser_id( unary_op_id ) )
	{
		dumpNode( "unary_op", i, indents );
	}
	// dot op
	else if( i->value.id() == parser_id( dot_op_id ) )
	{
		dumpNode( "dot_op", i, indents );
	}
	// paren ops
	else if( i->value.id() == parser_id( open_paren_op_id )
		  || i->value.id() == parser_id( close_paren_op_id ) )
	{
		dumpNode( "(open|close)_paren_op", i, indents );
	}
	// bracket ops
	else if( i->value.id() == parser_id( open_bracket_op_id ) 
			|| i->value.id() == parser_id( close_bracket_op_id ) )
	{
		dumpNode( "(open|close)_bracket_op", i, indents );
	}
	// brace ops
	else if( i->value.id() == parser_id( open_brace_op_id ) 
			|| i->value.id() == parser_id( close_brace_op_id ) )
	{
		dumpNode( "(open|close)_brace_op", i, indents );
	}
	// comma_op
	else if( i->value.id() == parser_id( comma_op_id ) )
	{
		dumpNode( "comma_op", i, indents );
	}
	// factor_exp
	else if( i->value.id() == parser_id( factor_exp_id ) )
	{
		dumpNode( "factor_exp", i, indents );
	}
	// postfix only exp
//	else if( i->value.id() == parser_id( postfix_only_exp_id ) )
//	{
//	}
	// primary_exp
//	else if( i->value.id() == parser_id( primary_exp_id ) )
//	{
//	}
//	// mult_exp
//	else if( i->value.id() == parser_id( mult_exp_id ) )
//	{
//	}
//	// add_exp
//	else if( i->value.id() == parser_id( add_exp_id ) )
//	{
//	}
//	// relational exp
//	else if( i->value.id() == parser_id( relational_exp_id ) )
//	{
//	}
//	// logical exp
//	else if( i->value.id() == parser_id( logical_exp_id ) )
//	{
//	}
	// arg list exp
	else if( i->value.id() == parser_id( arg_list_exp_id ) )
	{
		dumpNode( "arg_list_exp", i, indents );
	}
	// arg list decl
	else if( i->value.id() == parser_id( arg_list_decl_id ) )
	{
		dumpNode( "arg_list_decl", i, indents );
	}
	// arg 
	else if( i->value.id() == parser_id( arg_id ) )
	{
		dumpNode( "arg", i, indents );
	}
	// key exp
	else if( i->value.id() == parser_id( key_exp_id ) )
	{
		dumpNode( "key_exp", i, indents );
	}
	// const decl
	else if( i->value.id() == parser_id( const_decl_id ) )
	{
		dumpNode( "const_decl", i, indents );
	}
	// local decl
	else if( i->value.id() == parser_id( local_decl_id ) )
	{
		dumpNode( "local_decl", i, indents );
	}
	// new op
	else if( i->value.id() == parser_id( new_decl_id ) )
	{
		dumpNode( "new_decl", i, indents );
	}
	// exp
//	else if( i->value.id() == parser_id( exp_id ) )
//	{
//	}
//	// statement
//	else if( i->value.id() == parser_id( statement_id ) )
//	{
//	}
//	// top-level statement
//	else if( i->value.id() == parser_id( top_level_statement_id ) )
//	{
//	}
	// translation unit
	else if( i->value.id() == parser_id( translation_unit_id ) )
	{
		dumpNode( "translation_unit", i, indents );
	}
	// function decl
	else if( i->value.id() == parser_id( func_decl_id ) )
	{
		dumpNode( "func_decl", i, indents );
	}
	// class decl
	else if( i->value.id() == parser_id( class_decl_id ) )
	{
		dumpNode( "class_decl", i, indents );
	}
//	// while statement
//	else if( i->value.id() == parser_id( while_statement_id ) )
//	{
//		dumpNode( "while_statement", i, indents );
//	}
//	// for statement
//	else if( i->value.id() == parser_id( for_statement_id ) )
//	{
//		dumpNode( "for_statement", i, indents );
//	}
//	// if statement
//	else if( i->value.id() == parser_id( if_statement_id ) )
//	{
//		dumpNode( "if_statement", i, indents );
//	}
//	// else statement
//	else if( i->value.id() == parser_id( else_statement_id ) )
//	{
//		dumpNode( "else_statement", i, indents );
//	}
	// compound statement
	else if( i->value.id() == parser_id( compound_statement_id ) )
	{
		dumpNode( "compound_statement", i, indents );
	}
	// jump statement
	else if( i->value.id() == parser_id( jump_statement_id ) )
	{
		dumpNode( "jump_statement", i, indents );
	}
	// break statement
	else if( i->value.id() == parser_id( break_statement_id ) )
	{
		dumpNode( "break_statement", i, indents );
	}
	// continue statement
	else if( i->value.id() == parser_id( continue_statement_id ) )
	{
		dumpNode( "continue_statement", i, indents );
	}
	// return statement
	else if( i->value.id() == parser_id( return_statement_id ) )
	{
		dumpNode( "return_statement", i, indents );
	}
    else
    {
		// don't indent, so we stand out
		cout << "error, unknown id: " << i->value.id().to_long() << endl;
        string s( i->value.begin(), i->value.end() );
		cout << "unknown: " << strip_symbol( s ) << endl;
		indents++;
		for( int c = 0; c < i->children.size(); c++ )
			eval_expression( i->children.begin() + c );
		indents--;
    }
}

void evaluate( tree_parse_info<iterator_t, factory_t> info )
{
    eval_expression( info.trees.begin() );
}

