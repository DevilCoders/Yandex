#!/usr/bin/env bash
#
# Checks out the specified gencfg version.
#


if [ "$#" -ge 2 ]; then
    echo "Usage: $0 [branch name]"
    exit 1
fi

if [ "$#" -eq 1 ]; then
    REPO=$1
else
    REPO="trunk"
fi

MYDIR=`dirname "${BASH_SOURCE[0]}"`

(
cd ${MYDIR};

if [ "${REPO}" == "trunk" ] || [ "${REPO}" == "master" ]; then
    echo "Checking out trunk"
    svn switch -q ^/gencfgmain/trunk
else
    # check if we have tag
    if (svn ls `svn info | grep "Repository Root" | awk '{print $NF}'`/gencfgmain/tags | sed 's#/##' | grep -xq "${REPO}"); then
        echo "Checkint out tag ${REPO}"
        svn switch -q ^/gencfgmain/tags/${REPO}
    # check if we have branch
    elif (svn ls `svn info | grep "Repository Root" | awk '{print $NF}'`/gencfgmain/branches | sed 's#/##' | grep -xq "${REPO}"); then
        echo "Checking out branch ${REPO}"
        svn switch -q ^/gencfmain/branches/{${REPO}}
    else
        echo "Repo name <${REPO}> is not a tag or branch name"
        exit 1
    fi
fi
)
