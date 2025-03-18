#!/usr/bin/env bash

#
# Script update current repository. Not needed in svn, keep it for backward compability.
#

svn update ||
(
    echo "============================================================="
    echo "SVN UPDATE FAILED"
    echo "============================================================="
)

svn update db ||
(
    echo "============================================================="
    echo "SVN UPDATE IN DB FAILED"
    echo "============================================================="
)
