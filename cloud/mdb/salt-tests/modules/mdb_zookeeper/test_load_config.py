# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from collections import OrderedDict

import pytest
import tempfile

from cloud.mdb.salt.salt._modules import mdb_zookeeper


@pytest.mark.parametrize(
    ids=[
        'Base test',
    ],
    argnames=['config', 'expected'],
    argvalues=[
        (
            '''
                # Comment
                0key=0value
                1key  =  1value
                2key=2value  # Comment

                3key=3value=with=equals=signs==
            ''',
            OrderedDict(
                [
                    ('0key', '0value'),
                    ('1key', '1value'),
                    ('2key', '2value'),
                    ('3key', '3value=with=equals=signs=='),
                ]
            ),
        ),
    ],
)
def test_load_config(config, expected):
    config_file = make_config_file(config)
    result = mdb_zookeeper.load_config(config_file.name)
    assert result == expected


def make_config_file(config):
    tmp = tempfile.NamedTemporaryFile()
    tmp.write(config.encode('utf-8'))
    tmp.flush()
    tmp.seek(0)
    return tmp
