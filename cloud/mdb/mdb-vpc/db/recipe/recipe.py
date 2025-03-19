"""
Recipe for vpcdb postgres
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    return generic_start(
        config={
            'name': 'vpcdb',
            'db': 'vpcdb',
            'source_path': 'cloud/mdb/mdb-vpc/db',
            'users': ['vpc_worker', 'vpc_api'],
        }
    )


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
