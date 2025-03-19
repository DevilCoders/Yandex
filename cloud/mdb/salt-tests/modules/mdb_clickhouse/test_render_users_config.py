# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_grains, mock_version_cmp, mock_vtype
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'ClickHouse 20.8, disabled SQL user management',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'users': {
                            'test_user': {
                                "hash": "hashed_password1",
                                "quotas": [],
                                "settings": {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'force_index_by_date': 1,
                                    'force_primary_key': 1,
                                    'transform_null_in': 1,
                                },
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_explicit_profile': {
                                "hash": "hashed_password2",
                                "quotas": [],
                                "profile": 'readonly',
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_quotas': {
                                "hash": "hashed_password3",
                                "quotas": [
                                    {
                                        'interval_duration': 3600,
                                        'queries': 10,
                                        'errors': 10,
                                        'result_rows': 1000,
                                        'read_rows': 1000000000,
                                        'execution_time': 600,
                                    },
                                ],
                                "settings": {},
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_keyed_quotas': {
                                'hash': 'hashed_password4',
                                'quotas': [
                                    {
                                        'interval_duration': 3600,
                                        'queries': 10,
                                        'errors': 10,
                                        'result_rows': 1000,
                                        'read_rows': 1000000000,
                                        'execution_time': 600,
                                    },
                                ],
                                'settings': {
                                    'quota_mode': 'keyed',
                                },
                                'databases': {
                                    'db1': {},
                                },
                            },
                            'test_user_with_table_filters': {
                                'hash': 'hashed_password5',
                                'quotas': [],
                                'settings': {},
                                'databases': {
                                    'db1': {
                                        'tables': {
                                            'table1': {
                                                'filter': 'a &gt; 100',
                                            },
                                        },
                                    },
                                },
                            },
                            'test_user_without_access_to_databases': {
                                'hash': 'hashed_password6',
                                'quotas': [],
                                'settings': {},
                                'databases': {},
                            },
                        },
                        'profiles': {
                            'readonly': {
                                'readonly': 1,
                                'allow_dll': 0,
                            },
                        },
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                            <access_management>1</access_management>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_admin</profile>
                            <quota>default</quota>
                        </_admin>
                        <test_user>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password1</password_sha256_hex>
                            <profile>test_user_profile</profile>
                            <quota>default</quota>
                        </test_user>
                        <test_user_with_explicit_profile>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password2</password_sha256_hex>
                            <profile>readonly</profile>
                            <quota>default</quota>
                        </test_user_with_explicit_profile>
                        <test_user_with_keyed_quotas>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password4</password_sha256_hex>
                            <profile>test_user_with_keyed_quotas_profile</profile>
                            <quota>test_user_with_keyed_quotas_quota</quota>
                        </test_user_with_keyed_quotas>
                        <test_user_with_quotas>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password3</password_sha256_hex>
                            <profile>test_user_with_quotas_profile</profile>
                            <quota>test_user_with_quotas_quota</quota>
                        </test_user_with_quotas>
                        <test_user_with_table_filters>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases>
                                <db1>
                                    <table1>
                                        <filter>a &gt; 100</filter>
                                    </table1>
                                </db1>
                            </databases>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password5</password_sha256_hex>
                            <profile>test_user_with_table_filters_profile</profile>
                            <quota>default</quota>
                        </test_user_with_table_filters>
                        <test_user_without_access_to_databases>
                            <allow_databases>
                                <database>system</database>
                            </allow_databases>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password6</password_sha256_hex>
                            <profile>test_user_without_access_to_databases_profile</profile>
                            <quota>default</quota>
                        </test_user_without_access_to_databases>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <readonly>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                        </readonly>
                        <test_user_profile>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <force_index_by_date>1</force_index_by_date>
                            <force_primary_key>1</force_primary_key>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                            <transform_null_in>1</transform_null_in>
                        </test_user_profile>
                        <test_user_with_keyed_quotas_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_keyed_quotas_profile>
                        <test_user_with_quotas_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_quotas_profile>
                        <test_user_with_table_filters_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_table_filters_profile>
                        <test_user_without_access_to_databases_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_without_access_to_databases_profile>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                        <test_user_with_keyed_quotas_quota>
                             <interval>
                                 <duration>3600</duration>
                                 <queries>10</queries>
                                 <errors>10</errors>
                                 <result_rows>1000</result_rows>
                                 <read_rows>1000000000</read_rows>
                                 <execution_time>600</execution_time>
                             </interval>
                             <keyed/>
                        </test_user_with_keyed_quotas_quota>
                        <test_user_with_quotas_quota>
                             <interval>
                                 <duration>3600</duration>
                                 <queries>10</queries>
                                 <errors>10</errors>
                                 <result_rows>1000</result_rows>
                                 <read_rows>1000000000</read_rows>
                                 <execution_time>600</execution_time>
                             </interval>
                        </test_user_with_quotas_quota>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 20.8, enabled SQL user management',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'sql_user_management': True,
                        'admin_password': 'admin_password',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'users': {},
                        'profiles': {
                            'readonly': {
                                'readonly': 1,
                                'allow_dll': 0,
                            },
                        },
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                            <access_management>1</access_management>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_admin</profile>
                            <quota>default</quota>
                        </_admin>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <readonly>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                        </readonly>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 20.12, disabled SQL user management',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.7.3',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'users': {
                            'test_user': {
                                "hash": "hashed_password1",
                                "quotas": [],
                                "settings": {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'force_index_by_date': 1,
                                    'force_primary_key': 1,
                                    'transform_null_in': 1,
                                },
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_explicit_defaults': {
                                "hash": "hashed_password2",
                                "quotas": [],
                                "settings": {"count_distinct_implementation": "", "load_balancing": ""},
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_explicit_profile': {
                                "hash": "hashed_password3",
                                "quotas": [],
                                "profile": 'readonly',
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_quotas': {
                                "hash": "hashed_password4",
                                "quotas": [
                                    {
                                        'interval_duration': 3600,
                                        'queries': 10,
                                        'errors': 10,
                                        'result_rows': 1000,
                                        'read_rows': 1000000000,
                                        'execution_time': 600,
                                    },
                                ],
                                "settings": {},
                                "databases": {
                                    "db1": {},
                                },
                            },
                            'test_user_with_keyed_quotas': {
                                'hash': 'hashed_password5',
                                'quotas': [
                                    {
                                        'interval_duration': 3600,
                                        'queries': 10,
                                        'errors': 10,
                                        'result_rows': 1000,
                                        'read_rows': 1000000000,
                                        'execution_time': 600,
                                    },
                                ],
                                'settings': {
                                    'quota_mode': 'keyed',
                                },
                                'databases': {
                                    'db1': {},
                                },
                            },
                            'test_user_with_table_filters': {
                                'hash': 'hashed_password6',
                                'quotas': [],
                                'settings': {},
                                'databases': {
                                    'db1': {
                                        'tables': {
                                            'table1': {
                                                'filter': 'a &gt; 100',
                                            },
                                        },
                                    },
                                },
                            },
                            'test_user_without_access_to_databases': {
                                'hash': 'hashed_password7',
                                'quotas': [],
                                'settings': {},
                                'databases': {},
                            },
                            'test_user_with_none_values': {
                                "hash": "hashed_password8",
                                "quotas": None,
                                "settings": None,
                                "databases": None,
                            },
                        },
                        'profiles': {
                            'readonly': {
                                'readonly': 1,
                                'allow_dll': 0,
                            },
                        },
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                            <access_management>1</access_management>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_admin</profile>
                            <quota>default</quota>
                        </_admin>
                        <test_user>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password1</password_sha256_hex>
                            <profile>test_user_profile</profile>
                            <quota>default</quota>
                        </test_user>
                        <test_user_with_explicit_defaults>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password2</password_sha256_hex>
                            <profile>test_user_with_explicit_defaults_profile</profile>
                            <quota>default</quota>
                        </test_user_with_explicit_defaults>
                        <test_user_with_explicit_profile>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password3</password_sha256_hex>
                            <profile>readonly</profile>
                            <quota>default</quota>
                        </test_user_with_explicit_profile>
                        <test_user_with_keyed_quotas>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password5</password_sha256_hex>
                            <profile>test_user_with_keyed_quotas_profile</profile>
                            <quota>test_user_with_keyed_quotas_quota</quota>
                        </test_user_with_keyed_quotas>
                        <test_user_with_none_values>
                            <allow_databases>
                                <database>system</database>
                            </allow_databases>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password8</password_sha256_hex>
                            <profile>test_user_with_none_values_profile</profile>
                            <quota>default</quota>
                        </test_user_with_none_values>
                        <test_user_with_quotas>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password4</password_sha256_hex>
                            <profile>test_user_with_quotas_profile</profile>
                            <quota>test_user_with_quotas_quota</quota>
                        </test_user_with_quotas>
                        <test_user_with_table_filters>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases>
                                <db1>
                                    <table1>
                                        <filter>a &gt; 100</filter>
                                    </table1>
                                </db1>
                            </databases>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password6</password_sha256_hex>
                            <profile>test_user_with_table_filters_profile</profile>
                            <quota>default</quota>
                        </test_user_with_table_filters>
                        <test_user_without_access_to_databases>
                            <allow_databases>
                                <database>system</database>
                            </allow_databases>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password7</password_sha256_hex>
                            <profile>test_user_without_access_to_databases_profile</profile>
                            <quota>default</quota>
                        </test_user_without_access_to_databases>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <readonly>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                        </readonly>
                        <test_user_profile>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <force_index_by_date>1</force_index_by_date>
                            <force_primary_key>1</force_primary_key>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                            <transform_null_in>1</transform_null_in>
                        </test_user_profile>
                        <test_user_with_explicit_defaults_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_explicit_defaults_profile>
                        <test_user_with_keyed_quotas_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_keyed_quotas_profile>
                        <test_user_with_none_values_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_none_values_profile>
                        <test_user_with_quotas_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_quotas_profile>
                        <test_user_with_table_filters_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_with_table_filters_profile>
                        <test_user_without_access_to_databases_profile>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </test_user_without_access_to_databases_profile>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                        <test_user_with_keyed_quotas_quota>
                             <interval>
                                 <duration>3600</duration>
                                 <queries>10</queries>
                                 <errors>10</errors>
                                 <result_rows>1000</result_rows>
                                 <read_rows>1000000000</read_rows>
                                 <execution_time>600</execution_time>
                             </interval>
                             <keyed/>
                        </test_user_with_keyed_quotas_quota>
                        <test_user_with_quotas_quota>
                             <interval>
                                 <duration>3600</duration>
                                 <queries>10</queries>
                                 <errors>10</errors>
                                 <result_rows>1000</result_rows>
                                 <read_rows>1000000000</read_rows>
                                 <execution_time>600</execution_time>
                             </interval>
                        </test_user_with_quotas_quota>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 20.12, enabled SQL user management',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.7.3',
                        'sql_user_management': True,
                        'admin_password': 'admin_password',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'users': {},
                        'profiles': {
                            'readonly': {
                                'readonly': 1,
                                'allow_dll': 0,
                            },
                        },
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                            <access_management>1</access_management>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_admin</profile>
                            <quota>default</quota>
                        </_admin>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <readonly>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                        </readonly>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 20.12, pillar without profiles',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.7.3',
                        'admin_password': 'admin_password',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'users': {
                            'test_user': {
                                "hash": "hashed_password1",
                                "quotas": [],
                                "settings": {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'force_index_by_date': 1,
                                    'force_primary_key': 1,
                                    'transform_null_in': 1,
                                },
                                "databases": {
                                    "db1": {},
                                },
                            },
                        },
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                            <access_management>1</access_management>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_admin</profile>
                            <quota>default</quota>
                        </_admin>
                        <test_user>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password1</password_sha256_hex>
                            <profile>test_user_profile</profile>
                            <quota>default</quota>
                        </test_user>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <test_user_profile>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <force_index_by_date>1</force_index_by_date>
                            <force_primary_key>1</force_primary_key>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                            <transform_null_in>1</transform_null_in>
                        </test_user_profile>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 20.12, pillar without profiles (set to none)',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.7.3',
                        'admin_password': 'admin_password',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'users': {
                            'test_user': {
                                "hash": "hashed_password1",
                                "quotas": [],
                                "settings": {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'force_index_by_date': 1,
                                    'force_primary_key': 1,
                                    'transform_null_in': 1,
                                },
                                "databases": {
                                    "db1": {},
                                },
                            },
                        },
                        'profiles': None,
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                            <access_management>1</access_management>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <host>man-2.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_admin</profile>
                            <quota>default</quota>
                        </_admin>
                        <test_user>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password1</password_sha256_hex>
                            <profile>test_user_profile</profile>
                            <quota>default</quota>
                        </test_user>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <test_user_profile>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <force_index_by_date>1</force_index_by_date>
                            <force_primary_key>1</force_primary_key>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                            <transform_null_in>1</transform_null_in>
                        </test_user_profile>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 20.12, pillar profile merges with default',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.7.3',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                            'background_fetches_pool_size': 16,
                        },
                        'users': {
                            'test_user': {
                                "hash": "hashed_password1",
                                "quotas": [],
                                "settings": {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'force_index_by_date': 1,
                                    'force_primary_key': 1,
                                    'transform_null_in': 1,
                                },
                                "databases": {
                                    "db1": {},
                                },
                            },
                        },
                        'profiles': {
                            'readonly': {
                                'readonly': 1,
                                'allow_dll': 0,
                            },
                            'default': {
                                'load_balancing': 'random',
                            },
                        },
                    },
                },
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <users>
                        <default>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>system</profile>
                            <quota>default</quota>
                        </default>
                        <_metrics>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_metrics</profile>
                            <quota>default</quota>
                        </_metrics>
                        <_monitor>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_monitor</profile>
                            <quota>default</quota>
                        </_monitor>
                        <_dns>
                            <networks>
                                <host>man-1.db.yandex.net</host>
                                <ip>::1</ip>
                                <ip>127.0.0.1</ip>
                            </networks>
                            <password/>
                            <profile>_dns</profile>
                            <quota>default</quota>
                        </_dns>
                        <_admin>
                          <access_management>1</access_management>
                          <networks>
                            <host>man-1.db.yandex.net</host>
                            <ip>::1</ip>
                            <ip>127.0.0.1</ip>
                          </networks>
                          <password/>
                          <profile>_admin</profile>
                          <quota>default</quota>
                        </_admin>
                        <test_user>
                            <allow_databases>
                                <database>system</database>
                                <database>db1</database>
                            </allow_databases>
                            <databases/>
                            <networks>
                                <ip>::/0</ip>
                            </networks>
                            <password_sha256_hex>hashed_password1</password_sha256_hex>
                            <profile>test_user_profile</profile>
                            <quota>default</quota>
                        </test_user>
                    </users>
                    <profiles>
                        <system>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </system>
                        <_metrics>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_metrics>
                        <_monitor>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>10</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_monitor>
                        <_dns>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                            <max_execution_time>5</max_execution_time>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <skip_unavailable_shards>1</skip_unavailable_shards>
                        </_dns>
                        <default>
                            <allow_drop_detached>1</allow_drop_detached>
                            <background_fetches_pool_size>16</background_fetches_pool_size>
                            <background_pool_size>16</background_pool_size>
                            <background_schedule_pool_size>16</background_schedule_pool_size>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <load_balancing>random</load_balancing>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </default>
                        <_admin>
                            <allow_drop_detached>1</allow_drop_detached>
                            <allow_introspection_functions>1</allow_introspection_functions>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>0</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                        </_admin>
                        <readonly>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                        </readonly>
                        <test_user_profile>
                            <allow_dll>0</allow_dll>
                            <allow_drop_detached>1</allow_drop_detached>
                            <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                            <force_index_by_date>1</force_index_by_date>
                            <force_primary_key>1</force_primary_key>
                            <insert_distributed_sync>1</insert_distributed_sync>
                            <join_algorithm>auto</join_algorithm>
                            <log_queries>1</log_queries>
                            <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                            <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                            <max_memory_usage>3221225472</max_memory_usage>
                            <partial_merge_join_optimizations>0</partial_merge_join_optimizations>
                            <readonly>1</readonly>
                            <transform_null_in>1</transform_null_in>
                        </test_user_profile>
                    </profiles>
                    <quotas>
                        <default>
                            <interval>
                                <duration>3600</duration>
                                <queries>0</queries>
                                <errors>0</errors>
                                <result_rows>0</result_rows>
                                <read_rows>0</read_rows>
                                <execution_time>0</execution_time>
                            </interval>
                        </default>
                    </quotas>
                </yandex>
                ''',
        },
    },
    {
        'id': 'ClickHouse 21.3, enabled cloud storage',
        'args': {
            'vtype': 'porto',
            'grains': {'id': 'man-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.3.7.28',
                        'admin_password': 'admin_password',
                        'config': {
                            'background_pool_size': 16,
                            'background_schedule_pool_size': 16,
                        },
                        'system_users': {
                            'mdb_backup_admin': {
                                'hash': 'backup_admin_hased_password',
                            },
                        },
                        'users': {
                            'test_user': {
                                "hash": "hashed_password1",
                                "quotas": [],
                                "settings": {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'force_index_by_date': 1,
                                    'force_primary_key': 1,
                                    'transform_null_in': 1,
                                },
                                "databases": {
                                    "db1": {},
                                },
                            },
                        },
                        'profiles': {
                            'readonly': {
                                'readonly': 1,
                                'allow_dll': 0,
                            },
                        },
                    },
                    'cloud_storage': {'enabled': True},
                },
            },
            'result': '''
            <?xml version="1.0"?>
            <yandex>
                <users>
                    <default>
                        <networks>
                            <host>man-1.db.yandex.net</host>
                            <host>man-2.db.yandex.net</host>
                            <ip>::1</ip>
                            <ip>127.0.0.1</ip>
                        </networks>
                        <password/>
                        <profile>system</profile>
                        <quota>default</quota>
                    </default>
                    <_metrics>
                        <networks>
                            <host>man-1.db.yandex.net</host>
                            <ip>::1</ip>
                            <ip>127.0.0.1</ip>
                        </networks>
                        <password/>
                        <profile>_metrics</profile>
                        <quota>default</quota>
                    </_metrics>
                    <_monitor>
                        <networks>
                            <host>man-1.db.yandex.net</host>
                            <ip>::1</ip>
                            <ip>127.0.0.1</ip>
                        </networks>
                        <password/>
                        <profile>_monitor</profile>
                        <quota>default</quota>
                    </_monitor>
                    <_dns>
                        <networks>
                            <host>man-1.db.yandex.net</host>
                            <ip>::1</ip>
                            <ip>127.0.0.1</ip>
                        </networks>
                        <password/>
                        <profile>_dns</profile>
                        <quota>default</quota>
                    </_dns>
                    <_admin>
                        <access_management>1</access_management>
                        <networks>
                            <host>man-1.db.yandex.net</host>
                            <host>man-2.db.yandex.net</host>
                            <ip>::1</ip>
                            <ip>127.0.0.1</ip>
                        </networks>
                        <password/>
                        <profile>_admin</profile>
                        <quota>default</quota>
                    </_admin>
                    <_backup_admin>
                        <access_management>1</access_management>
                        <networks>
                            <ip>::/0</ip>
                        </networks>
                        <password_sha256_hex>backup_admin_hased_password</password_sha256_hex>
                        <profile>_backup_admin</profile>
                        <quota>default</quota>
                    </_backup_admin>
                    <test_user>
                        <allow_databases>
                            <database>system</database>
                            <database>db1</database>
                        </allow_databases>
                        <databases/>
                        <networks>
                            <ip>::/0</ip>
                        </networks>
                        <password_sha256_hex>hashed_password1</password_sha256_hex>
                        <profile>test_user_profile</profile>
                        <quota>default</quota>
                    </test_user>
                </users>
                <profiles>
                    <system>
                        <allow_drop_detached>1</allow_drop_detached>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>0</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </system>
                    <_metrics>
                        <allow_drop_detached>1</allow_drop_detached>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>0</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>30</max_concurrent_queries_for_user>
                        <max_execution_time>10</max_execution_time>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </_metrics>
                    <_monitor>
                        <allow_drop_detached>1</allow_drop_detached>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>0</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                        <max_execution_time>10</max_execution_time>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </_monitor>
                    <_dns>
                        <allow_drop_detached>1</allow_drop_detached>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>0</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>10</max_concurrent_queries_for_user>
                        <max_execution_time>5</max_execution_time>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <skip_unavailable_shards>1</skip_unavailable_shards>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </_dns>
                    <default>
                        <allow_drop_detached>1</allow_drop_detached>
                        <background_pool_size>16</background_pool_size>
                        <background_schedule_pool_size>16</background_schedule_pool_size>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>1</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </default>
                    <_admin>
                        <allow_drop_detached>1</allow_drop_detached>
                        <allow_introspection_functions>1</allow_introspection_functions>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>0</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </_admin>
                    <_backup_admin>
                        <allow_drop_detached>1</allow_drop_detached>
                        <allow_experimental_geo_types>1</allow_experimental_geo_types>
                        <allow_experimental_live_view>1</allow_experimental_live_view>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>0</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </_backup_admin>
                    <readonly>
                        <allow_dll>0</allow_dll>
                        <allow_drop_detached>1</allow_drop_detached>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>1</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <readonly>1</readonly>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                    </readonly>
                    <test_user_profile>
                        <allow_dll>0</allow_dll>
                        <allow_drop_detached>1</allow_drop_detached>
                        <distributed_directory_monitor_batch_inserts>1</distributed_directory_monitor_batch_inserts>
                        <force_index_by_date>1</force_index_by_date>
                        <force_primary_key>1</force_primary_key>
                        <force_remove_data_recursively_on_drop>1</force_remove_data_recursively_on_drop>
                        <insert_distributed_sync>1</insert_distributed_sync>
                        <join_algorithm>auto</join_algorithm>
                        <log_queries>1</log_queries>
                        <log_queries_cut_to_length>10000000</log_queries_cut_to_length>
                        <max_concurrent_queries_for_user>450</max_concurrent_queries_for_user>
                        <max_memory_usage>3221225472</max_memory_usage>
                        <readonly>1</readonly>
                        <s3_max_single_part_upload_size>33554432</s3_max_single_part_upload_size>
                        <s3_min_upload_part_size>33554432</s3_min_upload_part_size>
                        <timeout_before_checking_execution_speed>300</timeout_before_checking_execution_speed>
                        <transform_null_in>1</transform_null_in>
                    </test_user_profile>
                </profiles>
                <quotas>
                    <default>
                        <interval>
                            <duration>3600</duration>
                            <queries>0</queries>
                            <errors>0</errors>
                            <result_rows>0</result_rows>
                            <read_rows>0</read_rows>
                            <execution_time>0</execution_time>
                        </interval>
                    </default>
                </quotas>
            </yandex>
            ''',
        },
    },
)
def test_render_users_config(vtype, grains, pillar, result):
    mock_vtype(mdb_clickhouse.__salt__, vtype)
    mock_grains(mdb_clickhouse.__salt__, grains)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_version_cmp(mdb_clickhouse.__salt__)
    assert_xml_equals(mdb_clickhouse.render_users_config(), result)
