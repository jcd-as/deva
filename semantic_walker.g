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
using namespace deva_compile;
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
	|	func_decl[NULL]
	|	assign_statement { semantics->CheckForNoEffect( $assign_statement.start ); }
	;

block 
@init { semantics->PushScope(); }
@after { semantics->PopScope(); }
	:	^(Block statement*)
	;

func_decl[char* classname]
@init { semantics->in_for_loop.push_back( 0 ); semantics->in_while_loop.push_back( 0 ); }
@after { semantics->in_fcn--; semantics->PopScope(); semantics->in_for_loop.pop_back(); semantics->in_while_loop.pop_back(); }
	:	^(Def id=ID 
		{ semantics->in_fcn++; semantics->DefineFun( (char*)$id.text->chars, classname, $id->getLine($id) ); semantics->PushScope( (char*)$ID.text->chars, classname ); if( classname ) semantics->DefineVar( (char*)"self", $id->getLine($id), mod_local ); }
		arg_list_decl { semantics->CheckAndResetFun( $id->getLine($id) ); }
		block) 
	|	^(Def id='new' 
		{ semantics->in_fcn++; semantics->DefineFun( (char*)"new", classname, $id->getLine($id) ); semantics->PushScope( (char*)"new", classname ); if( classname ) semantics->DefineVar( (char*)"self", $id->getLine($id), mod_local ); }
		arg_list_decl { semantics->CheckAndResetFun( $id->getLine($id) ); }
		block)
	;
	
class_decl 
@init { semantics->in_class = true; }
@after { semantics->in_class = false; }
	:	^(Class id=ID 
		{ semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_local ); semantics->constants.insert( Object( (char*)$id.text->chars ) ); }
		(^(Base_classes ID*)) func_decl[(char*)$id.text->chars]*)
	;

while_statement 
@init { semantics->IncWhileLoopCounter(); }
@after { semantics->DecWhileLoopCounter(); }
	:	^(While ^(Condition con=exp[false]) block) { semantics->CheckConditional( $con.start ); }
	;

for_statement 
@init { semantics->IncForLoopCounter(); semantics->PushScope(); }
@after { semantics->DecForLoopCounter(); semantics->PopScope(); }
	:	^(For in_exp block)
	;

if_statement
	:	^(If ^(Condition con=exp[false]) block else_statement?) { semantics->CheckConditional( $con.start ); }
	;

else_statement 
	:	^(Else block)
	;

import_statement 
	:	^(Import (ID '/')* ID) { semantics->CheckImport( $Import ); }
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
	:	^(Return exp[false]) { semantics->CheckReturn( $Return ); }
	|	Return { semantics->CheckReturn( $Return ); }
	;

assign_statement
	: 	^(Const id=ID value[false]) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_constant ); }
	|	(^(Local ID new_exp))=> ^(Local id=ID new_exp) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_local ); }
	|	^(Local id=ID exp[false]) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_local ); }
	|	(^(Extern ID new_exp))=> ^(Extern id=ID new_exp) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_external ); }
	|	^(Extern id=ID exp[false]?) { semantics->DefineVar( (char*)$id.text->chars, $id->getLine($id), mod_external ); }
	|	(^(ASSIGN_OP exp[false] new_exp))=> ^(ASSIGN_OP lhs=exp[false] new_exp) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(ASSIGN_OP lhs=exp[false] (exp[false]|assign_rhs)) { semantics->CheckLhsForAssign( $lhs.start ); }
	|	^(ADD_EQ_OP lhs=exp[false] exp[false]) { semantics->CheckLhsForAugmentedAssign( $lhs.start ); }
	|	^(SUB_EQ_OP lhs=exp[false] exp[false]) { semantics->CheckLhsForAugmentedAssign( $lhs.start ); }
	|	^(MUL_EQ_OP lhs=exp[false] exp[false]) { semantics->CheckLhsForAugmentedAssign( $lhs.start ); }
	|	^(DIV_EQ_OP lhs=exp[false] exp[false]) { semantics->CheckLhsForAugmentedAssign( $lhs.start ); }
	|	^(MOD_EQ_OP lhs=exp[false] exp[false]) { semantics->CheckLhsForAugmentedAssign( $lhs.start ); }
	|	exp[false]
	;

assign_rhs 
	:	^(ASSIGN_OP lhs=exp[false] (assign_rhs|exp[false])) { semantics->CheckLhsForAssign( $lhs.start ); }
	;
	
new_exp
	:	^(New exp[false])
	;
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp[bool invert]
	:	^(GT_EQ_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(LT_EQ_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(GT_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(LT_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckRelationalOp( $lhs.start, $rhs.start ); }
	|	^(EQ_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckEqualityOp( $lhs.start, $rhs.start ); }
	|	^(NOT_EQ_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckEqualityOp( $lhs.start, $rhs.start ); }
	|	^(AND_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckLogicalOp( $lhs.start, $rhs.start ); }
	|	^(OR_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckLogicalOp( $lhs.start, $rhs.start ); }
	|	^(ANDF_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckLogicalOp( $lhs.start, $rhs.start ); }
	|	^(ORF_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckLogicalOp( $lhs.start, $rhs.start ); }
	|	^(ADD_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckAddOp( $lhs.start, $rhs.start ); }
	|	^(SUB_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(MUL_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(DIV_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(MOD_OP lhs=exp[false] rhs=exp[false]) { semantics->CheckMathOp( $lhs.start, $rhs.start ); }
	|	^(Negate in=exp[true]) { semantics->CheckNegateOp( $in.start ); }
	|	^(NOT_OP in=exp[false]) { semantics->CheckNotOp( $in.start ); }
	|	^(Key exp[false] key_exp)
	|	^(DOT_OP lhs=exp[false] 
			{ semantics->rhs_of_dot = true; }
			exp[false])
			{ semantics->rhs_of_dot = false; }
	|	call_exp
	|	(map_op | vec_op)
	|	value[invert]
// TODO: this calls ResolveFun on every ID in, for example, 'a.b.c()'
	|	ID  { if( semantics->making_call ) semantics->ResolveFun( (char*)$ID.text->chars, $ID->getLine($ID) ); else semantics->ResolveVar( (char*)$ID.text->chars, $ID->getLine($ID) ); }
	;

call_exp
	:	^(Call 
			^(ArgList exp[false]*) { semantics->making_call = true; }
			exp[false]
		) { semantics->making_call = false; }
	;

key_exp
@init { semantics->in_map_key = true; }
@after { semantics->in_map_key = false; }
	:	(idx idx idx)=> idx1=idx idx2=idx idx3=exp[false] { semantics->CheckKeyExp( $idx1.start, $idx2.start, $idx3.start ); }
	|	(idx idx)=> idx1=idx idx2=idx { semantics->CheckKeyExp( $idx1.start, $idx2.start ); }
	|	idx1=idx { semantics->CheckKeyExp( $idx1.start ); }
	;

idx 
	:	(END_OP | exp[false])
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(
			Def_arg id=ID { semantics->AddArg( (char*)$id.text->chars, $id->getLine($id) ); }
			(
				default_arg_val
				| // or nothing
			)
		)
	;

in_exp 
	:	^(In key=ID val=ID? exp[false]) { semantics->DefineVar( (char*)$key.text->chars, $key->getLine($key), mod_local ); if( $val ) semantics->DefineVar( (char*)$val.text->chars, $val->getLine($val), mod_local ); }
	;

map_op 
	:	^(Map_init map_item*)
	;

map_item 
	:	^(Pair map_key[false] exp[false])
	;

map_key[bool invert]
@init { semantics->in_map_key = true; }
@after { semantics->in_map_key = false; }
	:	(exp[invert])
	;

vec_op 
	:	^(Vec_init exp[false]*)
	;

value[bool invert]
	:	BOOL | NULLVAL 
	|	NUMBER { semantics->AddNumber( (invert ? -1.0 : 1.0) * parse_number( (char*)$NUMBER.text->chars ) ); $NUMBER->u = (void*)0x0; }
		
	|	STRING { semantics->AddString( (char*)$STRING.text->chars ); }
	;

default_arg_val
	:	BOOL { semantics->DefaultArgVal( $BOOL ); }
	|	NULLVAL { semantics->DefaultArgVal( $NULLVAL ); }
	|	NUMBER { semantics->AddNumber( parse_number( (char*)$NUMBER.text->chars ) ); semantics->DefaultArgVal( $NUMBER ); }
	|	STRING { semantics->AddString( (char*)$STRING.text->chars ); semantics->DefaultArgVal( $STRING ); } 
	|	ID { semantics->CheckDefaultArgVal( (char*)$ID.text->chars, $ID->getLine($ID) ); }
	|	^(Negate NUMBER) { semantics->AddNumber( parse_number( (char*)$NUMBER.text->chars ) * -1.0 ); semantics->DefaultArgVal( $NUMBER, true );}
	;

