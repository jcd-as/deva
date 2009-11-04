// parser_ids.cpp
// define (storage for) ids for the grammar rules:
// created by jcs, september 9, 2009 

#include <boost/spirit.hpp>
using namespace boost::spirit;

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
parser_id map_op_id;
parser_id vec_op_id;
parser_id in_op_id;
// constants
parser_id string_id;
parser_id number_id;
parser_id boolean_id;
parser_id null_id;
parser_id constant_id;
parser_id func_id;
parser_id while_s_id;
parser_id for_s_id;
parser_id if_s_id;
parser_id else_s_id;
parser_id identifier_id;
parser_id module_name_id;

