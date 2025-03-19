#!/bin/sh -e

if echo "$1" | nc -i1 localhost 2181 2>/dev/null | egrep -q "$2"; then exit 0; else exit 1; fi

# vim: set ts=4 sw=4 et:
