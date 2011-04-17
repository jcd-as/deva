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
	// passing -1 as a pointer is a bit of a hack, but we need a special marker
	// value besides NULL (which is already used) for CallOp()
	|	statement[(pANTLR3_BASE_TREE)-1]
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
@init { if( PSRSTATE->backtracking == 0 ){ compiler->fcn_nesting++; compiler->in_for_loop.push_back(0); compiler->in_while_loop.push_back(0); } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->fcn_nesting--; compiler->in_constructor = false; compiler->LeaveScope(); compiler->EndFun(); compiler->in_for_loop.pop_back(); compiler->in_while_loop.pop_back(); } }
	:	^(Def id=ID 
		{ compiler->AddScope(); compiler->DefineFun( (char*)$id.text->chars, classname, $ID->getLine($ID) ); }
		arg_list_decl block) 
	|	^(Def id='new' 
		{ compiler->AddScope(); compiler->in_constructor = true; compiler->DefineFun( const_cast<char*>("new"), classname, $Def->getLine($Def) ); }
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
	:	id=ID { compiler->Identifier( (char*)$id.text->chars, false, $id->getLine($id) ); }
	;

while_statement 
@init { if( PSRSTATE->backtracking == 0 ){ compiler->IncWhileLoopCounter(); } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->DecWhileLoopCounter(); } }
	:	^(While { compiler->WhileOpStart(); }
			^(Condition con=exp[false,NULL]) { compiler->WhileOpConditionJump(); }
			block { compiler->WhileOpEnd(); }
		)
	;

for_statement 
@init { if( PSRSTATE->backtracking == 0 ){ compiler->IncForLoopCounter(); compiler->AddScope(); } }
@after { if( PSRSTATE->backtracking == 0 ){ compiler->DecForLoopCounter(); compiler->LeaveScope(); } }
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
	:	^(Import (ID '/')* ID) { compiler->ImportOp( $Import, $Import->getLine($Import) ); }
	;
	
jump_statement 
	:	break_statement
	|	continue_statement
	|	return_statement
	;

break_statement 
	:	brk=Break { compiler->BreakOp( $brk->getLine($brk) ); }
	;

continue_statement 
	:	con=Continue { compiler->ContinueOp( $con->getLine($con) ); }
	;

return_statement 
	:	^(Return exp[false,NULL]) { compiler->ReturnOp( $Return->getLine($Return) ); }
	|	Return { compiler->ReturnOp( true ); }
	;

assign_statement[pANTLR3_BASE_TREE parent]
	: 	^(Const lhs=exp[true,NULL] value) { compiler->LocalVar( (char*)$lhs.text->chars, $Const->getLine($Const) ); }
	|	(^(Local exp[true,NULL] new_exp))=> ^(Local lhs=exp[true,NULL] new_exp) { compiler->LocalVar( (char*)$lhs.text->chars, $Local->getLine($Local) ); }
	|	(^(Local lhs=exp[true,NULL] exp[false,NULL]))=>^(Local lhs=exp[true,NULL] exp[false,NULL]) { compiler->LocalVar( (char*)$lhs.text->chars, $Local->getLine($Local) ); }
	|	^(Local lhs=exp[true,NULL]) { compiler->LocalVar( (char*)$lhs.text->chars, $Local->getLine($Local), true ); }
	|	(^(Extern exp[true,NULL] new_exp))=> ^(Extern lhs=exp[true,NULL] new_exp) { compiler->ExternVar( (char*)$lhs.text->chars, true, $Extern->getLine($Extern) ); }
	|	(^(Extern lhs=exp[true,NULL] exp[false,NULL]))=> ^(Extern lhs=exp[true,NULL] exp[false,NULL]) { compiler->ExternVar( (char*)$lhs.text->chars, true, $Extern->getLine($Extern) ); }
	|	^(Extern lhs=exp[true,NULL]) { compiler->ExternVar( (char*)$lhs.text->chars, false, $Extern->getLine($Extern) ); }
	|	(^(ASSIGN_OP exp[true,NULL] new_exp))=> ^(ASSIGN_OP lhs=exp[true,NULL] new_exp) { compiler->Assign( $lhs.start, false, $ASSIGN_OP->getLine($ASSIGN_OP) ); }
	|	(^(ASSIGN_OP lhs=exp[true,NULL] assign_rhs)) => ^(ASSIGN_OP lhs=exp[true,NULL] assign_rhs) { compiler->Assign( $lhs.start, false, $ASSIGN_OP->getLine($ASSIGN_OP) ); }
	|	^(ASSIGN_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->Assign( $lhs.start, false, $ASSIGN_OP->getLine($ASSIGN_OP) ); }
	|	^(ADD_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_add, $ADD_EQ_OP->getLine($ADD_EQ_OP) ); }
	|	^(SUB_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_sub, $SUB_EQ_OP->getLine($SUB_EQ_OP) ); }
	|	^(MUL_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_mul, $MUL_EQ_OP->getLine($MUL_EQ_OP) ); }
	|	^(DIV_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_div, $DIV_EQ_OP->getLine($DIV_EQ_OP) ); }
	|	^(MOD_EQ_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->AugmentedAssignOp( $lhs.start, op_mod, $MOD_EQ_OP->getLine($MOD_EQ_OP) ); }
	|	^(IncStat ID) { compiler->IncOp( $ID, false, $IncStat->getLine($IncStat) ); }
	|	^(DecStat ID) { compiler->DecOp( $ID, false, $DecStat->getLine($DecStat) ); }
	|	exp[false,parent]
	;

assign_rhs
	:	(^(ASSIGN_OP lhs=exp[true,NULL] assign_rhs)) => ^(ASSIGN_OP lhs=exp[true,NULL] assign_rhs) { compiler->Assign( $lhs.start, true, $ASSIGN_OP->getLine($ASSIGN_OP) ); }
	|	^(ASSIGN_OP lhs=exp[true,NULL] exp[false,NULL]) { compiler->Assign( $lhs.start, true, $ASSIGN_OP->getLine($ASSIGN_OP) ); }
	;
	
new_exp
	:	^(New { compiler->NewOp( $New->getLine($New) ); }
			exp[false,NULL]
		)
	;
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp[bool is_lhs_of_assign, pANTLR3_BASE_TREE parent]
	:	^(GT_EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->GtEqOp( $GT_EQ_OP->getLine($GT_EQ_OP) ); }
	|	^(LT_EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->LtEqOp( $LT_EQ_OP->getLine($LT_EQ_OP) ); }
	|	^(GT_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->GtOp( $GT_OP->getLine($GT_OP) ); }
	|	^(LT_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->LtOp( $LT_OP->getLine($LT_OP) ); }
	|	^(EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->EqOp( $EQ_OP->getLine($EQ_OP) ); }
	|	^(NOT_EQ_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->NotEqOp( $NOT_EQ_OP->getLine($NOT_EQ_OP) ); }
	|	^(AND_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->AndOp( $AND_OP->getLine($AND_OP) ); }
	|	^(
			ANDF_OP
			lhs=exp[true,parent]	{ compiler->AndFOpConditionJump(); }
			rhs=exp[true,parent]	{ compiler->AndFOpConditionJump(); }
		) 							{ compiler->AndFOp( $ANDF_OP->getLine($ANDF_OP) ); }
	|	^(
			ORF_OP
			lhs=exp[true,parent]	{ compiler->OrFOpConditionJump(true); }
			rhs=exp[true,parent]	{ compiler->OrFOpConditionJump(false); }
		) 							{ compiler->OrFOp( $ORF_OP->getLine($ORF_OP) ); }
	|	^(OR_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->OrOp( $OR_OP->getLine($OR_OP) ); }
	|	^(ADD_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->AddOp( $ADD_OP->getLine($ADD_OP) ); }
	|	^(SUB_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->SubOp( $SUB_OP->getLine($SUB_OP) ); }
	|	^(MUL_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->MulOp( $MUL_OP->getLine($MUL_OP) ); }
	|	^(DIV_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->DivOp( $DIV_OP->getLine($DIV_OP) ); }
	|	^(MOD_OP lhs=exp[false,parent] rhs=exp[false,parent]) { compiler->ModOp( $MOD_OP->getLine($MOD_OP) ); }
	|	^(Negate in=exp[false,parent]) { compiler->NegateOp( $in.start, $Negate->getLine($Negate)  ); }
	|	^(NOT_OP in=exp[false,parent]) { compiler->NotOp( $in.start, $NOT_OP->getLine($NOT_OP) ); }
	|	^(IncExp ID) { compiler->IncOp( $ID, true, $IncExp->getLine($IncExp) ); }
	|	^(DecExp ID) { compiler->DecOp( $ID, true, $DecExp->getLine($DecExp) ); }
	|	^(Key exp[false,NULL] key=key_exp[is_lhs_of_assign, parent])
	|	dot_exp[is_lhs_of_assign,$parent]
	|	call_exp[$parent]
	|	(map_op | vec_op)
	|	value
	|	ID { compiler->Identifier( (char*)$ID.text->chars, is_lhs_of_assign, $ID->getLine($ID) ); }
	;

dot_exp[bool is_lhs_of_assign, pANTLR3_BASE_TREE parent]
@after { compiler->is_dot_rhs = false; }
	:	^(
			DOT_OP 
			exp[false,NULL] {compiler->is_dot_rhs=true;} 
			rhs=exp[false,$parent]
		) { compiler->DotOp( is_lhs_of_assign, $rhs.start, parent, $DOT_OP->getLine($DOT_OP) ); }
	;

call_exp[pANTLR3_BASE_TREE parent]
@init { compiler->is_method = false; }
	:	^(Call args id=exp[false,$call_exp.start]) { compiler->CallOp( $id.start, $args.start, parent, $id.start->getLine($id.start) ); }
	;

args
	:	^(ArgList exp[false,NULL]*)
	;

key_exp[bool is_lhs_of_assign, pANTLR3_BASE_TREE parent]
	:	(idx idx idx)=> idx1=idx idx2=idx idx3=exp[false,NULL] { compiler->KeyOp( is_lhs_of_assign, 3, parent ); }
	|	(idx idx)=> idx1=idx idx2=idx { compiler->KeyOp( is_lhs_of_assign, 2, parent ); }
	|	idx1=idx { compiler->KeyOp( is_lhs_of_assign, 1, parent ); }
	;

idx 
	:	(END_OP { compiler->EndOp(); }
	|	exp[false,NULL])
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg id=ID default_arg_val?)
	;

in_exp 
//	:	^(In key=ID val=ID? exp[false,NULL]) { if( PSRSTATE->backtracking == 0 ){ compiler->InOp( $key $ ); }
	:	(^(In key=ID exp[false,NULL]))=> ^(In key=ID exp[false,NULL]) { if( PSRSTATE->backtracking == 0 ){ compiler->InOp( (char*)$key.text->chars, $exp.start, $key->getLine($key) ); } }
	|	^(In key=ID val=ID exp[false,NULL]) { if( PSRSTATE->backtracking == 0 ){ compiler->InOp( (char*)$key.text->chars, (char*)$val.text->chars, $exp.start, $key->getLine($key) ); } }
	;

map_op 
	:	^(Map_init map_item*) { compiler->MapOp( $Map_init, $Map_init->getLine($Map_init) ); }
	;

map_item 
	:	^(Pair exp[false,NULL] exp[false,NULL])
	;

vec_op 
	:	^(Vec_init exp[false,NULL]*) { compiler->VecOp( $Vec_init, $Vec_init->getLine($Vec_init) ); }
	;

value
	:	BOOL { if( strcmp( (char*)$BOOL.text->chars, "true" ) == 0 ) compiler->Bool( true, $BOOL->getLine($BOOL) ); else compiler->Bool( false, $BOOL->getLine($BOOL) ); }
	|	NULLVAL { compiler->Null( $NULLVAL->getLine($NULLVAL) ); }
	|	NUMBER { compiler->Number( $NUMBER, $NUMBER->getLine($NUMBER) ); }
	|	STRING { compiler->String( (char*)$STRING.text->chars, $STRING->getLine($STRING) ); }
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

