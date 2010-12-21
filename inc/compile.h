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
#include "executor.h"

#include <vector>
#include <cstdlib>

using namespace std;

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
	~InstructionStream(){ delete[] bytes; }
	const byte* Bytes(){ return bytes; }
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
private:
	void Realloc()
	{
		// out of space, double the size
		cur = (byte*)(cur - bytes);
		size_t new_sz = size * 2;
		byte* new_bytes = new byte[new_sz];
		memset( new_bytes, 0, new_sz );
		memcpy( new_bytes, bytes, new_sz );
		delete[] bytes;
		bytes = new_bytes;
		size = new_sz;
		cur = (byte*)((size_t)cur + (size_t)bytes);
	}
};

struct Compiler
{
	// scopes
private:
	int max_scope_idx;		// index to the highest scope num created so far
	vector<int> scopestack;

public:
	int num_locals;	// number of locals in the current scope

	// functions
	int fcn_nesting;

	// classes
	bool in_class;

	// instruction stream
	InstructionStream* is;


	// private helper functions
	/////////////////////////////////////////////////////////////////////////
private:
	// find index of constant
	int GetConstant( const DevaObject & o ) { return ex->FindConstant( o ); }

public:
	// public functions
	/////////////////////////////////////////////////////////////////////////
	Compiler( Semantics* sem, Executor* ex );
	~Compiler()
	{ delete is; }
	inline void Emit( Opcode o ){ is->Append( (byte)o ); }
	inline void Emit( Opcode o, dword op ){ is->Append( (byte)o ); is->Append( op ); }

	// mostly for debugging purposes
	void Decode();

	// scope tracking:
	inline void AddScope() { scopestack.push_back( max_scope_idx ); max_scope_idx++; }
	inline void LeaveScope() { scopestack.pop_back(); }
	inline Scope* CurrentScope() { return semantics->scopes[scopestack.back()]; }

	// node handling functions
	/////////////////////////////////////////////////////////////////////////

	// block
	void EnterBlock();
	void ExitBlock();

	// define a function
	void DefineFun( char* name, char* classname, int line );

	// define a class
	void DefineClass( char* name, int line );

	// constants
	void Number( double d );
	void String( char* s );

	// identifier
	void Identifier( char* s, bool is_lhs_of_assign );

	// operators
	inline void AddOp() { Emit( op_add ); }
	inline void SubOp() { Emit( op_sub ); }
	inline void MulOp() { Emit( op_mul ); }
	inline void DivOp() { Emit( op_div ); }
	inline void ModOp() { Emit( op_mod ); }
	inline void NegateOp() { Emit( op_neg ); }
	inline void NotOp() { Emit( op_not ); }
	inline void GtEqOp() { Emit( op_gte ); }
	inline void LtEqOp() { Emit( op_lte ); }
	inline void GtOp() { Emit( op_gt ); }
	inline void LtOp() { Emit( op_lt ); }
	inline void EqOp() { Emit( op_eq ); }
	inline void NotEqOp() { Emit( op_neg ); }
	inline void AndOp() { Emit( op_and ); }
	inline void OrOp() { Emit( op_or ); }

	// assignments and variable decls
	void LocalVar( char* n );
	void ExternVar( char* n, bool is_assign );
	void Assign( pANTLR3_BASE_TREE node );

	// augmented assignment operators (+=, -=, *=, /=, %=)
	void AugmentedAssignOp(  pANTLR3_BASE_TREE lhs_node, Opcode op );

	// function call
	void CallOp( pANTLR3_BASE_TREE fcn, pANTLR3_BASE_TREE args );

	// Key ('[]') and Dot ('.') ops
	void KeyOp( pANTLR3_BASE_TREE key_exp, bool is_lhs_of_assign );

	// return, continue, break ops
	void ReturnOp( bool no_val = false );
	// how to setup the jump and determine the right number of Leave ops to
	// generate??
//	void ContinueOp();
//	void BreakOp();

	void ImportOp( pANTLR3_BASE_TREE node );

	// 'new'
	void NewOp();
};


// global compiler object
extern Compiler* compiler;


#endif // __COMPILE_H__
