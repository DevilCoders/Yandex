#!/bin/bash

set -e 
for img in $@
do
    fname=$(basename -- "$img")
    sfx="${img##*.}"
    name="${fname%.*}"
    catcmd='cat'
    case "$sfx" in
	zst|zstd)
	    catcmd='zstdcat'
	    ;;
	xz)
	    catcmd='xzcat'
	    ;;
	tgz|gz)
	    catcmd='zcat'
	    ;;
	bz2|bz)
	    catcmd='bzcat'
	    ;;
	tar)
	    catcmd='cat'
	    ;;
    esac
    $catcmd $img | docker load
done
