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

    cd `dirname "${BASH_SOURCE[0]}"`

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

PYPI_URL="http://pypi.yandex-team.ru/simple"
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Building shared virtualenv..."
# ====================================================================================
run rm -rf ${MYDIR}/venv
run mkdir -p ${MYDIR}/venv/venv
VENVPATH=`realpath ${MYDIR}/venv/venv`

run /skynet/python/bin/virtualenv ${VENVPATH}
# ====================================================================================

echo "Installing common packages..."
# ====================================================================================
source ${MYDIR}/venv/venv/bin/activate
echo "Virtual environment activated"
echo `which python`

run ${MYDIR}/venv/venv/bin/python ${MYDIR}/venv/venv/bin/pip install -U ${MYDIR}/wheels/pip-7.1.0.tar.gz
run ${MYDIR}/venv/venv/bin/python ${MYDIR}/venv/venv/bin/pip install --no-index --find-links=${MYDIR}/wheels -r ${MYDIR}/requirements.txt

run deactivate
echo "Virtual environment deactivated"
echo `which python`
# ====================================================================================

