"""
Launcher for recipe for dbaas-internal-api
"""
from dbaas_internal_api_image_recipe_lib import start, stop
from library.python.testing.recipe import declare_recipe


def main():
    """
    Entry-point
    """
    declare_recipe(start, stop)
