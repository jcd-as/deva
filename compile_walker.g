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
	|	func_decl[NULL]
	|	assign_statement
	;

block 
@init { compiler->EnterBlock(); }
@after { compiler->ExitBlock(); }
	:	^(Block statement*)
	;

func_decl[char* classname]
@init { compiler->fcn_nesting++; }
@after { compiler->fcn_nesting--; compiler->LeaveScope(); }
	:	^(Def id=ID 
		{ compiler->AddScope(); compiler->DefineFun( (char*)$id.text->chars, classname, $id->getLine($id) ); }
		arg_list_decl block) 
	|	^(Def id='new' 
		{ compiler->AddScope(); compiler->DefineFun( const_cast<char*>("new"), classname, $id->getLine($id) ); }
		arg_list_decl block)
	;
	
class_decl 
@init { compiler->in_class = true; }
@after { compiler->in_class = false; }
	:	^(Class id=ID 
		{ compiler->DefineClass( (char*)$id.text->chars, $id->getLine($id) ); }
		(^(Base_classes ID+))? func_decl[(char*)$id.text->chars]*)
	;

while_statement 
	:	^(While ^(Condition con=exp[false]) block) // TODO
	;

for_statement 
@init { compiler->AddScope(); }
@after { compiler->LeaveScope(); }
	:	^(For in_exp block) // TODO
	;

if_statement
	:	^(If ^(Condition con=exp[false]) block else_statement?) // TODO
	;

else_statement 
	:	^(Else block) // TODO
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
	:	brk=Break // TODO
	;

continue_statement 
	:	con=Continue // TODO
	;

return_statement 
	:	^(Return exp[false]) { compiler->ReturnOp(); }
	|	Return { compiler->ReturnOp( true ); }
	;

assign_statement
	: 	^(Const lhs=exp[true] value) { compiler->LocalVar( (char*)$lhs.text->chars ); }
	|	(^(Local exp[true] new_exp))=> ^(Local lhs=exp[true] new_exp) { compiler->LocalVar( (char*)$lhs.text->chars ); }
	|	^(Local lhs=exp[true] exp[false]) { compiler->LocalVar( (char*)$lhs.text->chars ); }
	|	(^(Extern exp[true] new_exp))=> ^(Extern lhs=exp[true] new_exp) { compiler->ExternVar( (char*)$lhs.text->chars, true ); }
	|	(^(Extern lhs=exp[true] exp[false]))=> ^(Extern lhs=exp[true] exp[false]) { compiler->ExternVar( (char*)$lhs.text->chars, true ); }
	|	^(Extern lhs=exp[true]) { compiler->ExternVar( (char*)$lhs.text->chars, false ); }
	|	(^('=' exp[true] new_exp))=> ^('=' lhs=exp[true] new_exp) { compiler->Assign( $lhs.start ); }
	|	^('=' lhs=exp[true] (exp[false]|assign_rhs)) { compiler->Assign( $lhs.start ); }
	|	^(ADD_EQ_OP lhs=exp[true] exp[false]) { compiler->AugmentedAssignOp( $lhs.start, op_add ); }
	|	^(SUB_EQ_OP lhs=exp[true] exp[false]) { compiler->AugmentedAssignOp( $lhs.start, op_sub ); }
	|	^(MUL_EQ_OP lhs=exp[true] exp[false]) { compiler->AugmentedAssignOp( $lhs.start, op_mul ); }
	|	^(DIV_EQ_OP lhs=exp[true] exp[false]) { compiler->AugmentedAssignOp( $lhs.start, op_div ); }
	|	^(MOD_EQ_OP lhs=exp[true] exp[false]) { compiler->AugmentedAssignOp( $lhs.start, op_mod ); }
	|	exp[false]
	;

assign_rhs 
	:	^('=' lhs=exp[true] (assign_rhs|exp[false]))
	;
	
new_exp
	:	^(New { compiler->NewOp(); }
			exp[false]
		)
	;
	
/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
/////////////////////////////////////////////////////////////////////////////

exp[bool is_lhs_of_assign]
	:	^(GT_EQ_OP lhs=exp[false] rhs=exp[false]) { compiler->GtEqOp(); }
	|	^(LT_EQ_OP lhs=exp[false] rhs=exp[false]) { compiler->LtEqOp(); }
	|	^(GT_OP lhs=exp[false] rhs=exp[false]) { compiler->GtOp(); }
	|	^(LT_OP lhs=exp[false] rhs=exp[false]) { compiler->LtOp(); }
	|	^(EQ_OP lhs=exp[false] rhs=exp[false]) { compiler->EqOp(); }
	|	^(NOT_EQ_OP lhs=exp[false] rhs=exp[false]) { compiler->NotEqOp(); }
	|	^(AND_OP lhs=exp[false] rhs=exp[false]) { compiler->AndOp(); }
	|	^(OR_OP lhs=exp[false] rhs=exp[false]) { compiler->OrOp(); }
	|	^(ADD_OP lhs=exp[false] rhs=exp[false]) { compiler->AddOp(); }
	|	^(SUB_OP lhs=exp[false] rhs=exp[false]) { compiler->SubOp(); }
	|	^(MUL_OP lhs=exp[false] rhs=exp[false]) { compiler->MulOp(); }
	|	^(DIV_OP lhs=exp[false] rhs=exp[false]) { compiler->DivOp(); }
	|	^(MOD_OP lhs=exp[false] rhs=exp[false]) { compiler->ModOp(); }
	|	^(Negate in=exp[false]) { compiler->NegateOp(); }
	|	^(NOT_OP in=exp[false]) { compiler->NotOp(); }
	|	^(Key exp[false] key=key_exp) { compiler->KeyOp( $key.start, is_lhs_of_assign ); } // TODO: slices??
	|	^(DOT_OP exp[false] exp[false]) { compiler->KeyOp( NULL, is_lhs_of_assign ); }
	|	call_exp
	|	(map_op | vec_op)
	|	value
	|	ID { compiler->Identifier( (char*)$ID.text->chars, is_lhs_of_assign ); }
	;

call_exp
	:	^(Call id=exp[false] args) { compiler->CallOp( $id.start, $args.start ); }
	;

args
	:	^(ArgList exp[false]*)
	;

key_exp
	:	(idx idx idx)=> idx1=idx idx2=idx idx3=exp[false]
	|	(idx idx)=> idx1=idx idx2=idx
	|	idx1=idx
	;

idx 
	:	(END_OP | exp[false])
	;

arg_list_decl
	:	^(Arg_list_decl arg*)
	;

arg 
	:	^(Def_arg id=ID default_arg_val?)
	;

in_exp 
	:	^(In key=ID val=ID? exp[false])
	;

map_op 
	:	^(Map_init map_item*)
	;

map_item 
	:	^(Pair exp[false] exp[false])
	;

vec_op 
	:	^(Vec_init exp[false]*)
	;

value
	:	BOOL { if( strcmp( (char*)$BOOL.text->chars, "true" ) == 0 ) compiler->Emit( op_push_true ); else compiler->Emit( op_push_false ); }
	|	NULLVAL { compiler->Emit( op_push_null ); }
	|	NUMBER { compiler->Number( atof( (char*)$NUMBER.text->chars ) ); }
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

