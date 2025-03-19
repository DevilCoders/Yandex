"""
Recipe for billingdb postgres
"""
from cloud.mdb.recipes.postgresql.lib import start as generic_start, stop
from library.python.testing.recipe import declare_recipe


def start(_):
    return generic_start(config={
        'name': 'billingdb',
        'db': 'billingdb',
        'source_path': 'cloud/mdb/billing/db'
    })


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
