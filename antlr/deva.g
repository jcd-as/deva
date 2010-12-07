grammar deva;

// TBD:
// - line change nodes/hook for debug info ?

options 
{
//	language = C;
	output = AST;
	ASTLabelType=CommonTree;
}

tokens
{
	// "imaginary" tokens for AST
	Vec_init;
	Map_init;
	Base_classes;
	Arg_list_decl;
	Call;
	Def_arg;
	Key;				// index or slice with one, two or three args
	Condition;
	Block;
	Negate;				// unary '-' operator, to reduce confusion with binary '-'
	
	NULL = 'null';
}

// prevent antlr from trying to recover from syntax errors:
@members 
{
protected void mismatch( IntStream input, int ttype, BitSet follow )
throws RecognitionException
{
	throw new MismatchedTokenException( ttype, input );
}
public Object recoverFromMismatchedSet(IntStream input, RecognitionException e, BitSet follow )
throws RecognitionException
{
	throw e;
}
}
// Alter code generation so catch-clauses get replace with
// this action.
@rulecatch
{
catch (RecognitionException e)
{
	reportError( e );
	throw e;
}
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
	:	lc='{' statement* '}'								-> ^(Block[$lc, "Block"] statement*)
	;

func_decl 
	:	'def' ID arg_list_decl compound_statement		-> ^('def' ID arg_list_decl compound_statement)
	|	'def' 'new' arg_list_decl compound_statement		-> ^('def' 'new' arg_list_decl compound_statement)
	;
	
class_decl 
	:	'class' ID (':' ID (',' ID)*)? '{' func_decl* '}' -> ^('class' ID ^(Base_classes ID+)? func_decl*)
	;

while_statement 
	:	'while' '(' exp ')' statement					-> ^('while' ^(Condition exp) statement)
	;

for_statement 
	:	'for' '(' in_exp ')' statement					-> ^('for' in_exp statement)
	;

if_statement
	:	'if' '(' exp ')' statement else_statement?		-> ^('if' ^(Condition exp) statement else_statement?)
	;

else_statement 
	:	'else' statement								-> ^('else' statement)
	;

import_statement 
	:	'import' module_name ';'						-> ^('import' module_name)
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
	:	'return' logical_exp ';'						-> ^('return' logical_exp)
	|	'return' ';'!
	;
		
// TODO: 
// - prevent new_decl's from parsing with +=, -= etc ?
// - prevent 'a[0];' (for instance) from being parsed as a statement ?
assign_statement
	: 	const_decl '=' value ';'						-> ^(const_decl value)
	| 	(local_decl '=' 'new')=> local_decl '=' new_decl ';'	-> ^(local_decl new_decl)
	| 	local_decl '=' logical_exp ';'					-> ^(local_decl logical_exp)
	|	(primary_exp ';')=> primary_exp ';'!
	|	primary_exp assignment_op assign_or_new ';'		-> ^(assignment_op primary_exp assign_or_new)
	;
	
assign_or_new
	:	new_decl
	|	logical_exp
	;


/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////
	
exp
 	:	logical_exp
 	;

arg_list_decl
	: '(' (arg (',' arg)*)? ')'							-> ^(Arg_list_decl arg*)
	;

arg 
	:	ID ('=' default_arg_val)?						-> ^(Def_arg ID default_arg_val?)
	;

in_exp 
	:	exp (',' exp)? 'in' exp							-> ^('in' exp+)
	;

const_decl 
	:	'const' ID										-> ^('const' ID)
	;

local_decl
	:	'local' ID										-> ^('local' ID)
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
	|	'-' primary_exp									-> ^(Negate primary_exp)
	;	

// argument list use, not declaration ('()'s & contents)
primary_exp 
	:	(atom->atom)
		(
			args=arg_list_exp							-> ^(Call $primary_exp $args?)
		|	indices=key_exp								-> ^(Key $primary_exp $indices	)
		|	'.' id=ID									-> ^('.' $primary_exp $id)
		)*
	;

arg_list_exp
	:	
	'('! (exp (','! exp)*)? ')'!
	;

// map key (inside '[]'s) - only 'math' expresssions allowed inside, 
// not general expressions
key_exp
	:	'['! index (( ':'! index (':'! add_exp)? ))? ']'!
	;

index 
	:	('$' | add_exp)
	;

// map construction op
map_op 
	:	'{' map_item? (',' map_item)* '}'				-> ^(Map_init map_item*)
	;

map_item 
	:	(key=exp ':' val=exp)							-> ^($key $val)
	;

// vector construction op
vec_op 
	:	'[' (exp (',' exp)*)? ']' 						-> ^( Vec_init exp*)
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
