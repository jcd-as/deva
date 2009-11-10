// error_report_parsers.cpp
// definitions of the error report parsers 
// used by the deva language grammar
// created by jcs, september 9, 2009 

// TODO:
// *

#include "grammar.h"
#include "compile.h"

// error printing helper function
void report_error( file_position pos, char const* msg )
{
//	cout << "Syntax error at line " << pos.line << ": " << msg << endl;
	// format = filename:linenum: msg
	// TODO: filename!
	// also, this should call emit_error(), but the test program can't link to
	// compile.cpp
	cout << pos.file << ":" << pos.line << ":" << " error: " << msg << endl;
}


// error report parser definitions:
//////////////////////////////////////////////////////////
error_report_p error_missing_closing_brace = 
	error_report_parser ( 0, "Expecting a closing brace, but found something else" );

error_report_p error_invalid_else = 
	error_report_parser( 0, "Malformed else clause" );

error_report_p error_invalid_if = 
	error_report_parser( 0, "Malformed if clause" );

error_report_p error_invalid_statement =
	error_report_parser( 0, "Invalid statement" );

error_report_p error_invalid_func_decl =
	error_report_parser( 0, "Invalid function declaration" );

error_report_p error_invalid_while =
	error_report_parser( 0, "Invalid while statement" );

error_report_p error_invalid_for =
	error_report_parser( 0, "Invalid for statement" );
