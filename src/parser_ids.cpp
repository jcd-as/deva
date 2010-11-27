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

// parser_ids.cpp
// define (storage for) ids for the grammar rules:
// created by jcs, september 9, 2009 

#include <boost/spirit/include/classic.hpp>
using namespace boost::spirit::classic;

// statements
parser_id translation_unit_id;
parser_id top_level_statement_id;
parser_id statement_id;
parser_id if_statement_id;
parser_id else_statement_id;
parser_id import_statement_id;
parser_id while_statement_id;
parser_id for_statement_id;
parser_id jump_statement_id;
parser_id break_statement_id;
parser_id continue_statement_id;
parser_id return_statement_id;
parser_id func_decl_id;
parser_id class_decl_id;
parser_id compound_statement_id;
parser_id exp_statement_id;
parser_id open_brace_op_id;
parser_id close_brace_op_id;
parser_id semicolon_op_id;
// expressions
parser_id exp_id;
parser_id const_decl_id;
parser_id local_decl_id;
parser_id new_decl_id;
parser_id assignment_exp_id;
parser_id logical_exp_id;
parser_id relational_exp_id;
parser_id add_exp_id;
parser_id mult_exp_id;
parser_id unary_exp_id;
parser_id postfix_exp_id;
parser_id postfix_only_exp_id;
parser_id arg_list_exp_id;
parser_id arg_list_decl_id;
parser_id arg_id;
parser_id key_exp_id;
parser_id in_exp_id;
parser_id primary_exp_id;
parser_id factor_exp_id;
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
parser_id add_assignment_op_id;
parser_id sub_assignment_op_id;
parser_id mul_assignment_op_id;
parser_id div_assignment_op_id;
parser_id mod_assignment_op_id;
parser_id map_op_id;
parser_id vec_op_id;
parser_id in_op_id;
parser_id end_op_id;
// constants
parser_id string_id;
parser_id number_id;
parser_id boolean_id;
parser_id null_id;
parser_id constant_id;
parser_id local_id;
parser_id func_id;
parser_id while_s_id;
parser_id for_s_id;
parser_id if_s_id;
parser_id else_s_id;
parser_id identifier_id;
parser_id module_name_id;

