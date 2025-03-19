#!/usr/bin/env bash

set -eux

_term() {
    echo "Caught SIGTERM signal!"
    kill -TERM "$child" 2>/dev/null
    exit 0
}
trap _term SIGTERM

TEST_DATA_PATH="$1"
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR
rm -rf pytest/pytest_venv
python3 -m venv pytest/pytest_venv
set +u
. pytest/pytest_venv/bin/activate
set -u
python3 -m pip install -i https://pypi.yandex-team.ru/simple/ -r pytest/requirements.txt

while true
do
    python3 -m pytest pytest/ --full --test_data_path $TEST_DATA_PATH \
        -x --keep-on-failure --log-file=$TEST_DATA_PATH/common.log \
        -m 'not vhost_fs' \
        --junitxml=$TEST_DATA_PATH/result.xml &
    child=$!
    wait "$child"
    status=$?
    if [ $status -ne 0 ]; then
        echo "pytest failed with code $status"
        exit 1
    fi
done
