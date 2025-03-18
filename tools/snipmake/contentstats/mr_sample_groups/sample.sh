#!/usr/bin/env bash
set -e -o pipefail

srctable=urldats.1417443095
tmptable=pzuev/urls_to_download
dsttable=pzuev/urls_to_download_filtered
mr=/Berkanavt/bin/mapreduce
program=$HOME/s/ybuild/latest/tools/snipmake/contentstats/mr_sample_groups/mr_sample_groups
export MR_DEF_SERVER=cedar00:8013
export MR_USER=snippets
$mr -src $srctable -dst $tmptable -subkey -map './mr_sample_groups map' -file $program -file groups.txt
$mr -src $tmptable -dst $tmptable -sort
$mr -src $tmptable -dst $dsttable -reduce './mr_sample_groups reduce' -file $program
$mr -src $dsttable -dst $dsttable -sort
