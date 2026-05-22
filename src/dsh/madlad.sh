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

export PATH="$MADLAD_INSTALL/dsh/:$PATH"

# base commands
quit()            { sc quit $@; }
setv()            { sc setv $@; }
getv()            { sc getv $@; }
insert()          { sc insert $@; }
finsert()         { sc finsert $@; }
fwrite()          { sc fwrite "$@"; setv buf:path "$@"; }
eraseall()        { sc eraseall $@; }
cursor_u()        { sc cursor_u $@; }
cursor_d()        { sc cursor_d $@; }
cursor_l()        { sc cursor_l $@; }
cursor_r()        { sc cursor_r $@; }

# aliases
q()               { quit; }
e()               { edit $@; }
w()               { fwrite $@; }
g()               { getv $@; }
s()               { setv $@; }

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
