"""
Recipe for testdata for metadb
"""

from library.python.testing.recipe import declare_recipe
from util import config, metadb


class Context:
    def __init__(self):
        pass


def start(_):
    context = Context()
    context.conf = config.get_config()
    metadb.prepare(context)


def stop(_):
    pass


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
