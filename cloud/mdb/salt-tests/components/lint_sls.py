#!/usr/bin/python
# coding: utf-8

import os
import os.path
import subprocess

import yatest.common


def test_sls_ok():
    linter = yatest.common.build_path('cloud/mdb/salt-tests/obeyjinja/obeyjinja')
    config = yatest.common.source_path('cloud/mdb/salt-tests/obeyjinja/.obeyjinjarc')
    components = yatest.common.source_path('cloud/mdb/salt/salt/components')
    sls = []
    for root, _, files in os.walk(components):
        for file in files:
            if file.endswith(('.sls', '.jinja')):
                sls.append(os.path.join(root, file))
    process = subprocess.Popen([linter, '--config', config] + sls, stderr=subprocess.PIPE)
    _, stderr = process.communicate()
    if process.returncode != 0:
        raise AssertionError('Lint failed:\n{stderr}'.format(stderr=stderr))
