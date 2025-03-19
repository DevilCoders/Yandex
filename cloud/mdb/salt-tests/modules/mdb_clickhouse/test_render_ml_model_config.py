# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import pytest
from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals


def test_render_ml_model_config_succeeded():
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'models': {
                        'model1': {
                            'type': 'catboost',
                            'uri': 'uri1',
                        },
                        'model2': {
                            'type': 'catboost',
                            'uri': 'uri2',
                        },
                    },
                },
            },
        },
    )
    assert_xml_equals(
        mdb_clickhouse.render_ml_model_config('model1'),
        '''
        <?xml version="1.0"?>
        <models>
            <model>
                <name>model1</name>
                <type>catboost</type>
                <path>/var/lib/clickhouse/models/model1.bin</path>
                <lifetime>0</lifetime>
            </model>
        </models>
        ''',
    )


def test_render_ml_model_config_failed():
    mock_pillar(mdb_clickhouse.__salt__, {})
    with pytest.raises(KeyError):
        mdb_clickhouse.render_ml_model_config('model1')
