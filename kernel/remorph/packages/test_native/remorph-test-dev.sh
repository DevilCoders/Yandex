#!/usr/bin/env bash

set -eo pipefail

function callable {
    [ -n "$1" ] || return 1
    type -p "$1" >/dev/null || return 1
}

function error {
    echo >&2 "Error: $@"
    exit 1
}

function require {
    callable $1 || error "Test requirement not found: $1"
}

function test_case {
    tcase="$1"
    tcases[$tcase]="OK"
}

function test_fail {
    [ -n "${tcase}" ] || error "Test internal error"
    tcases[$tcase]="FAIL"
}

function test_case_finish {
    [ -n "${tcase}" ] || error "Test internal error"
    printf "[test] %20s: ${tcases[$tcase]}" "${tcase}"
    echo
}

echo "Remorph dev environment testing"

require dirname
require mktemp
require g++

declare -A tcases
script_dir=$(dirname $0)
work_dir=$(mktemp -d)

test_case app_info_links
g++ -o "${work_dir}/info_app" "${script_dir}/info.cpp" -lyandex-remorph || test_fail
test_case_finish

test_case app_info_runs
"${work_dir}/info_app" || test_fail
test_case_finish

test_case compiler_exists
callable remorphc || test_fail
test_case_finish

test_case compiler_runs
remorphc --info >/dev/null || test_fail
test_case_finish

test_case gztcompiler_exists
callable gztcompiler || test_fail
test_case_finish

failed=0
for status in "${tcases[@]}"; do
    if [ "${status}" == "FAIL" ]; then
        ((failed+=1))
    fi
done

if [ ${failed} -ne 0 ]; then
    echo "Failed: ${failed}"
fi
exit ${failed}
