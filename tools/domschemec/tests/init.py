#!/usr/bin/env python
# -*- coding: utf-8 -*-

import yatest.common
import tempfile
import os

def run_test(sc_dir, filename):
    output = tempfile.NamedTemporaryFile(suffix = filename).name
    yatest.common.execute(
        [ yatest.common.binary_path("tools/domschemec/domschemec"),
          "--in", os.path.join(sc_dir, filename),
          "--out", output ]
    )
    return yatest.common.canonical_file(output)
