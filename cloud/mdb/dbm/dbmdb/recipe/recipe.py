"""
Recipe for dbm postgres
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    return generic_start(
        config={
            'name': 'dbm',
            'db': 'dbm',
            'source_path': 'cloud/mdb/dbm/dbmdb',
            'config': {
                'lock_timeout': '5s',
            },
        }
    )


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
