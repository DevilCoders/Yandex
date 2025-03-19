"""
Config for S3 postgres recipe
"""

CLUSTER_CONFIG = {
    'pgmeta': [
        # Shard
        [
            # Hosts in shard
            {'name': 's3pgmeta', 'dc': 'DC1'},
            {'name': 's3pgmeta_r', 'dc': 'DC2'},
        ],
    ],
    's3meta': [
        # Shard
        [
            # Hosts in shard
            {'name': 's3meta01', 'dc': 'DC1'},
            {'name': 's3meta01r', 'dc': 'DC2'},
        ],
        [
            {'name': 's3meta02', 'dc': 'DC1'},
            {'name': 's3meta02r', 'dc': 'DC2'},
        ],
    ],
    's3db': [
        # Shard
        [
            # Hosts in shard
            {'name': 's3db01', 'dc': 'DC1'},
            {'name': 's3db01r', 'dc': 'DC2'},
        ],
        [
            {'name': 's3db02', 'dc': 'DC1'},
            {'name': 's3db02r', 'dc': 'DC2'},
        ],
    ],
    'pgproxy': [
        # Fake shard, proxy has no shards
        [
            # Hosts in shard
            {'name': 's3proxy', 'dc': 'DC2'},
        ],
    ],
}
