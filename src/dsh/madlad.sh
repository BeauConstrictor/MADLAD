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

: "${MADLAD_INSTALL:?MADLAD_INSTALL is not set.}"

export PATH="$MADLAD_INSTALL/dsh/:$PATH"

# BASE COMMANDS
quit()     { sc quit $@; }                         # no arguments
setv()     { sc setv $@; }                         # variable, value
getv()     { sc getv $@; }                         # variable
insert()   { sc insert $@; }                       # string
finsert()  { sc finsert $@; }                      # file path
fwrite()   { sc fwrite "$@"; setv buf:path "$@"; } # (file path)
eraseall() { sc eraseall $@; }                     # no arguments
cursor_u() { sc cursor_u $@; }                     # number
cursor_d() { sc cursor_d $@; }                     # number
cursor_l() { sc cursor_l $@; }                     # number
cursor_r() { sc cursor_r $@; }                     # number

# COMPOUND COMMANDS
#
# MADLAD has so called 'compound commands' - commands which are
# composed of the based commands above. To find these, look in
#   src/dsh/
# Besides madlad.sh, this directory contains all the other commands
# available from within MADLAD. This directory is added to PATH,
# which is how you can access compound commands

# VARIABLES
#
# Using getv and setv (or g and s), you can get and set MADLAD's
# 'variables'. Some of these variables only support one of either
# getting and setting. Here are some of these variables:
#
# - filetype: the syntax highlighter to use
# - highlightcol: a specific column to highlight
# - version (get-only): MADLAD's version
# - buf:path: the path of the current buffer (may be empty)
# - buf:lines (get-only): the number of the lines in the buffer

# ALIASES
q()               { quit; }
e()               { edit $@; }
w()               { fwrite $@; }
g()               { getv $@; }
s()               { setv $@; }
wq()              { w $@; q; }

# CHARS
lf='
'

# COLOR CODES (not useful just yet)
BLACK="30"
RED="31"
GREEN="32"
YELLOW="33"
BLUE="34"
MAGENTA="35"
CYAN="36"
WHITE="37"
DEFAULT="39"
