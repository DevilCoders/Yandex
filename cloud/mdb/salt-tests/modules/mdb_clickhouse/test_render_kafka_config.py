# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'no kafka config',
        'args': {
            'pillar': {},
            'result': {
                'kafka': {'ssl_ca_location': '/etc/clickhouse-server/ssl/allCAs.pem'},
            },
        },
    },
    {
        'id': 'default kafka CAs',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'kafka': {'sasl_mechanism': 'PLAIN', 'security_protocol': 'SSL'},
                            'kafka_topics': [
                                {
                                    'name': 'kafka_topic',
                                    'settings': {'sasl_mechanism': 'PLAIN', 'security_protocol': 'SSL'},
                                },
                                {
                                    'name': 'new_topic',
                                    'settings': {'sasl_mechanism': 'PLAIN', 'security_protocol': 'SASL_SSL'},
                                },
                            ],
                        },
                    },
                },
            },
            'result': {
                'kafka': {
                    'sasl_mechanism': 'PLAIN',
                    'security_protocol': 'SSL',
                    'ssl_ca_location': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'kafka_kafka_topic': {
                    'sasl_mechanism': 'PLAIN',
                    'security_protocol': 'SSL',
                    'ssl_ca_location': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'kafka_new_topic': {
                    'sasl_mechanism': 'PLAIN',
                    'security_protocol': 'SASL_SSL',
                    'ssl_ca_location': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
            },
        },
    },
    {
        'id': 'custom, removed and default certificate',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'kafka': {
                                'sasl_mechanism': 'PLAIN',
                                'security_protocol': 'SSL',
                                'ca_cert': '-----BEGIN CERTIFICATE-----',
                            },
                            'kafka_topics': [
                                {
                                    'name': 'kafka_topic',
                                    'settings': {
                                        'sasl_mechanism': 'PLAIN',
                                        'security_protocol': 'SSL',
                                        'ca_cert': "",
                                    },
                                },
                                {
                                    'name': 'new_topic',
                                    'settings': {'sasl_mechanism': 'PLAIN', 'security_protocol': 'SASL_SSL'},
                                },
                            ],
                        },
                    },
                },
            },
            'result': {
                'kafka': {
                    'sasl_mechanism': 'PLAIN',
                    'security_protocol': 'SSL',
                    'ssl_ca_location': '/etc/clickhouse-server/ssl/kafkaCAs.pem',
                },
                'kafka_kafka_topic': {
                    'sasl_mechanism': 'PLAIN',
                    'security_protocol': 'SSL',
                },
                'kafka_new_topic': {
                    'sasl_mechanism': 'PLAIN',
                    'security_protocol': 'SASL_SSL',
                    'ssl_ca_location': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
            },
        },
    },
)
def test_render_kafka_config(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mdb_clickhouse.__salt__['dbaas.is_public_ca'] = lambda: False
    assert mdb_clickhouse.render_kafka_config() == result
