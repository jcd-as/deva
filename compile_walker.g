tree grammar compile_walker;

options
{
	language = C;
	ASTLabelType=pANTLR3_BASE_TREE;
	tokenVocab=deva;
}

@header
{
// Copyright (c) 2010 Joshua C. Shepard
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
}
@includes
{
#include "inc/semantics.h"
#include "inc/compile.h"
}

@apifuncs 
{
	RECOGNIZER->displayRecognitionError = devaDisplayRecognitionError;
}


/////////////////////////////////////////////////////////////////////////////
// STATEMENTS
/////////////////////////////////////////////////////////////////////////////

translation_unit
@after { compiler->Emit( op_halt ); }
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
@init { compiler->EnterBlock(); }
@after { compiler->ExitBlock(); }
	:	^(Block statement*)
	;

func_decl
@init { compiler->fcn_nesting++; }
@after { compiler->fcn_nesting--; compiler->LeaveScope(); }
	:	^(Def id=ID 
		{ compiler->AddScope(); compiler->DefineFun( (char*)$id.text->chars, $id->getLine($id) ); }
		arg_list_decl block) 
	|	^(Def id='new' 
		{ compiler->AddScope(); compiler->DefineFun( const_cast<char*>("new"), $id->getLine($id) ); }
		arg_list_decl block)
	;
	
class_decl 
@init { compiler->in_class = true; }
@after { compiler->in_class = false; }
	:	^(Class id=ID 
		{ compiler->DefineClass( (char*)$id.text->chars, $id->getLine($id) ); }
		(^(Base_classes ID+))? func_decl*)
	;

while_statement 
	:	^(While ^(Condition con=exp) block)
	;

for_statement 
@init { compiler->AddScope(); }
@after { compiler->LeaveScope(); }
	:	^(For in_exp block)
	;

if_statement
	:	^(If ^(Condition con=exp) block else_statement?)
	;

else_statement 
	:	^(Else block)
	;

import_statement 
	:	^(Import module_name)
	;
	
jump_statement 
	:	break_statement
	|	continue_statement
	|	return_statement
	;

break_statement 
	:	brk=Break
	;

continue_statement 
	:	con=Continue
	;

return_statement 
	:	^(Return exp)
	|	Return
	;

assign_statement
	: 	^(Const id=ID value)
	|	(^(Local ID new_exp))=> ^(Local id=ID new_exp)
	|	^(Local id=ID exp) { compiler->LocalVar( (char*)$id.text->chars ); }
	|	(^(Extern ID new_exp))=> ^(Extern id=ID new_exp)
	|	^(Extern id=ID exp?)
	|	(^('=' exp new_exp))=> ^('=' lhs=exp new_exp)
	|	^('=' lhs=exp (exp|assign_rhs))
	|	^(ADD_EQ_OP lhs=exp exp)
	|	^(SUB_EQ_OP lhs=exp exp)
	|	^(MUL_EQ_OP lhs=exp exp)
	|	^(DIV_EQ_OP lhs=exp exp)
	|	^(MOD_EQ_OP lhs=exp exp)
	|	exp
	;

assign_rhs 
	:	^('=' lhs=exp (assign_rhs|exp))
	;
	
new_exp
	:	^(New exp)
	;
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp
	:	^(GT_EQ_OP lhs=exp rhs=exp)
	|	^(LT_EQ_OP lhs=exp rhs=exp)
	|	^(GT_OP lhs=exp rhs=exp)
	|	^(LT_OP lhs=exp rhs=exp)
	|	^(EQ_OP lhs=exp rhs=exp)
	|	^(NOT_EQ_OP lhs=exp rhs=exp)
	|	^(AND_OP lhs=exp rhs=exp)
	|	^(OR_OP lhs=exp rhs=exp)
	|	^(ADD_OP lhs=exp rhs=exp) { compiler->AddOp(); }
	|	^(SUB_OP lhs=exp rhs=exp) { compiler->SubOp(); }
	|	^(MUL_OP lhs=exp rhs=exp) { compiler->MulOp(); }
	|	^(DIV_OP lhs=exp rhs=exp) { compiler->DivOp(); }
	|	^(MOD_OP lhs=exp rhs=exp) { compiler->ModOp(); }
	|	^(Negate in=exp) { compiler->NegateOp(); }
	|	^(NOT_OP in=exp) { compiler->NotOp(); }
	|	^(Key exp key_exp)
	|	^(DOT_OP exp ID)
	|	call_exp
	|	(map_op | vec_op)
	|	value
	|	ID
	;

call_exp
	:	^(Call exp exp*)
	;

key_exp
	:	(idx idx idx)=> idx1=idx idx2=idx idx3=exp
	|	(idx idx)=> idx1=idx idx2=idx
	|	idx1=idx
	;

idx 
	:	(END_OP | exp)
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg id=ID default_arg_val?)
	;

in_exp 
	:	^(In key=ID val=ID? exp)
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

module_name 
	:	(ID '/')* nm=ID
	;

value
	:	BOOL { if( strcmp( (char*)$BOOL.text->chars, "true" ) == 0 ) compiler->Emit( op_push_true ); else compiler->Emit( op_push_false ); }
	|	NULLVAL { compiler->Emit( op_push_null ); }
	|	NUMBER { compiler->Number( atof( (char*)$NUMBER.text->chars ) ); }
	|	STRING { compiler->String( (char*)$STRING.text->chars ); }
	;

default_arg_val
	:	(
			BOOL { compiler->DefaultArgVal( $BOOL ); }
			| NULLVAL { compiler->DefaultArgVal( $NULLVAL ); }
			| NUMBER { compiler->DefaultArgVal( $NUMBER ); }
			| STRING  { compiler->DefaultArgVal( $STRING ); }
		)
	|	ID { compiler->DefaultArgId( $ID ); }
	|	^(Negate NUMBER) { compiler->DefaultArgVal( $NUMBER, true ); }
	;

