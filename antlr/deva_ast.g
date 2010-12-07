tree grammar deva_ast;

options
{
	tokenVocab=deva;
	ASTLabelType=CommonTree;
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
	:	compound_statement
	|	while_statement
	|	for_statement
	|	if_statement
	|	import_statement
	|	jump_statement
	|	func_decl
	|	assign_statement
	;

compound_statement 
	:	^(Block statement*)
	;

func_decl 
	:	^('def' ID arg_list_decl compound_statement)
	|	^('def' 'new' arg_list_decl compound_statement)
	;
	
class_decl 
	:	^('class' ID (^(Base_classes ID+))? func_decl*)
	;

while_statement 
	:	^('while' ^(Condition exp) statement)
	;

for_statement 
	:	^('for' in_exp statement)
	;

if_statement
	:	^('if' ^(Condition exp) statement)
	;

else_statement 
	:	^('else' statement)
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
		
assign_statement
	: 	^('const' exp value)
	|	^('local' exp 'new'? exp?)
//	|	^(assignment_op exp assign_or_new)
	|	^(assignment_op exp 'new'? exp)
	|	exp
	;
	
assign_or_new
	:	new_decl
	|	exp
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
	:	index (index (exp)? )? 
	;

index 
	:	('$' | exp)
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg ID default_arg_val?)
	;

in_exp 
	:	^('in' exp+)
	;

const_decl 
	:	^('const' ID)
	;

local_decl
	:	^('local' ID)
	;

new_decl 
	:	'new' exp
	;

map_op 
	:	^(Map_init map_item*)
	;

map_item 
	:	^(exp exp)
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
	:	BOOL | NULL | NUMBER | STRING
	;

default_arg_val
	:	value | ID
	;

math_assignment_op 
	:	('+=' | '-=' | '*=' | '/=' | '%=')
	;