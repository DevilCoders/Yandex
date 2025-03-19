#!/bin/bash
# log_tree stack overflow workaround
ulimit -s 16384
# macros-inc.m4 require ~5-9 second
# router.dnscache.full - 20
parallel -k --timeout 7 /usr/bin/rlog -r ::: $@ || true
