#!/usr/bin/env bash

function colorecho() {
    CLR=$1
    shift

    if [ -t 1 ]; then
        ncolors=$(tput colors)
        if test -n "$ncolors" && test ${ncolors} -ge 8; then
            echo "$(tput setaf ${CLR})$(tput bold)$@$(tput sgr 0)"
        fi
    else
        echo "$@"
    fi
}

# ==========================================================================================
# Run command and write something on fail status
# ==========================================================================================
function run() {
    OLD_PWD=`pwd`

    cd `dirname "${BASH_SOURCE[0]}"`/..

    if [ -n "$GENCFG_VERBOSE" ]; then
        colorecho 4 "Run command <$@>"
    fi

    if "$@"
    then
        cd ${OLD_PWD}
    else
        colorecho 1 "Command <$@> failed ... exiting"
        cd ${OLD_PWD}
        exit 1
    fi


}

# ==========================================================================================
# Run some commands in parallel. Set exit code to 1 if at least one of commands failed
# ==========================================================================================
function par() {
    if [ `uname` == "Linux" ]; then
        : ${MAXPROC_DIVIDER:=2}
        MAXCHILDS=$((`nproc` / $MAXPROC_DIVIDER))
        MAKE_OPTIONS="-B"
    elif [ `uname` == "FreeBSD" ]; then
        MAXCHILDS=`sysctl -n hw.ncpu`
        MAKE_OPTIONS=""
    elif [ `uname` == "Darwin" ]; then
        MAXCHILDS=`sysctl -n hw.ncpu`
        MAKE_OPTIONS=""
    else
        exit 1
    fi

    mypath=`realpath ${BASH_SOURCE[0]}`
    mydir=`dirname ${mypath}`
    (
        for jb in "$@" ; do
            echo -e "$((++i)):\n\t$mydir/makearound.sh $jb"
        done
        echo all: $(seq 1 ${i})
    ) | make ${MAKE_OPTIONS} -k -j ${MAXCHILDS} -f <(cat -) all
}
function download_sandbox_resource() {
    NAME=$1
    SBR=`python -c "import json; print(json.loads(open('${MYDIR}/sandbox.resources').read())['${NAME}']['resource'])"`
    if [ -z ${TMPDIR+x} ]; then
        TMPDIR="/var/tmp"
    fi

    RESOURCE_DIR=${TMPDIR}/${SBR}
    mkdir -p ${RESOURCE_DIR}

    if sky get -d ${RESOURCE_DIR} --deduplicate=Hardlink sbr:${SBR}; then
        tar -xzf ${RESOURCE_DIR}/*
    else
        echo "[ERROR] Could not download ${NAME} resource using sbr:${SBR}"
        exit 1
    fi
}
