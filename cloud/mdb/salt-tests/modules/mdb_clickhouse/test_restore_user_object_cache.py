# -*- coding: utf-8 -*-
from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.mocks import mock_pillar


def mock_s3(salt, contents=None):
    store = {} if contents is None else contents

    def _object_exists(s3_client, key, s3_bucket='default'):
        return key in store.get(s3_bucket)

    def _create_object(s3_client, key, data=b'', s3_bucket='default'):
        store[s3_bucket][key] = data

    def _get_object(s3_client, key, s3_bucket='default'):
        return store[s3_bucket][key]

    salt['mdb_s3.client'] = lambda: None
    salt['mdb_s3.object_exists'] = _object_exists
    salt['mdb_s3.create_object'] = _create_object
    salt['mdb_s3.get_object'] = _get_object


@parametrize(
    {
        'id': 'nothing cached',
        'args': {
            's3_contents': {
                'default': {},
                'source_bucket': {},
            },
            'pillar': {},
            'restore_geobase': False,
            'result': [],
        },
    },
    {
        'id': 'restore all',
        'args': {
            's3_contents': {
                'default': {},
                'source_bucket': {
                    'object_cache/geobase/custom_clickhouse_geobase': b'geo',
                    'object_cache/format_schema/test_schema': b'sch',
                    'object_cache/format_schema/test_schema2': b'sch2',
                    'object_cache/ml_model/test_model': b'mod',
                    'object_cache/ml_model/test_model2': b'mod2',
                },
            },
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'geoabse-uri',
                        },
                        'format_schemas': {
                            'test_schema': {},
                            'test_schema2': {},
                        },
                        'models': {
                            'test_model': {},
                            'test_model2': {},
                        },
                    },
                },
            },
            'restore_geobase': True,
            'result': [
                'object_cache/geobase/custom_clickhouse_geobase',
                'object_cache/format_schema/test_schema',
                'object_cache/format_schema/test_schema2',
                'object_cache/ml_model/test_model',
                'object_cache/ml_model/test_model2',
            ],
        },
    },
    {
        'id': 'without geobase',
        'args': {
            's3_contents': {
                'default': {},
                'source_bucket': {
                    'object_cache/format_schema/test_schema': b'sch',
                    'object_cache/ml_model/test_model': b'mod',
                },
            },
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'geoabse-uri',
                        },
                        'format_schemas': {
                            'test_schema': {},
                        },
                        'models': {
                            'test_model': {},
                        },
                    },
                },
            },
            'restore_geobase': False,
            'result': [
                'object_cache/format_schema/test_schema',
                'object_cache/ml_model/test_model',
            ],
        },
    },
    {
        'id': 'not all cached',
        'args': {
            's3_contents': {
                'default': {},
                'source_bucket': {
                    'object_cache/geobase/custom_clickhouse_geobase': b'geo',
                    'object_cache/format_schema/test_schema2': b'sch2',
                    'object_cache/ml_model/test_model2': b'mod2',
                },
            },
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'geoabse-uri',
                        },
                        'format_schemas': {
                            'test_schema': {},
                            'test_schema2': {},
                        },
                        'models': {
                            'test_model': {},
                            'test_model2': {},
                        },
                    },
                },
            },
            'restore_geobase': True,
            'result': [
                'object_cache/geobase/custom_clickhouse_geobase',
                'object_cache/format_schema/test_schema2',
                'object_cache/ml_model/test_model2',
            ],
        },
    },
)
def test_restore_user_object_cache(s3_contents, pillar, restore_geobase, result):
    mock_s3(mdb_clickhouse.__salt__, s3_contents)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert set(
        mdb_clickhouse.restore_user_object_cache(backup_bucket='source_bucket', restore_geobase=restore_geobase)
    ) == set(result)
    assert s3_contents['default'] == s3_contents['source_bucket']
