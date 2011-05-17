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

// executor.h
// executive/VM global object/functions for the deva language
// created by jcs, december 14, 2010 

// TODO:
// * 

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__


#include "opcodes.h"
#include "object.h"
#include "ordered_set.h"
#include "exceptions.h"
#include "util.h"
#include "scopetable.h"
#include "frame.h"
#include "module.h"
#include "builtins.h"
#include "string_builtins.h"
#include "vector_builtins.h"
#include "map_builtins.h"
#include "code.h"

#include <vector>
#include <set>
#include <climits>

using namespace std;


namespace deva
{


// number of 'global' constant symbols (true, false, null, 'delete', 'new' etc)
extern const int num_of_constant_symbols;

// singleton class for deva VM execution engine
class Executor
{
	friend class ScopeTable;

	// static bool for singleton functionality
	static bool instantiated;

	// current code block
	Code* cur_code;
	// current instruction pointer (current code)
	byte* ip;
	// current base pointer (module base)
	byte* bp;
	// end of current code block
	byte* end;

	// list of code blocks (modules, eval'd blocks)
	vector<const Code*> code_blocks;

	// call stack
	vector<Frame*> callstack;

	// operand stack
	// TODO: a vector's [] op uses one deref and two adds to get to a value,
	// with a plain pointer/array we can get rid of one add.. worth it?
	vector<Object> stack;

	// 'global' scope table (namespace)
	ScopeTable* scopes;

	// the main module (top-level)
	Module* main_module;

	// available native modules
	map<string, Object> native_modules;

	// native modules that have been imported
	map<string, Object> imported_native_modules;
	
	// list of possible module names (from compilation)
	set<string> module_names;

	// modules that have been imported
	map<string, Object> modules;

	// load-ordered list (stack) of modules
	vector<Module*> module_stack;

	// set of function objects
	multimap<string, Object*> functions;

	// set of classes
	map<string, vector<Function*> > classes;

	// set of constants (including all names)
	vector<Object> constants;
	set<Object> constants_set;

	// error flag
	bool is_error;
	// error object
	Object error;

public:
	// flags:
	bool debug;
	bool trace;

public:
	Executor();
	~Executor();

	size_t GetOffsetForCallSite( Frame* f, byte* addr ) const;

	inline Scope* CurrentScope() { return scopes->CurrentScope(); }
	inline Scope* GlobalScope() { return scopes->At( 0 ); }
	inline Frame* CurrentFrame() { return callstack.back(); }
	inline Frame* MainFrame() { return callstack[0]; }

	inline void PushFrame( Frame* f ) { callstack.push_back( f ); }
	inline void PopFrame() { if( !callstack.back()->IsModule() ) delete callstack.back(); callstack.pop_back(); }

	inline void PushScope( Scope* s ) { scopes->PushScope( s ); }
	inline void PopScope() { scopes->PopScope(); }

	inline void PushStack( Object o ) { stack.push_back( o ); }
	inline Object PopStack() { Object o = stack.back(); stack.pop_back(); return o; }

	Object* FindSymbol( const char* const name, Module* mod = NULL );
	const char* FindSymbolName( Object* o, bool local_only = false ) { return scopes->FindSymbolName( o, local_only ); }
	inline Object* FindLocal( const char* name ) { return CurrentScope()->FindSymbol( name ); }

	// helpers
	inline void AddFunction( Function* f ) { functions.insert( pair<string, Object*>( f->name, new Object( f ) ) ); }
	inline void AddFunction( const char* name, Function* f ) { functions.insert( pair<string, Object*>( string( name ), new Object( f ) ) ); }
private:
	Object* FindFunction( string name, string modulename, size_t offset );

	Object* FindSymbolInGlobalModules( const char* const name );

	// find a symbol in all current scopes, fcns, builtins, modules etc
	Object* FindSymbolInAnyScope( Object sym );
	// return a resolved symbol (find the symbol if 'sym' is a obj_symbol_name)
	Object ResolveSymbol( Object sym );

	Module* AddModule( const char* name, const Code* c, Scope* s, Frame* f, bool global = false );
	Object* GetModule( const char* const name );
	const char* const GetModuleName( const char* const name );

public:

	inline void AddNativeModule( NativeModule* mod ) { native_modules.insert( make_pair( mod->name, Object( mod ) ) ); }
	inline Object* GetNativeModule( const char* name )
	{
		map<string, Object>::iterator i = imported_native_modules.find( string( name ) );
		if( i != imported_native_modules.end() )
			return &(i->second);
		else return NULL;
	}


	// constant pool handling methods
	inline bool AddGlobalConstant( Object o ) { if( constants_set.count( o ) != 0 ) return false; else { constants_set.insert( o ); constants.push_back( o ); return true; } }
	inline int FindGlobalConstant( const Object & o )
	{
		if( constants_set.count( o ) == 0 ) return INT_MIN;
		for( size_t i = 0; i < constants.size(); i++ )
			if( o == constants.at( i ) )
				return -(int)i;
		return INT_MIN;
	}
	inline Object GetGlobalConstant( int idx ) { int i = idx < 0 ? -idx : idx; return constants.at( i ); }
	inline Object GetGlobalConstant( Object o ) { return GetGlobalConstant( FindGlobalConstant( o ) ); }

	// look up in the current module and globals:
	inline int FindConstant( const Code* code, const Object & o )
	{
		int i = FindGlobalConstant( o );
		if( i != INT_MIN )
			return i;
		else
			return code->FindConstant( o );
	}
	inline int FindConstant( const Object & o ) { return FindConstant( cur_code, o ); }
	// look up a constant in the current module and/or globals:
	inline Object GetConstant( const Code* code, int idx )
	{
		if( idx < 0 )
			return constants.at( -idx );
		else
			return code->GetConstant( idx );
	}
	inline Object GetConstant( int idx ) { return GetConstant( cur_code, idx ); }
	inline Object GetConstant( const Code* code, Object o ) { return GetConstant( FindConstant( code, o ) ); }
	inline Object GetConstant( Object o ) { return GetConstant( cur_code, o ); }
	inline size_t NumConstants() { return constants.size(); }

	// module/namespace handling methods
	inline void AddModuleName( const string n ) { module_names.insert( n ); }

	// code execution methods:
	//
	// initialize for execution, using this code block as 'main'
	void Begin( const Code* const code = NULL );
	void End();
	// main entry point - shortcut to initialize, execute code block and exit
	void Execute( const Code* const code );
	void CallConstructors( Object o, Object instance, int num_args = 0 );
	void CallDestructors( Object o );
	// add a code block
	void AddCode( const Code* const code ) { code_blocks.push_back( code ); }
	// execute the current (top of stack) code block
	void ExecuteCode();
	Object ExecuteText( const char* const text, bool global = false, bool ignore_undefined_vars = false );
	Opcode SkipInstruction();
	Opcode ExecuteInstruction();
	void ExecuteToReturn( bool is_destructor = false );
	void ExecuteFunction( Function* f, int num_args, bool method_call_op, bool is_destructor = false );
	void ExecuteFunction( NativeFunction f, int num_args, bool method_call_op );

	// .dv file reading/writing
	void WriteCode( string filename, const Code* const code );
	Code* ReadCode( string filename );

	void SetError( Object* err );
	bool Error();
	Object GetError();
private:
	void DeleteErrorObject(){ if( is_error ) DecRef( error ); }

	// helper fcn for parsing and compiling a block of text
	const Code* LoadText( const char* const text, const char* const name, bool ignore_undefined_vars = false );

	bool ImportBuiltinModule( const char* module_name );
	// helper fcn to find a loaded module (namespace)
	vector< pair<string, ScopeTable*> >::iterator FindNamespace( string mod );
	// helper fcn for ImportModule:
	string FindModule( string mod );
	// helper fcn for parsing and compiling a module
	const Code* const LoadModule( string module_name, string path );

public:
	Object ImportModule( const char* module_name );

	// get the code block corresponding to a particular address
	Code* GetCode( byte* address );

	// debug and output methods:
	// decode and print an opcode/instruction stream
protected:
	int PrintOpcode( const Code*, Opcode op, const byte* base, byte* ip );
	inline int PrintOpcode( Opcode op, const byte* base, byte* ip ) { return PrintOpcode( cur_code, op, base, ip ); }
public:
	void Decode( const Code* code );

	void DumpFunctions();
	void DumpConstantPool( const Code* code );
	void DumpStackTop( size_t n = 5, bool single_line = true );
	void DumpTrace( ostream & os );

	const byte* const GetIP() const { return ip; }

	// process exit
	// TODO: anything needs doing here? prevent the process from exit, just the
	// executor?? (so debugger won't exit, perhaps?)
	void Exit( int exit_code ) { exit( exit_code ); }
};

extern Executor* ex;

} // namespace deva

#endif // __EXECUTOR_H__
