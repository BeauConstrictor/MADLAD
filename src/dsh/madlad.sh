#!/bin/sh

# madlad.sh - the madlad script standard library
# 
# madlad.sh includes all the commands and variables you need to
# interact with madlad from a shell script. to use madlad.sh, just
# do:
#   eval $IMPORT_MADLAD
# at the top of your script.

if [ -n "$MADLAD_IMPORTED" ]; then
    exit
else
    MADLAD_IMPORTED="yes"
fi

export PATH="./build/dsh/:$PATH"

# base commands
quit()            { sc quit $@; }
setv()            { sc setv $@; }
getv()            { sc getv $@; }

# aliases
q()               { quit; }

# char escapes
lf='
'

BLACK="30"
RED="31"
GREEN="32"
YELLOW="33"
BLUE="34"
MAGENTA="35"
CYAN="36"
WHITE="37"
DEFAULT="39"
