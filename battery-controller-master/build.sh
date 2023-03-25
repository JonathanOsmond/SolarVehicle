#!/usr/bin/env bash

# Move to script directory
cd "${0%/*}"

CMD=$1

case $CMD in
    debug)
        POSTCMD="./gdb.sh"
        shift
        ;;
    flash)
        POSTCMD="flash-mbed build/*.bin"
        shift
        ;;
esac

mbed compile --profile mbed-os/tools/profiles/debug.json --profile c++11.profile.json --build ./build "$@" || exit 1

if [[ $POSTCMD ]]
then
    $POSTCMD
fi
