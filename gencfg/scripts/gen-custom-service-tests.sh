#!/usr/bin/env bash

if [ $# -lt 1 ]; then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "USAGE: $0 --db (trunk|tags) [--port PORT] [--host HOST] [-d -n THREADS] TESTID [TESTID..]"
    echo "       run \"source venv/venv/bin/activate && py.test --help\" for more details"
    source venv/venv/bin/activate
    py.test --help | tail -n +2
    deactivate
    exit 1
fi

echo "SHOTINLEG_PRINT_VENV"
echo `dirname "${BASH_SOURCE[0]}"`
echo `which python`
echo "SHOTINLEG_PRINT_VENV"

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh
run source `dirname "${BASH_SOURCE[0]}"`/../venv/venv/bin/activate

echo "SHOTINLEG_PRINT_VENV"
echo `dirname "${BASH_SOURCE[0]}"`
echo `which python`
echo "SHOTINLEG_PRINT_VENV"

PYTHONPATH=/skynet py.test $@
_ret=$?
run deactivate

exit ${_ret}
