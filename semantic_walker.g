tree grammar semantic_walker;

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
}

@apifuncs 
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
	:	block
	|	while_statement
	|	for_statement
	|	if_statement
	|	import_statement
	|	jump_statement
	|	func_decl
	|	assign_statement { semantics->CheckForNoEffect( $assign_statement.start ); }
	;

block 
@init { semantics->arg_names.clear(); semantics->PushScope(); }
@after { semantics->PopScope(); }
	:	^(Block statement*)
	;

func_decl
@after { semantics->PopScope(); }
	:	^(Def id=ID 
		{ semantics->DefineFun( (char*)$id.text->chars, $id->getLine($id) ); semantics->PushScope( (char*)$ID.text->chars ); if( semantics->in_class ) semantics->DefineVar( const_cast<char*>("self"), $id->getLine($id) ); }
		arg_list_decl block) 
	|	^(Def id='new' 
		{ semantics->DefineFun( const_cast<char*>("new"), $id->getLine($id) ); semantics->PushScope( const_cast<char*>("new") ); if( semantics->in_class ) semantics->DefineVar( const_cast<char*>("self"), $id->getLine($id) ); }
		arg_list_decl block)
	;
	
class_decl 
@after { semantics->in_class = false; }
	:	^(Class id=ID 
		{ semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id) ); semantics->in_class = true; }
		(^(Base_classes ID+))? block)
	;

while_statement 
@init { semantics->in_loop++; }
@after { semantics->in_loop--; }
	:	^(While ^(Condition con=exp) block) { semantics->CheckConditional( $con.start ); }
	;

for_statement 
@init { semantics->in_loop++; semantics->PushScope(); }
@after { semantics->in_loop--; semantics->PopScope(); }
	:	^(For in_exp block)
	;

if_statement
	:	^(If ^(Condition con=exp) block else_statement?) { semantics->CheckConditional( $con.start ); }
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
	:	brk=Break { semantics->CheckBreakContinue( $brk ); }
	;

continue_statement 
	:	con=Continue { semantics->CheckBreakContinue( $con ); }
	;

return_statement 
	:	^(Return exp)
	|	Return
	;

assign_statement
	: 	^(Const id=ID value) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_constant ); }
	|	(^(Local ID new_exp))=> ^(Local id=ID new_exp) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_local ); }
	|	^(Local id=ID exp) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_local ); }
	|	(^(Extern ID new_exp))=> ^(Extern id=ID new_exp) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_external ); }
	|	^(Extern id=ID exp?) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_external ); }
	|	(^('=' exp new_exp))=> ^('=' lhs=exp new_exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^('=' lhs=exp (exp|assign_rhs)) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(ADD_EQ_OP lhs=exp exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(SUB_EQ_OP lhs=exp exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(MUL_EQ_OP lhs=exp exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(DIV_EQ_OP lhs=exp exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(MOD_EQ_OP lhs=exp exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	exp
	;

assign_rhs 
	:	^('=' lhs=exp (assign_rhs|exp)) { semantics->CheckLhsForAssign( $lhs.start ); }
	;
	
new_exp
	:	^(New exp)
	;
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp
	:	^(GT_EQ_OP lhs=exp rhs=exp) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(LT_EQ_OP lhs=exp rhs=exp) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(GT_OP lhs=exp rhs=exp) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(LT_OP lhs=exp rhs=exp) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(EQ_OP lhs=exp rhs=exp) { semantics->CheckEqualityOp( $lhs.start, $rhs.start ); }
	|	^(NOT_EQ_OP lhs=exp rhs=exp) { semantics->CheckEqualityOp( $lhs.start, $rhs.start ); }
	|	^(AND_OP lhs=exp rhs=exp) { semantics->CheckLogicalOp( $lhs.start, $rhs.start ); }
	|	^(OR_OP lhs=exp rhs=exp) { semantics->CheckLogicalOp( $lhs.start, $rhs.start ); }
	|	^(ADD_OP lhs=exp rhs=exp) { semantics->CheckAddOp( $lhs.start, $rhs.start ); }
	|	^(SUB_OP lhs=exp rhs=exp) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(MUL_OP lhs=exp rhs=exp) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(DIV_OP lhs=exp rhs=exp) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(MOD_OP lhs=exp rhs=exp) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(Negate in=exp) { semantics->CheckNegateOp( $in.start ); }
	|	^(NOT_OP in=exp) { semantics->CheckNotOp( $in.start ); }
	|	^(Key exp key_exp)
	|	^(DOT_OP exp ID)
	|	call_exp
	|	(map_op | vec_op)
	|	value
	|	ID  { if( semantics->making_call ) semantics->ResolveFun( (char*)$ID.text->chars, $ID->getLine($ID) ); else semantics->ResolveVar( (char*)$ID.text->chars, $ID->getLine($ID) ); }
	;

call_exp
@init { semantics->making_call = true; }
@after { semantics->making_call = false; }
	:	^(Call exp exp*)
	;

key_exp
	:	(idx idx idx)=> idx1=idx idx2=idx idx3=exp { semantics->CheckKeyExp( $idx1.start, $idx2.start, $idx3.start ); }
	|	(idx idx)=> idx1=idx idx2=idx { semantics->CheckKeyExp( $idx1.start, $idx2.start ); }
	|	idx1=idx { semantics->CheckKeyExp( $idx1.start ); }
	;

idx 
	:	(END_OP | exp)
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg id=ID default_arg_val?) { semantics->AddArg( (char*)$id.text->chars, $id->getLine($id) ); }
	;

in_exp 
	:	^(In key=ID val=ID? exp) { semantics->DefineVar( (char*)$key.text->chars, $key->getLine($key) ); if( $val ) semantics->DefineVar( (char*)$val.text->chars, $val->getLine($val) ); }
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
		{ semantics->DefineVar( (char*)$nm->getText($nm)->chars, $nm->getLine($nm) ); }
	;

value
	:	BOOL | NULLVAL | NUMBER | STRING
	;

default_arg_val
	:	value | ID
	|	^(Negate (value | ID))
	;

