#!/bin/sh

echo
echo "RESOURCE_FILES("
echo "    PREFIX $1"
find $2 -name '*.sql' | sort | awk '{print "    " $0}'
echo ")"
echo

