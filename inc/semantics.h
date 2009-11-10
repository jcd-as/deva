// semantics.h
// semantic analysis functions for the deva language
// created by jcs, september 12, 2009 

// TODO:
// * 

#ifndef __SEMANTICS_H__
#define __SEMANTICS_H__

#include "types.h"

// declare semantic analysis functions
void check_number( iter_t const & i );
void check_string( iter_t const & i );
void check_boolean( iter_t const & i );
void check_null( iter_t const & i );
void check_func( iter_t const & i );
void pre_check_while_s( iter_t const & i );
void check_while_s( iter_t const & i );
void pre_check_for_s( iter_t const & i );
void check_for_s( iter_t const & i );
void check_if_s( iter_t const & i );
void check_else_s( iter_t const & i );
void check_import_s( iter_t const & i );
void check_identifier( iter_t const & i );
void check_module_name( iter_t const & i );
void check_in_op( iter_t const & i );
void check_map_op( iter_t const & i );
void check_vec_op( iter_t const & i );
void check_semicolon_op( iter_t const & i );
void check_assignment_op( iter_t const & i );
void check_op_assignment_op( iter_t const & i );
void check_logical_op( iter_t const & i );
void check_relational_op( iter_t const & i );
void check_mult_op( iter_t const & i );
void check_add_op( iter_t const & i );
void check_unary_op( iter_t const & i );
void check_dot_op( iter_t const & i );
void check_paren_op( iter_t const & i );
void check_bracket_op( iter_t const & i );
void check_arg_list_exp( iter_t const & i );
void check_arg_list_decl( iter_t const & i );
void check_arg( iter_t const & i );
void check_key_exp( iter_t const & i );
void pre_check_const_decl( iter_t const & i );
void check_const_decl( iter_t const & i );
void check_new_decl( iter_t const & i );
void check_class_decl( iter_t const & i );
void check_constant( iter_t const & i );
void check_translation_unit( iter_t const & i );
void check_compound_statement( iter_t const & i );
void check_break_statement( iter_t const & i );
void check_continue_statement( iter_t const & i );
void check_return_statement( iter_t const & i );

#endif // __SEMANTICS_H__

