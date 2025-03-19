#!/bin/bash
set -xe

# Script makes PR from release branch to trunk to save all changes from flow and fixes

ARC=${ARC_BIN:-arc}

ROBOT=robot-yc-billing-sdk

CUR_BRANCH=`$ARC branch |grep '\*'| cut -c3-`
TEMP_BRANCH="to-trunk/$CUR_BRANCH"

$ARC branch $TEMP_BRANCH
$ARC checkout $TEMP_BRANCH
$ARC push -u users/robot-yc-billing-sdk/$TEMP_BRANCH
$ARC pr create -m "$1 Changes from $CUR_BRANCH" --no-edit --to trunk -A --publish
