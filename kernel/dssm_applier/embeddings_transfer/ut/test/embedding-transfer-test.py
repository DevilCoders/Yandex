#!/usr/bin/env python

import yatest.common
import logging
import subprocess
import sys


def test_embedding_transfer():
    logger = logging.getLogger("test")

    bin_path = yatest.common.binary_path('kernel/dssm_applier/embeddings_transfer/ut/applier/applier')
    diff_path = yatest.common.binary_path('kernel/dssm_applier/embeddings_transfer/ut/diff_tool/diff_tool')
    input_path = yatest.common.work_path('in.txt')
    output_path = yatest.common.work_path('out.txt')
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
