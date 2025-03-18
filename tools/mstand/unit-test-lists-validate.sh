#!/bin/sh

set -e

echo "Comparing test lists..."
echo "Ya.Test"
ya_res=".test-list-ya-make"
../../ya make -t -L 2>&1 | awk -F :: '{print $3}' | awk  '{print $1}' | grep test | sort > "$ya_res"

echo "Py.Test"
py_res=".test-list-py-test"
py.test --collect-only | grep '<Function'  | awk '{print $2}' | sed s@"'"@@ | sed s@"'>"@@ | sort > "$py_res"

if ! diff -ub "$ya_res" "$py_res"; then
    echo "Tests lists mismatch detected. Check your CMakeLists.txt files across project."
    exit 1
else
    echo "Py.Test and Ya.Test test lists are identical."
fi
