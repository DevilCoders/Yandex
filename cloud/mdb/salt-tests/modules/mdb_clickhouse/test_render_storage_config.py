# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_version_cmp, mock_vtype
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'Default storage configuration, ClickHouse 20.8',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.5.45',
                    },
                },
            },
            'vtype': 'compute',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                    </disks>
                    <policies>
                      <default>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </default>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Cloud storage with HTTPS scheme and proxy resolver, ClickHouse 20.8',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.5.45',
                    },
                    'cloud_storage': {
                        'enabled': True,
                        's3': {
                            'scheme': 'https',
                            'bucket': 'bucket',
                            'endpoint': 's3.mds.yandex.net',
                            'virtual_hosted_style': 'true',
                            'access_key_id': 'access_key_id',
                            'access_secret_key': 'secret_access_key',
                            'proxy_resolver': {
                                'uri': 'http://s3.mds.yandex.net/hostname',
                                'proxy_port': {'http': '8080', 'https': '8443'},
                            },
                        },
                    },
                },
            },
            'vtype': 'compute',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                      <object_storage>
                        <type>s3</type>
                        <endpoint>https://bucket.s3.mds.yandex.net/cloud_storage/cid1/shard1/</endpoint>
                        <access_key_id>access_key_id</access_key_id>
                        <secret_access_key>secret_access_key</secret_access_key>
                        <max_connections>10000</max_connections>
                        <request_timeout_ms>3600000</request_timeout_ms>
                        <send_metadata>true</send_metadata>
                        <proxy>
                          <resolver>
                            <endpoint>http://s3.mds.yandex.net/hostname</endpoint>
                            <proxy_scheme>https</proxy_scheme>
                            <proxy_port>8443</proxy_port>
                          </resolver>
                        </proxy>
                      </object_storage>
                    </disks>
                    <policies>
                      <default>
                        <move_factor>0.01</move_factor>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                          <object_storage>
                            <disk>object_storage</disk>
                          </object_storage>
                        </volumes>
                      </default>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                      <object_storage>
                        <volumes>
                          <object_storage>
                            <disk>object_storage</disk>
                          </object_storage>
                        </volumes>
                      </object_storage>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Cloud storage with HTTP scheme and path style access, ClickHouse 20.8',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.5.45',
                    },
                    'cloud_storage': {
                        'enabled': True,
                        's3': {
                            'scheme': 'http',
                            'bucket': 'bucket',
                            'endpoint': 's3.mds.yandex.net',
                            'access_key_id': 'access_key_id',
                            'access_secret_key': 'secret_access_key',
                        },
                    },
                },
            },
            'vtype': 'compute',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                      <object_storage>
                        <type>s3</type>
                        <endpoint>http://s3.mds.yandex.net/bucket/cloud_storage/cid1/shard1/</endpoint>
                        <access_key_id>access_key_id</access_key_id>
                        <secret_access_key>secret_access_key</secret_access_key>
                        <max_connections>10000</max_connections>
                        <request_timeout_ms>3600000</request_timeout_ms>
                        <send_metadata>true</send_metadata>
                      </object_storage>
                    </disks>
                    <policies>
                      <default>
                        <move_factor>0.01</move_factor>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                          <object_storage>
                            <disk>object_storage</disk>
                          </object_storage>
                        </volumes>
                      </default>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                      <object_storage>
                        <volumes>
                          <object_storage>
                            <disk>object_storage</disk>
                          </object_storage>
                        </volumes>
                      </object_storage>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Cloud storage with HTTPS scheme and proxy resolver, ClickHouse 20.11',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.11.2.1',
                    },
                    'cloud_storage': {
                        'enabled': True,
                        's3': {
                            'scheme': 'https',
                            'bucket': 'bucket',
                            'endpoint': 's3.mds.yandex.net',
                            'virtual_hosted_style': 'true',
                            'access_key_id': 'access_key_id',
                            'access_secret_key': 'secret_access_key',
                            'proxy_resolver': {
                                'uri': 'http://s3.mds.yandex.net/hostname',
                                'proxy_port': {'http': '8080', 'https': '8443'},
                            },
                        },
                    },
                },
            },
            'vtype': 'compute',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                      <object_storage>
                        <type>s3</type>
                        <endpoint>https://bucket.s3.mds.yandex.net/cloud_storage/cid1/shard1/</endpoint>
                        <access_key_id>access_key_id</access_key_id>
                        <secret_access_key>secret_access_key</secret_access_key>
                        <max_connections>10000</max_connections>
                        <request_timeout_ms>3600000</request_timeout_ms>
                        <send_metadata>true</send_metadata>
                        <proxy>
                          <resolver>
                            <endpoint>http://s3.mds.yandex.net/hostname</endpoint>
                            <proxy_scheme>https</proxy_scheme>
                            <proxy_port>8443</proxy_port>
                          </resolver>
                        </proxy>
                      </object_storage>
                    </disks>
                    <policies>
                      <default>
                        <move_factor>0.01</move_factor>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                          <object_storage>
                            <disk>object_storage</disk>
                            <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
                            <prefer_not_to_merge>true</prefer_not_to_merge>
                          </object_storage>
                        </volumes>
                      </default>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                      <object_storage>
                        <volumes>
                          <object_storage>
                            <disk>object_storage</disk>
                            <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
                            <prefer_not_to_merge>true</prefer_not_to_merge>
                          </object_storage>
                        </volumes>
                      </object_storage>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Cloud storage with HTTP scheme and path style access, ClickHouse 20.11',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.11.2.1',
                    },
                    'cloud_storage': {
                        'enabled': True,
                        's3': {
                            'scheme': 'http',
                            'bucket': 'bucket',
                            'endpoint': 's3.mds.yandex.net',
                            'access_key_id': 'access_key_id',
                            'access_secret_key': 'secret_access_key',
                        },
                    },
                },
            },
            'vtype': 'compute',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                      <object_storage>
                        <type>s3</type>
                        <endpoint>http://s3.mds.yandex.net/bucket/cloud_storage/cid1/shard1/</endpoint>
                        <access_key_id>access_key_id</access_key_id>
                        <secret_access_key>secret_access_key</secret_access_key>
                        <max_connections>10000</max_connections>
                        <request_timeout_ms>3600000</request_timeout_ms>
                        <send_metadata>true</send_metadata>
                      </object_storage>
                    </disks>
                    <policies>
                      <default>
                        <move_factor>0.01</move_factor>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                          <object_storage>
                            <disk>object_storage</disk>
                            <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
                            <prefer_not_to_merge>true</prefer_not_to_merge>
                          </object_storage>
                        </volumes>
                      </default>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                      <object_storage>
                        <volumes>
                          <object_storage>
                            <disk>object_storage</disk>
                            <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
                            <prefer_not_to_merge>true</prefer_not_to_merge>
                          </object_storage>
                        </volumes>
                      </object_storage>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Cloud storage with advanced disk options',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.11.2.1',
                    },
                    'cloud_storage': {
                        'enabled': True,
                        's3': {
                            'scheme': 'http',
                            'bucket': 'bucket',
                            'endpoint': 's3.mds.yandex.net',
                            'access_key_id': 'access_key_id',
                            'access_secret_key': 'secret_access_key',
                        },
                        'settings': {
                            'move_factor': 0.4,
                            'connect_timeout_ms': 1000,
                            'request_timeout_ms': 1000,
                            'metadata_path': 'foo',
                            'cache_enabled': False,
                            'min_multi_part_upload_size': 1000,
                            'min_bytes_for_seek': 1000,
                            'skip_access_check': False,
                            'max_connections': 100,
                            'send_metadata': True,
                            'thread_pool_size': 1,
                            'list_object_keys_size': 10,
                            'data_cache_enabled': True,
                            'data_cache_max_size': 1024,
                            'prefer_not_to_merge': False,
                        },
                    },
                },
            },
            'vtype': 'compute',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                      <object_storage>
                        <type>s3</type>
                        <endpoint>http://s3.mds.yandex.net/bucket/cloud_storage/cid1/shard1/</endpoint>
                        <access_key_id>access_key_id</access_key_id>
                        <secret_access_key>secret_access_key</secret_access_key>
                        <max_connections>100</max_connections>
                        <request_timeout_ms>1000</request_timeout_ms>
                        <send_metadata>true</send_metadata>
                        <connect_timeout_ms>1000</connect_timeout_ms>
                        <metadata_path>foo</metadata_path>
                        <cache_enabled>false</cache_enabled>
                        <min_multi_part_upload_size>1000</min_multi_part_upload_size>
                        <min_bytes_for_seek>1000</min_bytes_for_seek>
                        <skip_access_check>false</skip_access_check>
                        <thread_pool_size>1</thread_pool_size>
                        <list_object_keys_size>10</list_object_keys_size>
                        <data_cache_enabled>true</data_cache_enabled>
                        <data_cache_max_size>1024</data_cache_max_size>
                      </object_storage>
                    </disks>
                    <policies>
                      <default>
                        <move_factor>0.4</move_factor>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                          <object_storage>
                            <disk>object_storage</disk>
                            <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
                            <prefer_not_to_merge>false</prefer_not_to_merge>
                          </object_storage>
                        </volumes>
                      </default>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                      <object_storage>
                        <volumes>
                          <object_storage>
                            <disk>object_storage</disk>
                            <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
                            <prefer_not_to_merge>false</prefer_not_to_merge>
                          </object_storage>
                        </volumes>
                      </object_storage>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Cloud storage with HTTP scheme and path style access, ClickHouse 20.8, DoubleCloud',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.5.45',
                    },
                    'cloud_storage': {
                        'enabled': True,
                        's3': {
                            'scheme': 'http',
                            'bucket': 'bucket',
                            'endpoint': 's3.mds.yandex.net',
                            'access_key_id': 'access_key_id',
                            'access_secret_key': 'secret_access_key',
                        },
                    },
                },
            },
            'vtype': 'aws',
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                  <storage_configuration>
                    <disks>
                      <default/>
                      <object_storage>
                        <type>s3</type>
                        <endpoint>http://s3.mds.yandex.net/bucket/cloud_storage/cid1/shard1/</endpoint>
                        <access_key_id>access_key_id</access_key_id>
                        <secret_access_key>secret_access_key</secret_access_key>
                        <max_connections>10000</max_connections>
                        <request_timeout_ms>3600000</request_timeout_ms>
                        <send_metadata>true</send_metadata>
                      </object_storage>
                    </disks>
                    <policies>
                      <default>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </default>
                      <hybrid_storage>
                        <move_factor>0.01</move_factor>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                          <object_storage>
                            <disk>object_storage</disk>
                          </object_storage>
                        </volumes>
                      </hybrid_storage>
                      <local>
                        <volumes>
                          <default>
                            <disk>default</disk>
                          </default>
                        </volumes>
                      </local>
                      <object_storage>
                        <volumes>
                          <object_storage>
                            <disk>object_storage</disk>
                          </object_storage>
                        </volumes>
                      </object_storage>
                    </policies>
                  </storage_configuration>
                </yandex>
                ''',
        },
    },
)
def test_render_storage_config(pillar, vtype, result):
    mock_vtype(mdb_clickhouse.__salt__, vtype)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_version_cmp(mdb_clickhouse.__salt__)
    assert_xml_equals(mdb_clickhouse.render_storage_config(), result)
