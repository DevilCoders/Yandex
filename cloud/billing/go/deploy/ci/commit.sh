#!/bin/bash
set -xe

# Make arc commit from CI flow with all changed files
# Args:
#   $1 - commit message

ARC=${ARC_BIN:-arc}

$ARC ci -a -m "$1"
$ARC push
