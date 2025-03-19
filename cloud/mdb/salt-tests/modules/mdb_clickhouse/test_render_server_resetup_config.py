# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_version_cmp

from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals


@parametrize(
    {
        'id': 'ClickHouse 21.8',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.8.15.7',
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionaries_lazy_load>1</dictionaries_lazy_load>
                    <distributed_ddl>
                        <path>/clickhouse/task_queue/fake_ddl</path>
                    </distributed_ddl>
                    <http_port>28123</http_port>
                    <https_port>29443</https_port>
                    <tcp_port>29000</tcp_port>
                    <tcp_port_secure>29440</tcp_port_secure>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 22.3',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '22.3.6.5',
                    },
                },
            },
            'result': '''
            <?xml version="1.0"?>
            <yandex>
                <dictionaries_lazy_load>1</dictionaries_lazy_load>
                <distributed_ddl>
                    <path>/clickhouse/task_queue/fake_ddl</path>
                </distributed_ddl>
                <http_port>28123</http_port>
                <https_port>29443</https_port>
                <merge_tree>
                    <check_sample_column_is_correct>0</check_sample_column_is_correct>
                </merge_tree>
                <tcp_port>29000</tcp_port>
                <tcp_port_secure>29440</tcp_port_secure>
            </yandex>
            ''',
        },
    },
)
def test_render_server_resetup_config(pillar, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert_xml_equals(mdb_clickhouse.render_server_resetup_config(), result)
