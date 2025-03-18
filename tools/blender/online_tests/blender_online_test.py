#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

import blender_test_util
import different_results_test

SRC = 'hamster.yandex.ru'

def run_tests(queries, src):
    tests = []
    tests.append(different_results_test.DifferentNumberOfResults(src, queries))
    failed = []
    for t in tests:
        success = t.run()
        if not success:
            failed.append(t)
    return failed


def get_mailing_list():
    f = open('mailing_list.txt', 'r')
    res = []
    for line in f:
        res += [line.strip()]


def send_report(failed_tests):
    body = 'Failed tests:\n'
    files = []
    for t in failed_tests:
        body += '\t%s\t, see results in: %s\n' % (t.test_name(), t.report_name())
        files.append(t.report_name())

    if len(failed_tests) > 0:
       blender_test_util.send_mail('blender-test@yandex-team.ru', ['epar'], 'Blender online test failed', body, files)


def main():
    q = [line.strip() for line in sys.stdin]
    failed = run_tests(q, SRC)
    if len(failed) > 0:
        print 'Some tests failed'
        #send_report(failed)
    else:
        print 'Success'

if __name__ == '__main__':
    main()
