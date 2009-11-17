bindir=$(top_srcdir)
# TODO: gcc 4.3+ requires this flag to disable this opt
# but gcc 4.2 does't have this flag
#AM_CXXFLAGS = -fno-inline-small-functions 
bin_PROGRAMS = deva devac test
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
	module_math.cpp
deva_CPPFLAGS = -I $(top_srcdir)/inc
# -undefined dynamic_lookup required on Mac OS X to find Boost symbols...
deva_LDFLAGS = -g -undefined dynamic_lookup -lboost_program_options -lboost_filesystem

devac_SOURCES = devac.cpp symbol.cpp fileformat.cpp debug.cpp parser_ids.cpp error_report_parsers.cpp scope.cpp compile.cpp semantics.cpp instructions.cpp dobject.cpp
devac_CPPFLAGS = -I $(top_srcdir)/inc
devac_LDFLAGS = -g -lboost_program_options

test_SOURCES=test.cpp symbol.cpp parser_ids.cpp error_report_parsers.cpp dobject.cpp
test_CPPFLAGS = -I $(top_srcdir)/inc
test_LDFLAGS = -g -lboost_program_options

