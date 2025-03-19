# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_elasticsearch
from cloud.mdb.salt_tests.common.mocks import mock_pillar
import yatest.common


def test_extension_executable():
    path = yatest.common.test_source_path("data/exec.zip")
    mock_pillar(mdb_elasticsearch.__salt__, {})
    try:
        mdb_elasticsearch.validate_extension('test', path)
    except RuntimeError as e:
        assert str(e).startswith('Invalid file type in extension')
        return
    assert False


def test_extension_cyrilic_and_chinese_dicts():
    path = yatest.common.test_source_path("data/unicode.zip")
    mock_pillar(mdb_elasticsearch.__salt__, {})
    try:
        mdb_elasticsearch.validate_extension('test', path)
    except RuntimeError:
        assert False


def test_extension_ascii_dict():
    path = yatest.common.test_source_path("data/ascii.zip")
    mock_pillar(mdb_elasticsearch.__salt__, {})
    try:
        mdb_elasticsearch.validate_extension('test', path)
    except RuntimeError:
        assert False


# 42Kb -> 5.5Gb
def test_extension_zip_bomb():
    path = yatest.common.test_source_path("data/zbsm.zip")
    mock_pillar(mdb_elasticsearch.__salt__, {})
    try:
        mdb_elasticsearch.validate_extension('test', path)
    except RuntimeError as e:
        assert str(e).startswith('Extension contains too many files')
        return
    assert False


def test_extension_cyrilic_and_spaces_in_name():
    path = yatest.common.test_source_path("data/names.zip")
    mock_pillar(mdb_elasticsearch.__salt__, {})
    try:
        mdb_elasticsearch.validate_extension('test', path)
    except RuntimeError:
        assert False


def test_extension_two_dot():
    path = yatest.common.test_source_path("data/out.zip")
    mock_pillar(mdb_elasticsearch.__salt__, {})
    try:
        mdb_elasticsearch.validate_extension('test', path)
    except RuntimeError as e:
        assert str(e).startswith('Invalid filename in extension')
        return
    assert False
