// module_os.h
// built-in module 'os' for the deva language 
// created by jcs, november 6, 2009 

// TODO:
// * 


#include "module_os.h"
#include <cstdlib>

void do_os_exec( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'os' function 'exec'." );

	// command line to exec is on top of the stack
	DevaObject obj = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( obj.Type() == sym_unknown )
	{
		o = ex->find_symbol( obj );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'command' argument in module 'os' function 'exec'" );
	}
	if( !o )
		o = &obj;

	// ensure command is a string
	if( o->Type() != sym_string )
		throw DevaRuntimeException( "'command' argument to module 'os' function 'exec' must be a string." );

	int ret = system( o->str_val );

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void AddOsModule( Executor & ex )
{
	map<string, builtin_fcn> fcns = map<string, builtin_fcn>();
	fcns.insert( make_pair( string( "exec@os" ), do_os_exec ) );
	ex.AddBuiltinModule( string( "os" ), fcns );
}
