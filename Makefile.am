bindir=$(top_srcdir)
SUBDIRS=src
EXTRA_DIST=inc tests deva-language-1.0.odt runtests.dv dotest dotest_compile dotest_exec dotest_valgrind valgrind.supp

# remove subversion files
dist-hook:
	-find $(distdir) -name ".svn" -exec rm -rf {} \; 

# run tests
check:
	./deva runtests.dv

