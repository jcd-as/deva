// module_bit.cpp
// built-in module 'bit' for the deva language 
// created by jcs, november 6, 2009 

// TODO:
// * 


#include "module_bit.h"

void do_bit_and( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'bit' function 'and'." );

	// number to 'and' is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// number to 'and' it with is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'bit' function 'and'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'operand' argument in module 'bit' function 'and'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	// TODO: args should be *integral* numbers, check them
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'bit' function 'and' must be an integral number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'operand' argument to module 'bit' function 'and' must be an integral number." );

	// TODO: lossy cast. support double-sized integers??
	size_t n = (size_t)(o->num_val);
	size_t op = (size_t)(a->num_val);
	size_t ret = n & op;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_bit_or( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'bit' function 'or'." );

	// number to 'and' is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// number to 'or' it with is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'bit' function 'or'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'operand' argument in module 'bit' function 'or'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	// TODO: args should be *integral* numbers, check them
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'bit' function 'or' must be an integral number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'operand' argument to module 'bit' function 'or' must be an integral number." );

	// TODO: lossy cast. support double-sized integers??
	size_t n = (size_t)(o->num_val);
	size_t op = (size_t)(a->num_val);
	size_t ret = n | op;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_bit_xor( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'bit' function 'xor'." );

	// number to 'and' is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// number to 'xor' it with is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'bit' function 'xor'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'operand' argument in module 'bit' function 'xor'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	// TODO: args should be *integral* numbers, check them
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'bit' function 'xor' must be an integral number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'operand' argument to module 'bit' function 'xor' must be an integral number." );

	// TODO: lossy cast. support double-sized integers??
	size_t n = (size_t)(o->num_val);
	size_t op = (size_t)(a->num_val);
	size_t ret = n ^ op;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_bit_complement( Executor* ex )
{
	if( Executor::args_on_stack != 1 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'bit' function 'complement'." );

	// number to 'and' is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'bit' function 'complement'" );
	}
	if( !o )
		o = &num;

	// ensure arg is a number
	// TODO: arg should be an *integral* number, check
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'bit' function 'complement' must be an integral number." );

	// TODO: lossy cast. support double-sized integers??
	size_t n = (size_t)(o->num_val);
	size_t ret = ~n;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_bit_shift_left( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'bit' function 'shift_left'." );

	// number to 'and' is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// number to 'shift_left' it with is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'bit' function 'shift_left'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'operand' argument in module 'bit' function 'shift_left'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	// TODO: args should be *integral* numbers, check them
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'bit' function 'shift_left' must be an integral number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'operand' argument to module 'bit' function 'shift_left' must be an integral number." );

	// TODO: lossy cast. support double-sized integers??
	size_t n = (size_t)(o->num_val);
	size_t op = (size_t)(a->num_val);
	size_t ret = n << op;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void do_bit_shift_right( Executor* ex )
{
	if( Executor::args_on_stack != 2 )
		throw DevaRuntimeException( "Incorrect number of arguments to module 'bit' function 'shift_right'." );

	// number to 'and' is on top of the stack
	DevaObject num = ex->stack.back();
	ex->stack.pop_back();
	
	// number to 'shift_right' it with is next
	DevaObject arg = ex->stack.back();
	ex->stack.pop_back();
	
	DevaObject* o = NULL;
	if( num.Type() == sym_unknown )
	{
		o = ex->find_symbol( num );
		if( !o )
			throw DevaRuntimeException( "Symbol not found for 'input' argument in module 'bit' function 'shift_right'" );
	}
	if( !o )
		o = &num;

	DevaObject* a = NULL;
	if( arg.Type() == sym_unknown )
	{
		a = ex->find_symbol( arg );
		if( !a )
			throw DevaRuntimeException( "Symbol not found for 'operand' argument in module 'bit' function 'shift_right'" );
	}
	if( !a )
		a = &arg;

	// ensure args are numbers
	// TODO: args should be *integral* numbers, check them
	if( o->Type() != sym_number )
		throw DevaRuntimeException( "'input' argument to module 'bit' function 'shift_right' must be an integral number." );
	if( a->Type() != sym_number )
		throw DevaRuntimeException( "'operand' argument to module 'bit' function 'shift_right' must be an integral number." );

	// TODO: lossy cast. support double-sized integers??
	size_t n = (size_t)(o->num_val);
	size_t op = (size_t)(a->num_val);
	size_t ret = n >> op;

	// pop the return address
	ex->stack.pop_back();

	// return the return value from the command
	ex->stack.push_back( DevaObject( "", (double)ret ) );
}

void AddBitModule( Executor & ex )
{
	map<string, builtin_fcn> fcns = map<string, builtin_fcn>();
	fcns.insert( make_pair( string( "and@bit" ), do_bit_and ) );
	fcns.insert( make_pair( string( "or@bit" ), do_bit_or ) );
	fcns.insert( make_pair( string( "xor@bit" ), do_bit_xor ) );
	fcns.insert( make_pair( string( "complement@bit" ), do_bit_complement ) );
	fcns.insert( make_pair( string( "shift_left@bit" ), do_bit_shift_left ) );
	fcns.insert( make_pair( string( "shift_right@bit" ), do_bit_shift_right ) );
	ex.AddBuiltinModule( string( "bit" ), fcns );
}

