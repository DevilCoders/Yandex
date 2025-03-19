#!/usr/bin/env python

import yatest.common
import logging
import subprocess
import sys


def test_batch_processor():
    logger = logging.getLogger("test")

    bin_path = yatest.common.binary_path('kernel/bert/tests/batch_runner/batch_runner')
    diff_path = yatest.common.binary_path('kernel/bert/tests/batch_runner/diff_tool/diff_tool')
    input_path = yatest.common.work_path('input.txt')
    output_path = yatest.common.work_path('output.txt')
    cmd = [
        bin_path,
        '-i',
        input_path,
        '-o',
        output_path
    ]
    logger.info("Running cmd: {}".format(cmd))

    subprocess.check_call(cmd, stdout=sys.stderr, stderr=subprocess.STDOUT)
    return yatest.common.canonical_file(output_path, diff_tool=diff_path)
