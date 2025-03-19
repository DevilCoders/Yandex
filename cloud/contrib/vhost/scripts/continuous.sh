#!/bin/bash

. $(dirname $0)/remote_job_helper.sh

set -xu

test_latest_workdir_symlink="latest_$QEMU_VERSION_BASE"
test_stdout="previous-run-stdout.txt"
test_stderr="previous-run-stderr.txt"
test_result="previous-run-result.xml"

run_test_remote "
    cat $REMOTE_TEST_ROOT/$test_latest_workdir_symlink/test-stdout
" > $test_stdout

run_test_remote "
    cat $REMOTE_TEST_ROOT/$test_latest_workdir_symlink/test-stderr
" > $test_stderr

run_test_remote "
    cat $REMOTE_TEST_ROOT/$test_latest_workdir_symlink/result.xml
" > $test_result

set -e

run_test_remote "scripts/serve_continuous_test.py $QEMU_VERSION_BASE"
