#!/usr/bin/env bash

#
# Script to create new branch in svn
#

MYDIR=`dirname "${BASH_SOURCE[0]}"`

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 branch_name"
    exit 1
fi

BRANCH=$1


svn copy ^/trunk/arcadia/gencfg ^/branches/gencfg/${BRANCH} -m "Created branch <${BRANCH}>"
svn copy ^/trunk/data/gencfg_db ^/branches/gencfg/${BRANCH}/db -m "Created branch <${BRANCH}> (no merge)"
