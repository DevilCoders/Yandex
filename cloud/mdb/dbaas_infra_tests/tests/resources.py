"""
Contains valid resources for clusters.
"""

from psycopg2.extras import NumericRange


def generate():
    """
    Generate valid resources table contents
    """
    cluster_type_role_pairs = [
        ('postgresql_cluster', 'postgresql_cluster'),
        ('clickhouse_cluster', 'clickhouse_cluster'),
        ('clickhouse_cluster', 'zk'),
        ('elasticsearch_cluster', 'elasticsearch_cluster.datanode'),
        ('elasticsearch_cluster', 'elasticsearch_cluster.masternode'),
        ('opensearch_cluster', 'opensearch_cluster.datanode'),
        ('opensearch_cluster', 'opensearch_cluster.masternode'),
        ('mongodb_cluster', 'mongodb_cluster.mongod'),
        ('mongodb_cluster', 'mongodb_cluster.mongos'),
        ('mongodb_cluster', 'mongodb_cluster.mongocfg'),
        ('mongodb_cluster', 'mongodb_cluster.mongoinfra'),
        ('mysql_cluster', 'mysql_cluster'),
        ('redis_cluster', 'redis_cluster'),
        ('greenplum_cluster', 'greenplum_cluster.master_subcluster'),
        ('greenplum_cluster', 'greenplum_cluster.segment_subcluster'),
    ]
    flavors = [
        '21f29f4a-17c5-4403-923b-2fae3089027a',
        '325648ce-2daa-4046-b958-d5a9fe27ccfe',
        '33f89442-0994-41ff-984a-6763e45bba82',
        '6aaf915e-ceb2-4b46-8f18-b71c2d4694d1',
        'ad8e7269-c87b-41a7-bc2e-3db64ee23e1d',
        'c4b807f9-72ff-4322-aaba-e8a995b8bad5',
    ]
    extra_role_flavors = {
        "redis_cluster": [
            '6a9f8640-cb56-4afe-985d-0a407936ca50',
            '7e589410-c8c9-4a66-bac4-09d9dfb5de5e',
        ],
    }

    resources = []
    for pair in cluster_type_role_pairs:
        min_hosts = 1
        max_hosts = 10
        if pair[1] == 'zk':
            min_hosts = 3
            max_hosts = 5
        elif pair[0] == 'mongodb_cluster' and pair[1] != 'mongodb_cluster.mongod':
            min_hosts = 0
        for flavor in [] + flavors + extra_role_flavors.get(pair[1], []):
            for geo_id in range(1, 6):
                resources.append({
                    'cluster_type': pair[0],
                    'role': pair[1],
                    'flavor': flavor,
                    'disk_type_id': 1,
                    'geo_id': geo_id,
                    'disk_size_range': NumericRange(10737418240, 2199023255552),
                    'disk_sizes': None,
                    'min_hosts': min_hosts,
                    'max_hosts': max_hosts,
                })

    return resources


RESOURCES = generate()
