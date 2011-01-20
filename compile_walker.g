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
using namespace deva_compile;
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
	|	statement[$top_level_statement.start]
	;

statement[pANTLR3_BASE_TREE parent]
	:	block
	|	while_statement
	|	for_statement
	|	if_statement
	|	import_statement
	|	jump_statement
	|	func_decl[NULL]
	|	assign_statement[$parent]
	;

block 
@init { if( PSRSTATE->backtracking == 0 ){ compiler->EnterBlock(); } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->ExitBlock(); } }
	:	^(Block statement[$block.start]*)
	;

func_decl[char* classname]
@init { if( PSRSTATE->backtracking == 0 ){ compiler->fcn_nesting++; } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->fcn_nesting--; compiler->in_constructor = false; compiler->LeaveScope(); compiler->EndFun(); } }
	:	^(Def id=ID 
		{ compiler->AddScope(); compiler->DefineFun( (char*)$id.text->chars, classname, $id->getLine($id) ); }
		arg_list_decl block) 
	|	^(Def id='new' 
		{ compiler->AddScope(); compiler->in_constructor = true; compiler->DefineFun( const_cast<char*>("new"), classname, $id->getLine($id) ); }
		arg_list_decl block)
	;
	
class_decl 
@init { if( PSRSTATE->backtracking == 0 ){ compiler->in_class = true; } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->in_class = false; } }
	:	^(Class id=ID 
		(^(Base_classes base_class*))
		func_decl[(char*)$id.text->chars]*)
		{ compiler->DefineClass( (char*)$id.text->chars, $id->getLine($id), $Base_classes ); }
	;

base_class
	:	id=ID { compiler->Identifier( (char*)$id.text->chars, false ); }
	;

while_statement 
	:	^(While { compiler->WhileOpStart(); }
			^(Condition con=exp[false,NULL]) { compiler->WhileOpConditionJump(); }
			block { compiler->WhileOpEnd(); }
		)
	;

for_statement 
@init { if( PSRSTATE->backtracking == 0 ){ compiler->AddScope(); } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->LeaveScope(); } }
	:	^(For in_exp block) { if( PSRSTATE->backtracking == 0 ) compiler->ForOpEnd(); }
	;

if_statement
	:	(^(If ^(Condition con=exp[false,NULL]) block else_statement))=>
		^(If ^(Condition con=exp[false,NULL]) { compiler->IfOpJump(); } block { compiler->ElseOpJump(); } else_statement { compiler->ElseOpEndLabel(); } )
	|	^(If ^(Condition con=exp[false,NULL]) { compiler->IfOpJump(); } block { compiler->EndIfOpJump(); } )
	;

else_statement 
	:	^(Else block)
	;

import_statement 
	:	^(Import (ID '/')* ID) { compiler->ImportOp( $Import ); }
	;
	
jump_statement 
	:	break_statement
	|	continue_statement
	|	return_statement
	;

break_statement 
	:	brk=Break { compiler->BreakOp(); }
	;

continue_statement 
	:	con=Continue { compiler->ContinueOp(); }
	;

return_statement 
	:	^(Return exp[false,NULL]) { compiler->ReturnOp(); }
	|	Return { compiler->ReturnOp( true ); }
	;

assign_statement[pANTLR3_BASE_TREE parent]
	: 	^(Const lhs=exp[true,NULL] value) { compiler->LocalVar( (char*)$lhs.text->chars ); }
	|	(^(Local exp[true,NULL] new_exp))=> ^(Local lhs=exp[true,NULL] new_exp) { compiler->LocalVar( (char*)$lhs.text->chars ); }
	|	^(Local lhs=exp[true,NULL] exp[false,NULL]) { compiler->LocalVar( (char*)$lhs.text->chars ); }
	|	(^(Extern exp[true,NULL] new_exp))=> ^(Extern lhs=exp[true,NULL] new_exp) { compiler->ExternVar( (char*)$lhs.text->chars, true ); }
	|	(^(Extern lhs=exp[true,NULL] exp[false,NULL]))=> ^(Extern lhs=exp[true,NULL] exp[false,NULL]) { compiler->ExternVar( (char*)$lhs.text->chars, true ); }
	|	^(Extern lhs=exp[true,NULL]) { compiler->ExternVar( (char*)$lhs.text->chars, false ); }
	|	(^('=' exp[true,NULL] new_exp))=> ^('=' lhs=exp[true,NULL] new_exp) { compiler->Assign( $lhs.start ); }
	|	^('=' lhs=exp[true,NULL] (exp[false,NULL]|assign_rhs)) { compiler->Assign( $lhs.start ); }
	|	^(ADD_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_add ); }
	|	^(SUB_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_sub ); }
	|	^(MUL_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_mul ); }
	|	^(DIV_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_div ); }
	|	^(MOD_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_mod ); }
	|	exp[false,parent]
	;

assign_rhs 
	:	^('=' lhs=exp[true,NULL] (assign_rhs|exp[false,NULL]))
	;
	
new_exp
	:	^(New { compiler->NewOp(); }
			exp[false,NULL]
		)
	;
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp[bool is_lhs_of_assign, pANTLR3_BASE_TREE parent]
	:	^(GT_EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->GtEqOp(); }
	|	^(LT_EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->LtEqOp(); }
	|	^(GT_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->GtOp(); }
	|	^(LT_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->LtOp(); }
	|	^(EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->EqOp(); }
	|	^(NOT_EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->NotEqOp(); }
	|	^(AND_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->AndOp(); }
	|	^(OR_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->OrOp(); }
	|	^(ADD_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->AddOp(); }
	|	^(SUB_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->SubOp(); }
	|	^(MUL_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->MulOp(); }
	|	^(DIV_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->DivOp(); }
	|	^(MOD_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->ModOp(); }
	|	^(Negate in=exp[false,parent]) { compiler->NegateOp( $in.start ); }
	|	^(NOT_OP in=exp[false,parent]) { compiler->NotOp( $in.start ); }
	|	^(Key exp[false,NULL] key=key_exp) { compiler->KeyOp( $key.start, is_lhs_of_assign ); } // TODO: slices??
	|	dot_exp[is_lhs_of_assign]
	|	call_exp[$parent]
	|	(map_op | vec_op)
	|	value
	|	ID { compiler->Identifier( (char*)$ID.text->chars, is_lhs_of_assign ); }
	;

dot_exp[bool is_lhs_of_assign]
@after { compiler->is_dot_rhs = false; }
	:	^(
			DOT_OP 
			exp[false,NULL] {compiler->is_dot_rhs=true;} 
			exp[false,NULL]
		) { compiler->KeyOp( NULL, is_lhs_of_assign ); }
	;

call_exp[pANTLR3_BASE_TREE parent]
	:	^(Call args id=exp[false,NULL]) { compiler->CallOp( $id.start, $args.start, $parent ); }
	;

args
	:	^(ArgList exp[false,NULL]*)
	;

key_exp
	:	(idx idx idx)=> idx1=idx idx2=idx idx3=exp[false,NULL]
	|	(idx idx)=> idx1=idx idx2=idx
	|	idx1=idx
	;

idx 
	:	(END_OP | exp[false,NULL])
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg id=ID default_arg_val?)
	;

in_exp 
//	:	^(In key=ID val=ID? exp[false,NULL]) { if( PSRSTATE->backtracking == 0 ){ compiler->InOp( $key $ ); }
	:	(^(In key=ID exp[false,NULL]))=> ^(In key=ID exp[false,NULL]) { if( PSRSTATE->backtracking == 0 ){ compiler->InOp( (char*)$key.text->chars, $exp.start ); } }
	|	^(In key=ID val=ID exp[false,NULL]) { if( PSRSTATE->backtracking == 0 ){ compiler->InOp( (char*)$key.text->chars, (char*)$val.text->chars, $exp.start ); } }
	;

map_op 
	:	^(Map_init map_item*) { compiler->MapOp( $Map_init ); }
	;

map_item 
	:	^(Pair exp[false,NULL] exp[false,NULL])
	;

vec_op 
	:	^(Vec_init exp[false,NULL]*) { compiler->VecOp( $Vec_init ); }
	;

value
	:	BOOL { if( strcmp( (char*)$BOOL.text->chars, "true" ) == 0 ) compiler->Emit( op_push_true ); else compiler->Emit( op_push_false ); }
	|	NULLVAL { compiler->Emit( op_push_null ); }
	|	NUMBER { compiler->Number( $NUMBER ); }
	|	STRING { compiler->String( (char*)$STRING.text->chars ); }
	;

default_arg_val
	:	(
			BOOL { /*do nothing!*/ }
			| NULLVAL { /*do nothing!*/ }
			| NUMBER { /*do nothing!*/ }
			| STRING { /*do nothing!*/ }
		)
	|	ID { /*do nothing!*/ }
	|	^(Negate NUMBER) { /*do nothing!*/ }
	;

