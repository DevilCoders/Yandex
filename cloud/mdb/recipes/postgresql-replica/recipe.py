"""
Recipe for testing replication of postgres recipe
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    master_config = {
        'name': 'master',
    }
    master_host, master_port = generic_start(config=master_config)
    replica_config = {
        'name': 'replica',
        'replication_source': 'host={host} port={port}'.format(
            host=master_host, port=master_port)
    }
    generic_start(config=replica_config)


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
