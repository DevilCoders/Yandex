# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_version_cmp
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'Dictionary with HTTP source',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'http_source': {'url': 'test_url', 'format': 'TSV'},
                'structure': {'id': {'name': 'id'}, 'attributes': [{'name': 'text', 'type': 'String'}]},
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <http>
                                <url>test_url</url>
                                <format>TSV</format>
                            </http>
                        </source>
                        <structure>
                            <id>
                                <name>id</name>
                            </id>
                            <attribute>
                                <name>text</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with MySQL source',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'mysql_source': {
                    'db': 'test_db',
                    'table': 'test_table',
                    'user': 'test_user',
                    'password': 'test_password',
                    'replicas': [
                        {
                            'host': 'test_host',
                            'priority': 1,
                        }
                    ],
                    'close_connection': 1,
                },
                'structure': {
                    'id': {'name': 'id'},
                    'attributes': [
                        {
                            'name': 'text',
                            'type': 'String',
                        }
                    ],
                },
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <mysql>
                                <db>test_db</db>
                                <table>test_table</table>
                                <user>test_user</user>
                                <password>test_password</password>
                                <replica>
                                    <host>test_host</host>
                                    <priority>1</priority>
                                </replica>
                                <close_connection>1</close_connection>
                            </mysql>
                        </source>
                        <structure>
                            <id>
                                <name>id</name>
                            </id>
                            <attribute>
                                <name>text</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with ClickHouse source',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'clickhouse_source': {
                    'host': 'test_host',
                    'port': 9000,
                    'user': 'test_user',
                    'password': 'test_password',
                    'db': 'test_db',
                    'table': 'test_table',
                },
                'structure': {'id': {'name': 'id'}, 'attributes': [{'name': 'text', 'type': 'String'}]},
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <clickhouse>
                                <host>test_host</host>
                                <port>9000</port>
                                <user>test_user</user>
                                <password>test_password</password>
                                <db>test_db</db>
                                <table>test_table</table>
                            </clickhouse>
                        </source>
                        <structure>
                            <id>
                                <name>id</name>
                            </id>
                            <attribute>
                                <name>text</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with MongoDB source',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'mongodb_source': {
                    'host': 'test_host',
                    'port': 27017,
                    'user': 'test_user',
                    'password': 'test_password',
                    'db': 'test_db',
                    'collection': 'test_collection',
                },
                'structure': {'id': {'name': 'id'}, 'attributes': [{'name': 'text', 'type': 'String'}]},
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <mongodb>
                                <host>test_host</host>
                                <port>27017</port>
                                <user>test_user</user>
                                <password>test_password</password>
                                <db>test_db</db>
                                <collection>test_collection</collection>
                            </mongodb>
                        </source>
                        <structure>
                            <id>
                                <name>id</name>
                            </id>
                            <attribute>
                                <name>text</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with PostgreSQL source',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'postgresql_source': {
                    'table': 'test_table',
                },
                'structure': {'id': {'name': 'id'}, 'attributes': [{'name': 'text', 'type': 'String'}]},
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <odbc>
                                <connection_string>DSN=test_dict_dsn</connection_string>
                                <table>test_table</table>
                            </odbc>
                        </source>
                        <structure>
                            <id>
                                <name>id</name>
                            </id>
                            <attribute>
                                <name>text</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with YT source and cache layout',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'yt_source': {
                    'clusters': ['hahn'],
                    'table': '//home/TestTable',
                    'query': '''PageID, TargetType, 'Константа' as Constant''',
                    'user': 'test_user',
                    'token': 'test_token',
                },
                'structure': {
                    'id': {'name': 'PageID'},
                    'attributes': [
                        {'name': 'TargetType', 'type': 'Int32'},
                        {
                            'name': 'Constant',
                            'type': 'String',
                        },
                    ],
                },
                'layout': {
                    'type': 'cache',
                    'size_in_cells': 3000000,
                },
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <library>
                                <path>/usr/lib/libclickhouse_dictionary_yt.so</path>
                                <settings>
                                    <cluster>hahn</cluster>
                                    <table>//home/TestTable</table>
                                    <key>PageID</key>
                                    <query>PageID, TargetType, 'Константа' as Constant</query>
                                    <user>test_user</user>
                                    <token>test_token</token>
                                </settings>
                            </library>
                        </source>
                        <structure>
                            <id>
                                <name>PageID</name>
                            </id>
                            <attribute>
                                <name>TargetType</name>
                                <type>Int32</type>
                                <null_value></null_value>
                            </attribute>
                            <attribute>
                                <name>Constant</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <cache>
                                <size_in_cells>3000000</size_in_cells>
                            </cache>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with YT source and range hashed layout',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'yt_source': {
                    'clusters': ['hahn'],
                    'table': '//home/TestTable',
                    'query': '',
                    'user': 'test_user',
                    'token': 'test_token',
                },
                'structure': {
                    'id': {'name': 'ID'},
                    'range_min': {
                        'name': 'StartDate',
                    },
                    'range_max': {
                        'name': 'EndDate',
                    },
                    'attributes': [
                        {'name': 'Amount', 'type': 'UInt64'},
                    ],
                },
                'layout': {'type': 'range_hashed'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <library>
                                <path>/usr/lib/libclickhouse_dictionary_yt.so</path>
                                <settings>
                                    <cluster>hahn</cluster>
                                    <table>//home/TestTable</table>
                                    <key>ID</key>
                                    <date_field>StartDate</date_field>
                                    <date_field>EndDate</date_field>
                                    <field>Amount</field>
                                    <user>test_user</user>
                                    <token>test_token</token>
                                </settings>
                            </library>
                        </source>
                        <structure>
                            <id>
                                <name>ID</name>
                            </id>
                            <range_min>
                                <name>StartDate</name>
                            </range_min>
                            <range_max>
                                <name>EndDate</name>
                            </range_max>
                            <attribute>
                                <name>Amount</name>
                                <type>UInt64</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <range_hashed/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with YT source and field name mapping',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'yt_source': {
                    'clusters': ['hahn'],
                    'table': '//home/TestTable',
                    'keys': ['UserID'],
                    'fields': ['Field1', 'Field2'],
                    'user': 'test_user',
                    'token': 'test_token',
                    'cluster_selection': 'Random',
                    "use_query_for_cache": True,
                    "force_read_table": True,
                    "range_expansion_limit": 1000,
                    "input_row_limit": 1000,
                    "yt_socket_timeout_msec": 100000,
                    "yt_connection_timeout_msec": 200000,
                    "yt_lookup_timeout_msec": 30000,
                    "yt_select_timeout_msec": 40000,
                    "output_row_limit": 1024,
                    "yt_retry_count": 10,
                },
                'structure': {
                    'id': {'name': 'user_id'},
                    'attributes': [
                        {'name': 'field1', 'type': 'UInt64'},
                        {'name': 'field2', 'type': 'String'},
                    ],
                },
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <library>
                                <path>/usr/lib/libclickhouse_dictionary_yt.so</path>
                                <settings>
                                    <cluster>hahn</cluster>
                                    <table>//home/TestTable</table>
                                    <key>UserID</key>
                                    <field>Field1</field>
                                    <field>Field2</field>
                                    <user>test_user</user>
                                    <token>test_token</token>
                                    <cluster_selection>Random</cluster_selection>
                                    <use_query_for_cache>true</use_query_for_cache>
                                    <force_read_table>true</force_read_table>
                                    <range_expansion_limit>1000</range_expansion_limit>
                                    <input_row_limit>1000</input_row_limit>
                                    <yt_socket_timeout_msec>100000</yt_socket_timeout_msec>
                                    <yt_connection_timeout_msec>200000</yt_connection_timeout_msec>
                                    <yt_lookup_timeout_msec>30000</yt_lookup_timeout_msec>
                                    <yt_select_timeout_msec>40000</yt_select_timeout_msec>
                                    <output_row_limit>1024</output_row_limit>
                                    <yt_retry_count>10</yt_retry_count>
                                </settings>
                            </library>
                        </source>
                        <structure>
                            <id>
                                <name>user_id</name>
                            </id>
                            <attribute>
                                <name>field1</name>
                                <type>UInt64</type>
                                <null_value></null_value>
                            </attribute>
                            <attribute>
                                <name>field2</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with composite keys',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'postgresql_source': {
                    'table': 'test_table',
                },
                'structure': {
                    'key': {
                        'attributes': [
                            {'name': 'text', 'type': 'String', 'expression': 'lower(text)'},
                            {'name': 'text2', 'type': 'String', 'expression': 'lower(text2)'},
                        ]
                    },
                    'attributes': [
                        {'name': 'field1', 'type': 'UInt64'},
                        {'name': 'field2', 'type': 'String'},
                    ],
                },
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <odbc>
                                <connection_string>DSN=test_dict_dsn</connection_string>
                                <table>test_table</table>
                            </odbc>
                        </source>
                        <structure>
                            <key>
                                <attribute>
                                    <name>text</name>
                                    <type>String</type>
                                    <expression>lower(text)</expression>
                                </attribute>
                                <attribute>
                                    <name>text2</name>
                                    <type>String</type>
                                    <expression>lower(text2)</expression>
                                </attribute>
                            </key>
                            <attribute>
                                <name>field1</name>
                                <type>UInt64</type>
                                <null_value></null_value>
                            </attribute>
                            <attribute>
                                <name>field2</name>
                                <type>String</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Dictionary with XML-escaped string',
        'args': {
            'dictionary': {
                'name': 'test_dict',
                'mysql_source': {
                    'db': 'test_db',
                    'table': 'test_table',
                    'user': 'test_user',
                    'password': 'test_password',
                    'replicas': [
                        {
                            'host': 'test_host',
                            'priority': 1,
                        }
                    ],
                    'where': 'value is null or value &lt;&gt; 1',
                },
                'structure': {
                    'id': {'name': 'id'},
                    'attributes': [
                        {
                            'name': 'value',
                            'type': 'Int64',
                        }
                    ],
                },
                'layout': {'type': 'flat'},
                'fixed_lifetime': 300,
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <dictionary>
                        <name>test_dict</name>
                        <source>
                            <mysql>
                                <db>test_db</db>
                                <table>test_table</table>
                                <user>test_user</user>
                                <password>test_password</password>
                                <replica>
                                    <host>test_host</host>
                                    <priority>1</priority>
                                </replica>
                                <where>value is null or value &lt;&gt; 1</where>
                            </mysql>
                        </source>
                        <structure>
                            <id>
                                <name>id</name>
                            </id>
                            <attribute>
                                <name>value</name>
                                <type>Int64</type>
                                <null_value></null_value>
                            </attribute>
                        </structure>
                        <layout>
                            <flat/>
                        </layout>
                        <lifetime>300</lifetime>
                    </dictionary>
                </yandex>
                ''',
        },
    },
)
def test_render_dictionary_config(dictionary, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'ch_version': '21.8.14.5',
                    'config': {
                        'dictionaries': [dictionary],
                    },
                },
            },
        },
    )
    assert_xml_equals(mdb_clickhouse.render_dictionary_config(dictionary_id=0), result)
