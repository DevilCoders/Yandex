"""
Recipe for S3 postgres
"""
from cloud.mdb.recipes.postgresql.lib import stop
from library.python.testing.recipe import declare_recipe

from .lib import CLUSTER_CONFIG, PgCluster


def start(_):
    cluster = PgCluster(CLUSTER_CONFIG, 'cloud/mdb/pg')
    cluster.start()
    cluster.set_env()


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
