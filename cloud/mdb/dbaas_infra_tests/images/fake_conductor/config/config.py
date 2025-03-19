# flake8: noqa
# pylint: skip-file

OAUTH_TOKEN = '{{ conf.projects.fake_conductor.config.oauth.token }}'

DCS = {
    'man': 1,
    'sas': 2,
    'vla': 3,
    'myt': 4,
    'iva': 5,
}

GROUPS = [
    {
        'id': 1,
        'name': 'mdb_porto_clickhouse',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 2,
        'name': 'mdb_other_prod',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 4,
        'name': 'mdb_porto_zookeeper',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 5,
        'name': 'mdb_porto_redis',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 6,
        'name': 'mdb_porto_mysql',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 7,
        'name': 'mdb_porto_mongod',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 8,
        'name': 'mdb_porto_mongos',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 9,
        'name': 'mdb_porto_mongocfg',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 10,
        'name': 'mdb_porto_elasticsearch_data',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 11,
        'name': 'mdb_porto_elasticsearch_master',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 12,
        'name': 'mdb_porto_mongoinfra',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 13,
        'name': 'mdb_porto_greenplum_master',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 14,
        'name': 'mdb_porto_greenplum_segment',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 15,
        'name': 'mdb_porto_opensearch_data',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
    {
        'id': 16,
        'name': 'mdb_porto_opensearch_master',
        'parent_ids': [],
        'export_to_racktables': False,
        'project': {
            'id': 1,
        },
    },
]
