#!/bin/sh
. ./error_functions.sh

if [ $# -ne 1 ]
then
    warn "Turns dumped pure to the format suitable for the pure_compiler as input."
    warn "The dump is read from stdin, the body of the pure is written to stdout,"
    warn "The header with language fingerprints is written to HEADER_FILE."
    warn "Usage: $0 HEADER_FILE"
    exit 1
fi

RunAndKill python rearrange.py $1 | 
   LC_ALL=C sort
