"""
Recipe for salt-tests mdb_postgresql
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    return generic_start(
        config={
            'name': 'salt',
            'db': 'db1',
            'config': {
                'timezone': 'UTC',
                'max_connections': 64,
            },
        }
    )


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
