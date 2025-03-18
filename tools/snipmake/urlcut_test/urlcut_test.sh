#!/usr/bin/env bash

set -e

DEFAULT_TEST_FILE="../../../../arcadia_tests_data/snippets_tests_data/urls.gz"
TEST_FILE=${1:-$DEFAULT_TEST_FILE}
REFERENCE_MD5="tests/urls.gz.out.md5"
TEST_MD5="md5_"`date +%Y_%m_%d`".txt"

./urlcut_test < $TEST_FILE | md5sum | head -n 32 > $TEST_MD5


TEMP_DIFF_FILE="diff_"`date +%Y_%m_%d`".txt"
grep -Fxv -f $REFERENCE_MD5 $TEST_MD5 > $TEMP_DIFF_FILE

if test -s $TEMP_DIFF_FILE
then
    echo "DIFFERS! wrong md5 cheksum"
else
    echo "OK!"
fi

rm $TEST_MD5
rm $TEMP_DIFF_FILE

