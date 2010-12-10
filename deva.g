grammar deva;

// TBD:
// -

options 
{
//	language = Java;
//	ASTLabelType=CommonTree;
	language = C;
	ASTLabelType=pANTLR3_BASE_TREE;

	output = AST;
}

tokens
{
	// "imaginary" tokens for AST
	Vec_init;			// vector initializer
	Map_init;			// map initializer
	Pair;				// pair (map item)
	Base_classes;		// base class list
	Arg_list_decl;		// argument list declaration
	Call;				// function call
	Def_arg;			// define function argument 
	Key;				// index or slice with one, two or three args
	Condition;			// if condition
	Block;				// code block (body of if,else,while,for,function def)
	Negate;				// unary '-' operator, to reduce confusion with binary '-'
	
	NULLVAL = 'null';
}

@parser::includes
{
#include "inc/semantics.h"
}

@parser::apifuncs 
{
	RECOGNIZER->displayRecognitionError = devaDisplayRecognitionError;
	//RECOGNIZER->reportError = devaReportError;
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

block 
	:	compound_statement
	|	statement											-> ^(Block statement)
	;

compound_statement 
	:	lc='{' statement* '}'								-> ^(Block[$lc, "Block"] statement*)
	;

func_decl 
	:	'def' ID arg_list_decl compound_statement		-> ^('def' ID arg_list_decl compound_statement)
	|	'def' 'new' arg_list_decl compound_statement		-> ^('def' 'new' arg_list_decl compound_statement)
	;
	
class_decl 
	:	'class' ID (':' ID (',' ID)*)? '{' func_decl* '}' 	-> ^('class' ID ^(Base_classes ID+)? ^(Block func_decl*))
	;

while_statement 
	:	'while' '(' exp ')' block							-> ^('while' ^(Condition exp) block)
	;	

for_statement 
	:	'for' '(' in_exp ')' block							-> ^('for' in_exp block)
	;

if_statement
	:	'if' '(' exp ')' block else_statement?				-> ^('if' ^(Condition exp) block else_statement?)
	;

else_statement 
	:	'else' block										-> ^('else' block)
	;

import_statement 
	:	'import' module_name ';'							-> ^('import' module_name)
	;
	
jump_statement 
	:	break_statement
	|	continue_statement
	|	return_statement
	;

break_statement 
	:	'break' ';'!
	;

continue_statement 
	:	'continue' ';'!
	;

return_statement 
	:	'return' logical_exp ';'							-> ^('return' logical_exp)
	|	'return' ';'!
	;
		
assign_statement
	: 	const_decl '=' value ';'							-> ^(const_decl value)
	| 	(local_decl '=' 'new')=> local_decl '=' new_decl ';'	-> ^(local_decl new_decl)
	| 	local_decl '=' logical_exp ';'						-> ^(local_decl logical_exp)
	| 	(external_decl '=' 'new')=> external_decl '=' new_decl ';'	-> ^(external_decl new_decl)
	| 	external_decl '=' logical_exp ';'					-> ^(external_decl logical_exp)
	|	(primary_exp ';')=> primary_exp ';'!
	|	(primary_exp '=' 'new')=> primary_exp '=' new_decl ';'	-> ^('=' primary_exp new_decl)
	|	(primary_exp '=')=> primary_exp '='^ assign_rhs ';'!
	|	primary_exp math_assignment_op logical_exp ';'		-> ^(math_assignment_op primary_exp logical_exp)
	;

assign_rhs
	:	exp	('='^ assign_rhs)?
	;

/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////
	
exp
 	:	logical_exp
 	;

arg_list_decl
	: '(' (arg (',' arg)*)? ')'								-> ^(Arg_list_decl arg*)
	;

arg 
	:	ID ('=' default_arg_val)?							-> ^(Def_arg ID default_arg_val?)
	;

in_exp 
	:	ID (',' ID)? 'in' primary_exp						-> ^('in' ID+ primary_exp)
	;

const_decl 
	:	'const' ID											-> ^('const' ID)
	;

local_decl
	:	'local' ID											-> ^('local' ID)
	;

external_decl
	:	'extern' ID											-> ^('extern' ID)
	;

new_decl 
	:	'new' primary_exp
	;

logical_exp 
	:	relational_exp (logical_op^ relational_exp)*
	|	(map_op | vec_op)
	;

relational_exp 
	:	add_exp (relational_op^ add_exp)*
	;

add_exp
	:
	mul_exp (add_op^ mul_exp)*
	;
	
mul_exp
	:	unary_exp (mul_op^ unary_exp)*
	;

unary_exp 
	:	primary_exp
	|	'!'^ primary_exp
	|	'-' primary_exp										-> ^(Negate primary_exp)
	;	

// argument list use, not declaration ('()'s & contents)
primary_exp 
	:	(atom->atom)
		(
			args=arg_list_exp								-> ^(Call $primary_exp $args?)
		|	indices=key_exp									-> ^(Key $primary_exp $indices	)
		|	'.' id=ID										-> ^('.' $primary_exp $id)
		)*
	;

arg_list_exp
	:	
	'('! (exp (','! exp)*)? ')'!
	;

// map key (inside '[]'s) - only 'math' expresssions allowed inside, 
// not general expressions
key_exp
	:	'['! idx (( ':'! idx (':'! add_exp)? ))? ']'!
	;

idx 
	:	('$' | add_exp)
	;

// map construction op
map_op 
	:	'{' map_item? (',' map_item)* '}'					-> ^(Map_init map_item*)
	;

map_item 
	:	(key=exp ':' val=exp)								-> ^(Pair $key $val)
	;

// vector construction op
vec_op 
	:	'[' (exp (',' exp)*)? ']' 							-> ^( Vec_init exp*)
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
	|	'-'^ (value | ID)
		;
	
atom
	:	value
	|	ID
	| 	'('! exp ')'!
	;

add_op 
	:	('+' | '-')
	;
	
unary_op 
	:	'-' | '!'
	;

math_assignment_op 
	:	('+=' | '-=' | '*=' | '/=' | '%=')
	;

relational_op 
	:	'>=' | '<=' | '>' | '<' | '==' | '!='
	;

logical_op 
	:	'&&' | '||'
	;

mul_op 
	:	'*' | '/' | '%'
	;


/////////////////////////////////////////////////////////////////////////////
// TOKENS
/////////////////////////////////////////////////////////////////////////////
NUMBER 
	:	HEX_INT
	|	BIN_INT
	|	OCT_INT
	|	EFLOAT
	|	FLOAT
	|	INT
	;

STRING 
    :	'"' (ESC_SEQ | ~('\\'|'"'))* '"'
    |	'\'' (ESC_SEQ | ~('\\'|'\''))* '\''
    ;

BOOL 
	:	'true' | 'false'
	;

ID  :	(ALPHA | '_') (ALNUM | '_')*
    ;

WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;}
    ;

COMMENT 
	:	'#' (~'\n')* {$channel=HIDDEN;}
	;

/////////////////////////////////////////////////////////////////////////////
// FRAGMENTS
/////////////////////////////////////////////////////////////////////////////
fragment ALPHA 
	:	'a'..'z'|'A'..'Z'
	;
	
fragment HEX_INT 
	:	'0x' HEX_DIGIT+
	;

fragment BIN_INT 
	:	'0b' ('0'|'1')+
	;
	
fragment OCT_INT
	:	'0o' OCT_DIGIT+
	;

fragment EFLOAT 
	:	FLOAT ('E'|'e')  INT
	;

fragment FLOAT 
	:	INT '.' DEC_DIGIT+
	;

fragment INT 
	:	DEC_DIGIT+
    ;

fragment
DEC_DIGIT 
	:	('0'..'9')
	;

fragment
HEX_DIGIT
	:	('0'..'9'|'a'..'f'|'A'..'F')
	;

fragment
OCT_DIGIT
	:	('0'..'7')
	;

fragment
ALNUM 
	:	ALPHA | DEC_DIGIT
	;

ESC_SEQ
	: '\\' .
	;
