// builtins.cpp
// built-in functions for the deva language virtual machine
// created by jcs, october 04, 2009 

// TODO:
// * 

#include "builtins.h"
#include <iostream>

// is this name a built-in function?
bool is_builtin( string name )
{
	if( name == "print"
		|| name == "str"
		|| name == "append"
		)
	return true;
	else return false;
}

void do_print( Executor *ex, const Instruction & inst )
{
	ex->Output( inst );

	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

void do_str( Executor *ex, const Instruction & inst )
{
	// get the argument off the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	// if it's a variable, locate it in the symbol table
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &obj;

	ostringstream s;

	// evaluate it
	switch( o->Type() )
	{
		case sym_number:
			s << o->num_val;
			break;
		case sym_string:
			s << o->str_val;
			break;
		case sym_boolean:
			if( o->bool_val )
				s << "true";
			else
				s << "false";
			break;
		case sym_null:
			s << "null";
			break;
		case sym_map:
			{
			// dump map contents
			s << "map: '" << o->name << "' = " << endl;
			map<DevaObject, DevaObject>* mp = o->map_val;
			for( map<DevaObject, DevaObject>::iterator it = mp->begin(); it != mp->end(); ++it )
			{
				DevaObject key = (*it).first;
				DevaObject val = (*it).second;
				s << key << " : " << val << endl;
			}
			break;
			}
		case sym_vector:
			{
			// dump vector contents
			s << "vector: '" << o->name << "' = " << endl;
			vector<DevaObject>* vec = o->vec_val;
			for( vector<DevaObject>::iterator it = vec->begin(); it != vec->end(); ++it )
			{
				DevaObject val = (*it);
				s << val << endl;
			}
			break;
			}
		case sym_function:
			s << "function: '" << o->name << "'";
			break;
		case sym_function_call:
			s << "function_call: '" << o->name << "'";
			break;
		default:
			s << "unknown: '" << o->name << "'";
	}

	// pop the return address
	ex->stack.pop_back();

	// push the string onto the stack
	ex->stack.push_back( DevaObject( "", s.str() ) );
}

void do_append( Executor *ex, const Instruction & inst )
{
	// value is on top of stack
	DevaObject val = ex->stack.back();
	ex->stack.pop_back();
	// vector or string to append to is next-to-top
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	// string
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found in function call" );
	}
	if( !o )
		o = &obj;

	if( o->Type() == sym_string )
	{
		// value has to be a string too
		if( val.Type() != sym_string )
			throw DevaRuntimeException( "Cannot append a non-string to a string in append() built-in." );
		// concat the strings
		string ret( o->str_val );
		ret += val.str_val;
		o->SetValue( DevaObject( "", ret ) );
	}
	// vector
	else if( o->Type() == sym_vector )
	{
		o->vec_val->push_back( val );
	}
	// pop the return address
	ex->stack.pop_back();

	// all fcns return *something*
	ex->stack.push_back( DevaObject( "", sym_null ) );
}

// exectute built-in function
void execute_builtin( Executor *ex, const Instruction & inst )
{
	string name = inst.args[0].name;
	if( name == "print" )
		do_print( ex, inst );
	else if( name == "str" )
		do_str( ex, inst );
	else if( name == "append" )
		do_append( ex, inst );
	else
		throw DevaICE( "No such built-in function." );
}

