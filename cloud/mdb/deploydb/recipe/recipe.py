"""
Recipe for deploydb postgres
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    return generic_start(config={'name': 'deploydb', 'db': 'deploydb', 'source_path': 'cloud/mdb/deploydb'})


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
