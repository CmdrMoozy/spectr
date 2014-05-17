#!/bin/bash
#
# Script to clean up & rebuild spectr.
# This will build our application in DEBUG mode.
#
# NOTE: Any arguments passed to this script will be propgated along to cmake.
# This can be used, e.g., to set various cmake defines or etc. for a custom
# build type.

# Clean up our previous build, if any.

./clean.sh

if [[ $? -ne 0 ]];
then
	echo "FATAL: Couldn't find scripts. Are you in the source tree root?"
	exit 1
fi

# Make sure the directories we need exist.

./dir-prepare.sh

if [[ $? -ne 0 ]];
then
	echo "FATAL: Couldn't find scripts. Are you in the source tree root?"
	exit 1
fi

cd build
cmake -DCMAKE_BUILD_TYPE=Debug "$@" ..
make
cd ..
