#!/usr/bin/python
# python script to run all the deva lang tests
# walks the 'tests' sub-dir looking for directories with a file called
# 'baseline'. when found, it tries to run "dotest" in the local directory, which
# should be a program or link to a program to run the actual test on an input
# file called "input.dv" and comparing the results to the 'baseline' file

import os
import sys

baseline = "baseline"

# TODO: to this (machine int-size detection) correctly ??
hosttype = os.getenv( "HOSTTYPE" )
if hosttype == "x86_64":
    baseline_sized = "baseline.64"
else:
    baseline_sized = "baseline"

#print( baseline )
#print( baseline_sized )

home = os.getenv( "HOME" )
init_dir = os.getcwd()

failed_tests = []

dotest = "./dotest"

# check for valgrind cmd line option
valgrind = False
if len( sys.argv ) > 1:
    if sys.argv[1] == "v" or sys.argv[1] == "valgrind":
        valgrind = True
        dotest = "./dotest_valgrind"

# run 'dotest input.dv baseline' in each sub-dir
tests = 0
for root, dir, files in os.walk( os.path.join( os.getcwd(), "tests" ) ):
    if baseline in files:
        print( "processing " + root + " ..." )
        # if there is a 'disable' file, don't run this test
        if "disable" not in files:
            tests += 1
            os.chdir( root )
            if baseline_sized in files:
                bl = baseline_sized
            else:
                bl = baseline
        #        ret = os.spawnl( os.P_WAIT, "./dotest", "dotest", "input.dv", bl )
            if valgrind:
                ret = os.spawnl( os.P_WAIT, dotest, "dotest", "input.dv" )
            else:
                ret = os.spawnl( os.P_WAIT, dotest, "dotest", "input.dv", bl )
            if ret != 0:
                failed_tests.append( root )
        else:
            print( "test disabled" );

os.chdir( init_dir )

# re-print failures:
if len( failed_tests ) != 0:
    print( "\n" + str( len( failed_tests ) ) + " FAILED tests:" )
    for s in failed_tests:
        print( s )
else:
	print( "\nall tests SUCCEEDED" )
