grammar deva;
options 
{
//	language = C;
	output = AST;
	ASTLabelType=CommonTree;
}

tokens
{
	Vec_Init;	// "imaginary" token for vector initialization
	NULL = 'null';
}

/////////////////////////////////////////////////////////////////////////////
// STATEMENTS
/////////////////////////////////////////////////////////////////////////////

translation_unit
	: top_level_statement* {System.out.println( $top_level_statement.tree == null ? "null" : $top_level_statement.tree.toStringTree() ); }
	EOF
	;

top_level_statement
	:	class_decl
	|	statement
	;

statement 
	:	assign_stat ';'!
	|	compound_statement
	|	while_statement
	|	for_statement
	|	func_decl
	;

compound_statement 
	:	'{'! statement* '}'!
	;

func_decl 
	:	'def' ID arg_list_decl compound_statement
	;

class_decl 
	:	'class' ID (':' ID (',' ID)*)?
	'{'
	func_decl*
	'}'
	;

assign_stat
	: exp -> exp
	| ID '=' exp -> ^('=' ID exp)
	| ID '=' vec_init -> ^('=' vec_init)
	;

while_statement 
	:	'while' '(' exp ')' statement
	;

for_statement 
	:	'for' '(' exp (',' exp)? in_exp ')' statement
	;




/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////
	
exp
// 	:	add_exp
 	:	logical_exp
 	;
 		
vec_init 
	: '[' vec_element  (',' vec_element)* ']' -> ^( Vec_Init vec_element+)
	;

vec_element
	: atom
	| vec_init
	;

arg_list_decl
	: '(' (arg (',' arg)*)? ')'
	;

arg 
	:	ID ('=' (BOOL | NULL | ID | NUMBER | STRING))?
	;

in_exp 
	:	'in' exp
	;

logical_exp 
	:	relational_exp (LOGICAL_OP relational_exp)*
//	| (map_op | vec_op)
	;

relational_exp 
	:	add_exp (RELATIONAL_OP add_exp)*
	;

add_exp
	:
	mul_exp (ADD_OP mul_exp)*
	;
	
mul_exp
	:	unary_exp (MUL_OP unary_exp)* 	//atom (MUL_OP^ atom)*
	;
	
unary_exp 
	:	postfix_exp
	|	(UNARY_OP postfix_exp)
	;

postfix_exp 
	:	postfix_only_exp ('.' postfix_only_exp)*
	|	primary_exp
	;
	
postfix_only_exp 
	:	ID (arg_list_exp | key_exp)?
	;

// argument list use, not declaration ('()'s & contents)
arg_list_exp 
	:	
	'(' (exp (',' exp)*)? ')'
	;

// map key (inside '[]'s) - only 'math' expresssions allowed inside, 
// not general expressions
key_exp 
	:	'[' ('$' | add_exp) (( ':' ('$' | add_exp) (':' add_exp)? ))? ']' 
	;

primary_exp 
	:	BOOL
	| 	NULL
	| 	ID
	| 	NUMBER
	| 	STRING
	|	'(' exp ')'
	;

atom
	:
	NUMBER
	| ID
	| STRING
	| '('! exp ')'!
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

STRING :  '"' ( ESC_SEQ | ~('\\'|'"') )* '"'
    ;

BOOL 
	:	'true' | 'false'
	;

ADD_OP 
	:	'+' | '-'
	;
	
UNARY_OP 
	:	'-' | '!'
	;

RELATIONAL_OP 
	:	'>=' | '<=' | '>' | '<' | '==' | '!='
	;

LOGICAL_OP 
	:	'&&' | '||'
	;

MUL_OP 
	:	'*' | '/' | '%'
	;

ID  :	(ALPHA | '_') (ALNUM | '_')*
    ;

MODULE_NAME 
	:	ID ('/' ID)*
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

fragment INT :	('+'|'-')? DEC_DIGIT+
    ;

fragment
DEC_DIGIT :	 ('0'..'9') ;

fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
OCT_DIGIT : ('0'..'7');	

fragment
ALNUM 
	:	ALPHA | DEC_DIGIT
	;

fragment
ESC_SEQ 
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    |   UNICODE_ESC
    |   OCTAL_ESC
    ;

fragment
OCTAL_ESC
    :   '\\' ('0'..'3') ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7')
    ;

fragment
UNICODE_ESC :   '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;
