tree grammar semantic_walker;

options
{
	language = C;
	ASTLabelType=pANTLR3_BASE_TREE;
	tokenVocab=deva;
}

@includes
{
#include "inc/semantics.h"
}


/////////////////////////////////////////////////////////////////////////////
// STATEMENTS
/////////////////////////////////////////////////////////////////////////////

translation_unit
	: top_level_statement*
	EOF
	;

top_level_statement
	:	class_decl
	|	statement
	;

statement 
	:	block
	|	while_statement
	|	for_statement
	|	if_statement
	|	import_statement
	|	jump_statement
	|	func_decl
	|	assign_statement
	;

block 
@init { devaPushScope( semantics->deva_function_name ); semantics->deva_in_function--; semantics->deva_arg_names.clear(); }
@after { devaPopScope(); }
	:	^(Block statement*)
	;

func_decl
@init { semantics->deva_in_function++; }
	:	^('def' ID arg_list_decl block) { semantics->deva_function_name = (char*)$ID.text->chars; }
	|	^('def' 'new' arg_list_decl block) { semantics->deva_function_name = "new"; }
	;
	
class_decl 
	:	^('class' ID (^(Base_classes ID+))? block)
	;

while_statement 
	:	^('while' ^(Condition exp) block)
	;

for_statement 
	:	^('for' in_exp block)
	;

if_statement
	:	^('if' ^(Condition exp) block else_statement?)
	;

else_statement 
	:	^('else' block)
	;

import_statement 
	:	^('import' module_name)
	;
	
jump_statement 
	:	break_statement
	|	continue_statement
	|	return_statement
	;

break_statement 
	:	'break'
	;

continue_statement 
	:	'continue'
	;

return_statement 
	:	^('return' exp)
	|	'return'
	;

// TODO: move 'devaCheckVarForAssign' call to use of vars in exps, not
// assignment
assign_statement
	: 	^('const' id=ID value) { devaDefineVar( (char*)$id.text->chars, $id->getLine($id), mod_constant ); }
	|	^('local' id=ID 'new'? exp) { devaDefineVar( (char*)$id.text->chars, $id->getLine($id), mod_local ); }
	|	^('extern' id=ID 'new'? exp) { devaDefineVar( (char*)$id.text->chars, $id->getLine($id), mod_external ); }
	|	^('=' exp 'new'? (exp|assign_rhs)) //{ devaCheckVarForAssign( (char*)$id.text->chars, $id.start->getLine($id.start) ); }
	|	^(math_assignment_op exp exp)
	|	exp
	;


assign_rhs 
	:	^('=' exp (assign_rhs|exp))
	;
	
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp
	:	^('>=' exp exp)
	|	^('<=' exp exp)
	|	^('==' exp exp)
	|	^('!=' exp exp)
	|	^('>' exp exp)
	|	^('<' exp exp)
	|	^('&&' exp exp)
	|	^('||' exp exp)
	|	^('+' exp exp)
	|	^('-' exp exp)
	|	^('*' exp exp)
	|	^('/' exp exp)
	|	^('%' exp exp)
	|	^(Negate exp)
	|	^('!' exp)
	|	^(Call exp+)
	|	^(Key exp key_exp)
	|	^('.' exp ID)
	|	(map_op | vec_op)
	|	value
	|	ID
	;

key_exp
	:	idx (idx (exp)? )? 
	;

idx 
	:	('$' | exp)
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg id=ID default_arg_val?) { devaAddArg( (char*)$id.text->chars, $id->getLine($id) ); }
	;

in_exp 
	:	^('in' exp+)
	;

map_op 
	:	^(Map_init map_item*)
	;

map_item 
	:	^(Pair exp exp)
	;

vec_op 
	:	^(Vec_init exp*)
	;

assignment_op 
	:	'=' | math_assignment_op
	;	

module_name 
	:	ID ('/' ID)*
	;

value
	:	BOOL | NULLVAL | NUMBER | STRING
	;

default_arg_val
	:	value | ID
	|	^('-' (value | ID))
	;

math_assignment_op 
	:	('+=' | '-=' | '*=' | '/=' | '%=')
	;

 
