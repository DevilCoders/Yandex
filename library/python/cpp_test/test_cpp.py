import json
import os
import subprocess

import pytest
import yaml
import yatest

import library.python.resource as lpr


STYLE_CONFIG = yaml.safe_load(lpr.find('/cpp_style/config'))
STYLE_CONFIG_JSON = json.dumps(STYLE_CONFIG)

RES_FILE_PREFIX = '/cpp_style/files/'
CHECKED_PATHS = list(lpr.iterkeys(RES_FILE_PREFIX, strip_prefix=True))


def check_style(filename, actual_source):
    clang_format_binary = yatest.common.binary_path('contrib/libs/clang12/tools/clang-format/clang-format')
    command = [clang_format_binary, '-assume-filename=' + filename, '-style=' + STYLE_CONFIG_JSON]
    styled_source = subprocess.check_output(command, input=actual_source)

    assert actual_source.decode() == styled_source.decode()


@pytest.mark.parametrize('path', CHECKED_PATHS)
def test_cpp_style(path):
    check_style(os.path.basename(path), lpr.find(RES_FILE_PREFIX + path))
