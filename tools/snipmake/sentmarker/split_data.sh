#!/bin/sh -e

#split multiline output of
# csnip -r json -E shits5
#into {1..n}.js files to be used in list.js, dat/{1..n}.js
awk '{ f=("./" NR ".js"); print "window.load (" $0 " );" > f; close(f) }'
