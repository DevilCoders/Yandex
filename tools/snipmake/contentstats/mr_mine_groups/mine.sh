#!/usr/bin/env bash
set -e -o pipefail

dsttable=pzuev/urlgroups_full
srctable=urldats.1417443095
mr=/Berkanavt/bin/mapreduce
export MR_DEF_SERVER=cedar00:8013
export MR_USER=snippets
$mr -src $srctable -dst $dsttable -map './mr_mine_groups' -file ~/s/ybuild/latest/bin/mr_mine_groups
$mr -src $dsttable -dst $dsttable -sort
