bindir=$(top_srcdir)
# configure handles this, but for the record:
# gcc 4.3+ requires the -fno-inline-small-functions flag to disable this opt
# but gcc 4.2 does't have this flag
AM_CXXFLAGS=$(MY_CXXFLAGS)
bin_PROGRAMS = deva devac devadb test
deva_SOURCES= \
	builtins.cpp \
	compile.cpp \
	deva.cpp \
	dobject.cpp \
	error_report_parsers.cpp \
	executor.cpp \
	fileformat.cpp \
	instructions.cpp \
	parser_ids.cpp \
	scope.cpp \
	semantics.cpp \
	symbol.cpp \
	util.cpp \
	vector_builtins.cpp \
	map_builtins.cpp \
	string_builtins.cpp \
	module_os.cpp \
	module_bit.cpp \
	module_math.cpp \
	module_re.cpp
deva_CPPFLAGS = -I $(top_srcdir)/inc
# configure handles this, but for the record:
# -undefined dynamic_lookup required on Mac OS X to find Boost symbols...
deva_LDFLAGS = $(MY_LDFLAGS) -g -lboost_program_options -lboost_filesystem -lboost_regex

devac_SOURCES = devac.cpp symbol.cpp fileformat.cpp debug.cpp parser_ids.cpp error_report_parsers.cpp scope.cpp compile.cpp semantics.cpp instructions.cpp dobject.cpp
devac_CPPFLAGS = -I $(top_srcdir)/inc
devac_LDFLAGS = -g -lboost_program_options

devadb_SOURCES= \
	builtins.cpp \
	compile.cpp \
	devadb.cpp \
	dobject.cpp \
	error_report_parsers.cpp \
	executor.cpp \
	fileformat.cpp \
	instructions.cpp \
	parser_ids.cpp \
	scope.cpp \
	semantics.cpp \
	symbol.cpp \
	util.cpp \
	vector_builtins.cpp \
	map_builtins.cpp \
	string_builtins.cpp \
	module_os.cpp \
	module_bit.cpp \
	module_math.cpp \
	module_re.cpp
devadb_CPPFLAGS = -I $(top_srcdir)/inc
# -undefined dynamic_lookup required on Mac OS X to find Boost symbols...
devadb_LDFLAGS = $(MY_LDFLAGS) -g -lboost_program_options -lboost_filesystem -lboost_regex

test_SOURCES=test.cpp symbol.cpp parser_ids.cpp error_report_parsers.cpp dobject.cpp
test_CPPFLAGS = -I $(top_srcdir)/inc
test_LDFLAGS = -g -lboost_program_options

