#!/usr/bin/env bash
set -e -o pipefail

srctable=pzuev/urlgroups_full
dsttable=pzuev/urlgroup_sizes
mr=/Berkanavt/bin/mapreduce
export MR_DEF_SERVER=cedar00:8013
export MR_USER=snippets
$mr -src $srctable -dst $dsttable -reduce './mr_reduce_groups' -file ~/s/ybuild/latest/bin/mr_reduce_groups
$mr -src $dsttable -dst $dsttable -sort
