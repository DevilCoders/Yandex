#!/bin/bash
#
# Generate changelog for spec file from rpm

latest_rev=$(svn log -q -l1 config-monitoring-common.spec | awk '/^r/ { print $1 }')
svn log -r HEAD:$latest_rev | egrep -v '^(r|-|$)' | sed 's|\(.*\)|- \1|g'
