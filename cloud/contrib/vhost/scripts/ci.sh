#!/bin/bash

. $(dirname $0)/remote_job_helper.sh

set -xu

result_no_asan="test-result-noasan.xml"
result_asan="test-result-asan.xml"
result_rel_path="test/work/result.xml"

run_test_remote "
    make -j check TEST_CACHE_DIR=\$TEST_CACHE_DIR
"
test_res=$?
run_test_remote "cat $result_rel_path" > $result_no_asan

if [ $test_res -ne 0]; then
    exit $test_res
fi

run_test_remote "
    make -j clean
    make -j check TEST_CACHE_DIR=\$TEST_CACHE_DIR WITH_ASAN=1
"
test_res=$?
run_test_remote "cat $result_rel_path" > $result_asan

exit $test_res
