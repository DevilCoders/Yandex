#!/usr/bin/env python
# -*- coding: utf-8 -*-

from init import run_test
import yatest.common
import pytest
import os

sc_dir = yatest.common.source_path("tools/domschemec/tests/data")


@pytest.mark.parametrize("filename",
                          [f for f in os.listdir(sc_dir) if os.path.isfile(os.path.join(sc_dir, f))])
def test_regression(filename):
    return run_test(sc_dir, filename)
