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

// compile.h
// compilation global object/functions for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __COMPILE_H__
#define __COMPILE_H__


#include "symbol.h"
#include "scope.h"
#include "error.h"
#include "opcodes.h"
#include "object.h"
#include "linemap.h"
#include "semantics.h"
#include "executor.h"

#include <vector>
#include <cstdlib>

using namespace std;

namespace deva_compile
{

class InstructionStream
{
	size_t size;
	byte* bytes;
	byte* cur;

public:
	InstructionStream( size_t start_sz = 1024 )
	{
		size = start_sz;
		bytes = new byte[size];
		cur = bytes;
		memset( bytes, 0, size );
	}
	~InstructionStream(){ /*delete[] bytes;*/ /*don't delete the bytes, the executor will*/ }
	const byte* Bytes(){ return bytes; }
	const byte* Current(){ return cur; }
	inline size_t Length(){ return cur - bytes; }
	void Append( byte b )
	{
		if( (Length() + sizeof(byte)) > size )
			Realloc();
		*cur++ = b;
	}
	void Append( word w )
	{
		if( (Length() + sizeof(word)) > size )
			Realloc();
		*((word*)cur) = w;
		cur += sizeof(word);
	}
	void Append( dword dw )
	{
		if( (Length() + sizeof(dword)) > size )
			Realloc();
		*((dword*)cur) = dw;
		cur += sizeof(dword);
	}
	// random access, for back-patching
	inline void Set( size_t loc, dword val )
	{
		*((dword*)(bytes + loc)) = val;
	}
private:
	void Realloc()
	{
		// out of space, double the size
		cur = (byte*)(cur - bytes);
		size_t new_sz = size * 2;
		byte* new_bytes = new byte[new_sz];
		memset( new_bytes, 0, new_sz );
		memcpy( new_bytes, bytes, (size_t)cur );
		delete[] bytes;
		bytes = new_bytes;
		size = new_sz;
		cur = (byte*)((size_t)cur + (size_t)bytes);
	}
};


class Compiler
{
private:
	// code module object for this compile session
	Code* code;

	// if we're compiling an eval or text from an interactive session, 
	// we may have undeclared symbols
	bool interactive;

	// compile with debug (line number) statements?
	bool emit_debug_info;

	// scopes & scope handling
	int max_scope_idx;		// index to the highest scope num created so far
	vector<int> scopestack;
	// stacks for labels/back-patching
	vector<size_t> patchstack;
	vector<size_t> labelstack;

	// have new/delete methods been added to the current class?
	bool has_new;
	bool has_delete;

	// name of the module we're compiling
	const char* module_name;

public:
	int num_locals;	// number of locals in the current scope

	// functions
	int fcn_nesting;
	bool is_method;

	// classes
	bool in_class;

	bool in_constructor;

	// dot op
	bool is_dot_rhs;

	// are we in a loop? 
	// vector of 'in-loop' counters, one for each function scope
	// if the back item is non-zero then we are inside a loop construct
	vector<int> in_for_loop;
	vector<int> in_while_loop;

	// instruction stream
	InstructionStream* is;

	// private helper functions
	/////////////////////////////////////////////////////////////////////////
private:
	bool InForLoop() { return in_for_loop.back() > 0; }
	bool InWhileLoop() { return in_while_loop.back() > 0; }

	// find index of constant
	inline int GetConstant( const Object & o ){ int i = code->FindConstant( o ); if( i == INT_MIN ) return ex->FindGlobalConstant( o ); else return i; }

	// label and back-patching helpers
	inline void AddLabel() { labelstack.push_back( is->Length() ); }
	inline void AddPatchLoc() { patchstack.push_back( is->Length() - sizeof(dword) ); }
	inline void BackpatchToCur() { is->Set( patchstack.back(), (dword)is->Length() ); patchstack.pop_back(); }
	inline void BackpatchToLastLabel() { is->Set( patchstack.back(), (dword)labelstack.back() ); patchstack.pop_back(); labelstack.pop_back(); }

	// clean-up loop/break tracking helper
	void CleanupEndLoop();

public:
	/////////////////////////////////////////////////////////////////////////
	// functions for public consumption
	/////////////////////////////////////////////////////////////////////////
	Compiler( const char* mod_name, Semantics* sem, bool interactive = false );
	~Compiler() { delete is; }

	// get the Code block object for this compiled module. this is the
	// end-of-life for the Compiler object, its raison d'etre. once this is
	// called, the compiler object should not be used again
	Code* GetCode() { code->code = (byte*)is->Bytes(); code->len = is->Length(); return code; }


	/////////////////////////////////////////////////////////////////////////
	// functions for consumption by the ANTLR compile-walker tree parser only
	/////////////////////////////////////////////////////////////////////////
	inline void Emit( Opcode o ){ is->Append( (byte)o ); }
	inline void Emit( Opcode o, dword op ){ is->Append( (byte)o ); is->Append( op ); }
	inline void Emit( Opcode o, dword op1, dword op2 ){ is->Append( (byte)o ); is->Append( op1 ); is->Append( op2 ); }
	inline void Emit( Opcode o, dword op1, dword op2, dword op3 ){ is->Append( (byte)o ); is->Append( op1 ); is->Append( op2 ); is->Append( op3 ); }
	inline void Emit( Opcode o, dword op1, dword op2, dword op3, dword op4 ){ is->Append( (byte)o ); is->Append( op1 ); is->Append( op2 ); is->Append( op3 ); is->Append( op4 ); }

	// generate a line number
	inline void EmitLineNum( int line ) { if( emit_debug_info ) code->lines->Add( line, (dword)is->Length() ); }

	// mostly for debugging purposes
	void Decode();

	// scope tracking:
	inline void AddScope() { scopestack.push_back( max_scope_idx ); max_scope_idx++; }
	inline void LeaveScope() { scopestack.pop_back(); }
	inline Scope* CurrentScope() { return semantics->scopes[scopestack.back()]; }
	inline Scope* ParentScope() { if( scopestack.size() < 2 ) throw ICE( "Invalid scope stack: No parent scope." ); return semantics->scopes[scopestack[scopestack.size()-2]]; }

	/////////////////////////////////////////////////////////////////////////
	// node handling functions
	/////////////////////////////////////////////////////////////////////////

	// block
	void EnterBlock();
	void ExitBlock();

	// define a function
	void DefineFun( char* name, char* classname, int line );
	// define an anonymus function and put it on the stack
	void DefineLambda( int line );
	void EndFun();

	// define a class
	void DefineClass( char* name, int line );
	// create a new class object
	void CreateClass( char* name, int line, pANTLR3_BASE_TREE bases );

	// constants
	void Number( pANTLR3_BASE_TREE node, int line );
	void String( char* s, int line );
	void Bool( bool b, int line ) { EmitLineNum( line ); if( b ) Emit( op_push_true ); else Emit( op_push_false ); }
	void Null( int line ) { EmitLineNum( line ); Emit( op_push_null ); }

	// identifier
	void Identifier( char* s, bool is_lhs_of_assign, int line );

	// binary operators
	inline void AddOp( int line ) { EmitLineNum( line ); Emit( op_add ); }
	inline void SubOp( int line ) { EmitLineNum( line ); Emit( op_sub ); }
	inline void MulOp( int line ) { EmitLineNum( line ); Emit( op_mul ); }
	inline void DivOp( int line ) { EmitLineNum( line ); Emit( op_div ); }
	inline void ModOp( int line ) { EmitLineNum( line ); Emit( op_mod ); }
	inline void GtEqOp( int line ) { EmitLineNum( line ); Emit( op_gte ); }
	inline void LtEqOp( int line ) { EmitLineNum( line ); Emit( op_lte ); }
	inline void GtOp( int line ) { EmitLineNum( line ); Emit( op_gt ); }
	inline void LtOp( int line ) { EmitLineNum( line ); Emit( op_lt ); }
	inline void EqOp( int line ) { EmitLineNum( line ); Emit( op_eq ); }
	inline void NotEqOp( int line ) { EmitLineNum( line ); Emit( op_neq ); }

	inline void AndOp( int line ) { EmitLineNum( line ); Emit( op_and ); }
	inline void OrOp( int line ) { EmitLineNum( line ); Emit( op_or ); }

	void AndFOpConditionJump();
	void AndFOp( int line );
	void OrFOpConditionJump( bool first );
	void OrFOp( int line );

	// unary operators
	void NegateOp( pANTLR3_BASE_TREE node, int line );
	void NotOp( pANTLR3_BASE_TREE node, int line );

	// assignments and variable decls
	void LocalVar( char* n, int line, bool lacks_initializer = false );
	void ExternVar( char* n, bool is_assign, int line );
	void Assign( pANTLR3_BASE_TREE lhs_node, bool parent_is_assign, int line );

	// augmented assignment operators (+=, -=, *=, /=, %=)
	void AugmentedAssignOp(  pANTLR3_BASE_TREE lhs_node, Opcode op, int line );

	void IncOp( pANTLR3_BASE_TREE lhs_node, bool is_expression, int line );
	void DecOp( pANTLR3_BASE_TREE lhs_node, bool is_expression, int line );

	// function call
	void CallOp( pANTLR3_BASE_TREE fcn, pANTLR3_BASE_TREE args, pANTLR3_BASE_TREE parent, int line );

	// Key ('[]') op
	void KeyOp( bool is_lhs_of_assign, int num_children, pANTLR3_BASE_TREE parent );
	// '$' op in a slice or index
	void EndOp();

	// Dot ('.') op
	void DotOp( bool is_lhs_of_assign, pANTLR3_BASE_TREE rhs, pANTLR3_BASE_TREE parent, int line );

	// return, continue, break ops
	void ReturnOp( int line, bool no_val = false );
	void ContinueOp( int line );
	void BreakOp( int line );

	void ImportOp( pANTLR3_BASE_TREE node, int line );

	// 'new'
	void NewOp( int line );
	
	// vector and map creation ops
	void VecOp( pANTLR3_BASE_TREE node, int line );
	void MapOp( pANTLR3_BASE_TREE node, int line );

	// if/else statements
	void IfOpJump();
	void EndIfOpJump();
	void ElseOpJump();
	void ElseOpEndLabel();

	// loop tracking
	void IncForLoopCounter(){ int i = in_for_loop.back(); in_for_loop.pop_back(); i++; in_for_loop.push_back( i ); }
	void DecForLoopCounter(){ int i = in_for_loop.back(); in_for_loop.pop_back(); i--; in_for_loop.push_back( i ); }
	void IncWhileLoopCounter(){ int i = in_while_loop.back(); in_while_loop.pop_back(); i++; in_while_loop.push_back( i ); }
	void DecWhileLoopCounter(){ int i = in_while_loop.back(); in_while_loop.pop_back(); i--; in_while_loop.push_back( i ); }

	// while statement
	void WhileOpStart();
	void WhileOpConditionJump();
	void WhileOpEnd();

	// for statement
	void InOp( char* key, char* val, pANTLR3_BASE_TREE container, int line );
	void InOp( char* key, pANTLR3_BASE_TREE container, int line );
	void ForOpEnd();
};


// global compiler object
extern Compiler* compiler;

} // namespace deva_compile

#endif // __COMPILE_H__
