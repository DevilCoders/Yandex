# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_elasticsearch
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize
from mock import patch

import pytest


@parametrize(
    {
        'id': 'Master and data nodes',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['elasticsearch_cluster.masternode'],
                                    'hosts': {
                                        'man-1.db.yandex.net': {},
                                        'sas-1.db.yandex.net': {},
                                        'vla-1.db.yandex.net': {},
                                    },
                                },
                                'subcid2': {
                                    'roles': ['elasticsearch_cluster.datanode'],
                                    'hosts': {
                                        'man-2.db.yandex.net': {},
                                        'sas-2.db.yandex.net': {},
                                        'vla-2.db.yandex.net': {},
                                    },
                                },
                            },
                        },
                    },
                },
            },
            'result': (
                {
                    'master_nodes': {'man-1.db.yandex.net': {}, 'sas-1.db.yandex.net': {}, 'vla-1.db.yandex.net': {}},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'sas-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
        },
    },
    {
        'id': 'Single data node',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['elasticsearch_cluster.datanode'],
                                    'hosts': {
                                        'man-1.db.yandex.net': {},
                                    },
                                },
                            },
                        },
                    },
                },
            },
            'result': ({'master_nodes': {}, 'data_nodes': {'man-1.db.yandex.net': {}}}),
        },
    },
)
def test_hosts(pillar, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    assert mdb_elasticsearch.hosts() == result


@parametrize(
    {
        'id': 'Master and data nodes',
        'args': {
            'hosts': (
                {
                    'master_nodes': {'man-1.db.yandex.net': {}, 'vla-1.db.yandex.net': {}, 'iva-1.db.yandex.net': {}},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
            'result': [
                'iva-1.db.yandex.net',
                'man-1.db.yandex.net',
                'man-2.db.yandex.net',
                'vla-1.db.yandex.net',
                'vla-2.db.yandex.net',
            ],
        },
    },
    {
        'id': 'Only data nodes',
        'args': {
            'hosts': (
                {
                    'master_nodes': {},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
            'result': ['man-2.db.yandex.net', 'vla-2.db.yandex.net'],
        },
    },
)
def test_seed_peers(hosts, result):
    with patch('cloud.mdb.salt.salt._modules.mdb_elasticsearch.hosts') as hosts_mock:
        hosts_mock.return_value = hosts
        assert mdb_elasticsearch.seed_peers() == result


@parametrize(
    {
        'id': 'Master and data nodes',
        'args': {
            'hosts': (
                {
                    'master_nodes': {'man-1.db.yandex.net': {}, 'vla-1.db.yandex.net': {}, 'iva-1.db.yandex.net': {}},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
            'result': ['iva-1.db.yandex.net', 'man-1.db.yandex.net', 'vla-1.db.yandex.net'],
        },
    },
    {
        'id': 'Only data nodes',
        'args': {
            'hosts': (
                {
                    'master_nodes': {},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
            'result': ['man-2.db.yandex.net', 'vla-2.db.yandex.net'],
        },
    },
)
def test_initial_master_nodes(hosts, result):
    with patch('cloud.mdb.salt.salt._modules.mdb_elasticsearch.hosts') as hosts_mock:
        hosts_mock.return_value = hosts
        assert mdb_elasticsearch.initial_master_nodes() == result


@parametrize(
    {
        'id': 'Master and data nodes',
        'args': {
            'hosts': (
                {
                    'master_nodes': {'man-1.db.yandex.net': {}, 'vla-1.db.yandex.net': {}, 'iva-1.db.yandex.net': {}},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
            'result': ['https://man-2.db.yandex.net:9200', 'https://vla-2.db.yandex.net:9200'],
        },
    },
    {
        'id': 'Only data nodes',
        'args': {
            'hosts': (
                {
                    'master_nodes': {},
                    'data_nodes': {'man-2.db.yandex.net': {}, 'vla-2.db.yandex.net': {}},
                }
            ),
            'result': ['https://man-2.db.yandex.net:9200', 'https://vla-2.db.yandex.net:9200'],
        },
    },
)
def test_data_nodes(hosts, result):
    with patch('cloud.mdb.salt.salt._modules.mdb_elasticsearch.hosts') as hosts_mock:
        hosts_mock.return_value = hosts
        assert mdb_elasticsearch.data_node_urls() == result


@parametrize(
    {
        'id': 'Some settings defined in pillar',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'config': {
                            'data_node': {
                                'not_found_setting': 'value',
                                'fielddata_cache_size': '10%',
                                'max_clause_count': '512mb',
                                'reindex_ssl_ca_path': 'some_path'
                            },
                        },
                    },
                },
            },
            'result': ({
                'indices.fielddata.cache.size': '10%',
                'indices.query.bool.max_clause_count': '512mb',
                'reindex.ssl.certificate_authorities': ['/etc/elasticsearch/certs/ca.pem', 'some_path']
            }),
        },
    },
    {
        'id': 'No setting defined',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'config': {
                            'data_node': {},
                        },
                    },
                },
            },
            'result': ({'reindex.ssl.certificate_authorities': ['/etc/elasticsearch/certs/ca.pem']}),
        },
    },
)
def test_data_node_settings(pillar, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    assert mdb_elasticsearch.data_node_settings() == result


@parametrize(
    {
        'id': 'General case (single -)',
        'args': {
            'fqdn': 'iva-host.yandex.net',
            'result': 'iva',
        },
    },
    {
        'id': 'Multiple -',
        'args': {
            'fqdn': 'vla-new-host.yandex.net',
            'result': 'vla',
        },
    },
)
def test_geo_correct(fqdn, result):
    with patch('cloud.mdb.salt.salt._modules.mdb_elasticsearch.fqdn') as fqdn_mock:
        fqdn_mock.return_value = fqdn
        assert mdb_elasticsearch.geo() == result


@parametrize(
    {
        'id': 'No -',
        'args': {
            'fqdn': 'ivahost.yandex.net',
        },
    },
    {
        'id': 'Empty fqdn',
        'args': {
            'fqdn': '',
        },
    },
)
def test_geo_incorrect(fqdn):
    with patch('cloud.mdb.salt.salt._modules.mdb_elasticsearch.fqdn') as fqdn_mock:
        fqdn_mock.return_value = fqdn
        with pytest.raises(Exception):
            mdb_elasticsearch.geo()
