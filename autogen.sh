#!/bin/sh
# generate makefiles etc for deva

aclocal
automake -a
autoconf

