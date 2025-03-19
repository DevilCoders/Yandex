"""
Recipe for postgres
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    generic_start(
        config={
            'name': 'pg',
        }
    )


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
