"""
Recipe for dbaas_metadb postgres
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    return generic_start(
        config={
            'name': 'metadb',
            'db': 'dbaas_metadb',
            'source_path': 'cloud/mdb/dbaas_metadb',
            'grants_path': 'head/grants',
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
